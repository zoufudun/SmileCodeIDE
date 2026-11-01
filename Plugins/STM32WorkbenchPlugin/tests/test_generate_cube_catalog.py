import hashlib
from contextlib import closing
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "tools"))
import generate_cube_catalog as catalog


class CubeCatalogTest(unittest.TestCase):
    def test_selects_remaining_targets_from_one_family(self):
        entries = [{"referenceName": "STM32F100C4Tx", "family": "STM32F1"},
                   {"referenceName": "STM32F722RCTx", "family": "STM32F7"},
                   {"referenceName": "STM32H745ZITx", "family": "STM32H7"},
                   {"referenceName": "STM32H747XIHx", "family": "STM32H7"}]
        manifest = {"targets": {"STM32H745ZITx": {}}, "failures": {}}
        pending = catalog.select_pending(entries, manifest, {"STM32H7"}, False, 0)
        self.assertEqual([entry["referenceName"] for entry in pending],
                         ["STM32H747XIHx"])

    def test_selects_only_h743_reference_prefix(self):
        entries = [{"referenceName": "STM32H742AGIx", "family": "STM32H7"},
                   {"referenceName": "STM32H743VITx", "family": "STM32H7"},
                   {"referenceName": "STM32H745ZITx", "family": "STM32H7"}]
        pending = catalog.select_pending(entries, {"targets": {}},
                                         {"STM32H7"}, False, 0, "STM32H743")
        self.assertEqual([entry["referenceName"] for entry in pending],
                         ["STM32H743VITx"])

    def test_verify_one_family_ignores_other_incomplete_families(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            output = Path(scratch)
            linker = output / "h7.ld"
            linker.write_bytes(b"FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1M\n")
            entry = {"referenceName": "STM32H743VITx", "family": "STM32H7"}
            manifest = {"targets": {
                "STM32H743VITx": {"family": "STM32H7", "cores": ["Arm Cortex-M7"],
                                   "scripts": [{"path": "h7.ld",
                                                "sha256": catalog.hash_file(linker),
                                                "defines": ["STM32H743xx"]}]},
                "STM32F100C4Tx": {"family": "STM32F1", "scripts": []}},
                "failures": {"STM32F722RCTx": "not generated"}}
            self.assertTrue(catalog.verify_catalog(output, [entry], manifest,
                                                   ["STM32H7"]))

    def test_verify_h743_prefix_ignores_other_h7_failures(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            output = Path(scratch)
            linker = output / "h743.ld"
            linker.write_bytes(b"FLASH (rx) : ORIGIN = 0x08000000\n")
            entry = {"referenceName": "STM32H743VITx", "family": "STM32H7"}
            manifest = {"targets": {
                "STM32H743VITx": {"family": "STM32H7", "cores": ["Arm Cortex-M7"],
                                   "scripts": [{"path": "h743.ld",
                                                "sha256": catalog.hash_file(linker),
                                                "defines": ["STM32H743xx"]}]},
                "STM32H745ZITx": {"family": "STM32H7", "scripts": []}},
                "failures": {"STM32H742AGIx": "not generated"}}
            self.assertTrue(catalog.verify_catalog(output, [entry], manifest,
                                                   ["STM32H7"], "STM32H743"))

    def test_filter_entries_limits_generation_without_changing_inventory(self):
        entries = [
            {"referenceName": "STM32F100C4Tx", "family": "STM32F1"},
            {"referenceName": "STM32H743VITx", "family": "STM32H7"},
        ]
        self.assertEqual(
            catalog.filter_entries(entries, ["STM32H7"]),
            [entries[1]])
        self.assertEqual(catalog.filter_entries(entries, []), entries)

    def test_batch_keeps_results_when_scratch_cleanup_fails(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            root = Path(scratch)
            work = root / "work"
            work.mkdir()
            entry = {"referenceName": "STM32F100C4Tx"}
            completed = type("Completed", (), {"stdout": "", "stderr": "", "returncode": 0})()
            with patch.object(catalog.subprocess, "run", return_value=completed), \
                 patch.object(catalog, "extract_target", return_value={"scripts": []}), \
                 patch.object(catalog.shutil, "rmtree", side_effect=PermissionError("locked")):
                result = catalog.run_batch([entry], 0, work, root / "output",
                                           root / "java", root / "cube", 5)
            self.assertIn("STM32F100C4Tx", result[3])
            self.assertEqual(result[4], {})

    def test_timeout_keeps_cubemx_diagnostics(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            root = Path(scratch)
            work = root / "work"
            work.mkdir()
            error = subprocess.TimeoutExpired("CubeMX", 5,
                                              output=b"load STM32H7RS\n",
                                              stderr=b"firmware unavailable\n")
            with patch.object(catalog.subprocess, "run", side_effect=error):
                result = catalog.run_batch([{"referenceName": "STM32H7R3A8Ix"}],
                                           0, work, root / "output",
                                           root / "java", root / "cube", 5)
            self.assertIn("load STM32H7RS", result[2])
            self.assertIn("firmware unavailable", result[2])

    def test_inventory_uses_exact_reference_and_excludes_mp(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            root = Path(scratch)
            db = root / "devices.db"
            with closing(sqlite3.connect(db)) as connection:
                connection.execute("CREATE TABLE cpn (cpn TEXT, refname TEXT)")
                connection.executemany("INSERT INTO cpn VALUES (?, ?)", [
                    ("STM32F100C4T6B", "STM32F100C4Tx"),
                    ("STM32F100C4T7B", "STM32F100C4Tx"),
                    ("STM32MP157AA", "STM32MP157A"),
                ])
                connection.commit()
            xml = root / "families.xml"
            xml.write_text(
                '<Root><Mcu RefName="STM32F100C4Tx"><Core>Arm Cortex-M3</Core>'
                '<Flash>16</Flash><Ram>4</Ram></Mcu></Root>', encoding="utf-8")
            entries = catalog.inventory(db, xml)
            self.assertEqual(len(entries), 1)
            self.assertEqual(entries[0]["referenceName"], "STM32F100C4Tx")
            self.assertEqual(entries[0]["cpnAliases"],
                             ["STM32F100C4T6B", "STM32F100C4T7B"])

    def test_extract_preserves_linker_bytes_and_compiler_metadata(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as scratch:
            root = Path(scratch)
            project = root / "T0000"
            ide = project / "STM32CubeIDE"
            ide.mkdir(parents=True)
            original = b"MEMORY { FLASH : ORIGIN = 0x08000000 }\r\n\r\n"
            (ide / "STM32F100C4TX_FLASH.ld").write_bytes(original)
            (ide / ".cproject").write_text(
                '<cproject><option superClass="x.tool.c.compiler.option.definedsymbols">'
                '<listOptionValue value="DEBUG"/><listOptionValue value="USE_HAL_DRIVER"/>'
                '<listOptionValue value="STM32F100xB"/></option>'
                '<option superClass="x.fpu" value="x.value.fpv4-sp-d16"/>'
                '<option superClass="x.floatabi" value="x.value.hard"/>'
                '</cproject>', encoding="utf-8")
            (project / "T0000.ioc").write_bytes(b"Mcu.Name=STM32F100C4Tx\r\n")
            output = root / "package"
            entry = {"referenceName": "STM32F100C4Tx"}
            result = catalog.extract_target(project, output, entry)
            script = result["scripts"][0]
            self.assertEqual((output / script["path"]).read_bytes(), original)
            self.assertEqual(script["sha256"], hashlib.sha256(original).hexdigest())
            self.assertEqual(script["defines"], ["USE_HAL_DRIVER", "STM32F100xB"])
            self.assertEqual((script["fpu"], script["floatAbi"]), ("fpv4-sp-d16", "hard"))
            self.assertEqual((output / result["iocPath"]).read_bytes(),
                             b"Mcu.Name=STM32F100C4Tx\r\n")


if __name__ == "__main__":
    unittest.main()
