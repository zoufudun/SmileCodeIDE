#!/usr/bin/env python3
"""Generate byte-preserved CubeMX linker scripts for F1, F4, F7 and H7."""

from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
import shutil
import sqlite3
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

FAMILIES = ("STM32F1", "STM32F4", "STM32F7", "STM32H7")

def filter_entries(entries: list[dict], families: list[str]) -> list[dict]:
    selected = set(families)
    return entries if not selected else [entry for entry in entries
                                          if entry["family"] in selected]


def select_pending(entries: list[dict], manifest: dict, families: set[str],
                   refresh: bool, limit: int,
                   reference_prefix: str = "") -> list[dict]:
    selected = [entry for entry in filter_entries(entries, list(families))
                if entry["referenceName"].startswith(reference_prefix)]
    pending = selected if refresh else [entry for entry in selected
                                        if entry["referenceName"] not in manifest["targets"]]
    return pending[:limit] if limit else pending

def inventory(database: Path, families_xml: Path) -> list[dict]:
    connection = sqlite3.connect(f"file:{database.as_posix()}?mode=ro", uri=True)
    cpns: dict[str, set[str]] = {}
    for cpn, reference in connection.execute("SELECT cpn, refname FROM cpn WHERE refname IS NOT NULL"):
        if any(reference.startswith(family) for family in FAMILIES):
            cpns.setdefault(reference, set()).add(cpn)
    connection.close()
    metadata = {}
    for mcu in ET.parse(families_xml).iter("Mcu"):
        reference = mcu.get("RefName")
        if reference in cpns:
            metadata[reference] = {
                "referenceName": reference,
                "cpnAliases": sorted(cpns[reference]),
                "family": next(f for f in FAMILIES if reference.startswith(f)),
                "core": mcu.findtext("Core", ""),
                "cores": [core.text for core in mcu.findall("Core")],
                "flashKiB": mcu.findtext("Flash", ""),
                "ramKiB": mcu.findtext("Ram", ""),
            }
    missing = cpns.keys() - metadata.keys()
    if missing:
        raise RuntimeError(f"{len(missing)} CubeFinder references absent from families.xml")
    return [metadata[reference] for reference in sorted(metadata)]

def hash_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def save_manifest(path: Path, manifest: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", newline="\n",
                                     dir=path.parent, delete=False) as stream:
        json.dump(manifest, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
        temporary = Path(stream.name)
    temporary.replace(path)

def save_qrc(path: Path, output: Path, manifest: dict) -> None:
    import xml.sax.saxutils
    def file_line(source: Path, alias: str) -> str:
        relative = Path(os.path.relpath(source, path.parent)).as_posix()
        return f'    <file alias="{xml.sax.saxutils.escape(alias)}">{xml.sax.saxutils.escape(relative)}</file>'
    lines = ['<RCC>', '  <qresource prefix="/stm32/full">',
             file_line(output / "manifest.json", "manifest.json")]
    for reference in sorted(manifest["targets"]):
        for script in manifest["targets"][reference]["scripts"]:
            lines.append(file_line(output / script["path"], script["path"]))
    lines.extend(('  </qresource>', '</RCC>', ''))
    path.write_text("\n".join(lines), encoding="utf-8")

def verify_catalog(output: Path, entries: list[dict], manifest: dict,
                   families: list[str] | None = None,
                   reference_prefix: str = "") -> bool:
    expected = {entry["referenceName"] for entry in entries}
    targets = {name: target for name, target in manifest["targets"].items()
               if (not families or target.get("family") in families)
               and name.startswith(reference_prefix)}
    problems = []
    for reference in sorted(expected - targets.keys()):
        problems.append(f"{reference}: target missing")
    for reference in sorted(targets.keys() - expected):
        problems.append(f"{reference}: unexpected target")
    for reference, target in targets.items():
        if not target.get("scripts"):
            problems.append(f"{reference}: no linker scripts")
        if len(target.get("cores", [])) > 1:
            names = [script.get("sourceRelativePath", "").upper()
                     for script in target.get("scripts", [])]
            for core in ("CM7", "CM4"):
                if not any(f"/{core}/" in name or f"_{core}_" in name for name in names):
                    problems.append(f"{reference}: missing {core} linker context")
        for script in target.get("scripts", []):
            path = output / script["path"]
            if not path.is_file() or hash_file(path) != script["sha256"]:
                problems.append(f"{reference}: hash mismatch: {script['path']}")
            if not any(symbol.startswith("STM32") for symbol in script.get("defines", [])):
                problems.append(f"{reference}: missing CubeMX device macro: {script['path']}")
    problems.extend(f"{reference}: {reason}" for reference, reason in
                    manifest.get("failures", {}).items()
                    if (not families or any(reference.startswith(family) for family in families))
                    and reference.startswith(reference_prefix))
    for problem in problems[:30]:
        print(problem)
    counts = {family: sum(name.startswith(family) for name in targets) for family in FAMILIES}
    print(f"verified={len(targets)}/{len(expected)} families={counts} problems={len(problems)}")
    return not problems

def extract_target(project: Path, destination: Path, entry: dict) -> dict:
    scripts = sorted(project.rglob("*.ld"))
    if not scripts:
        raise RuntimeError("CubeMX produced no .ld")
    result = dict(entry)
    result["scripts"] = []
    configurations = []
    for cproject in project.rglob(".cproject"):
        defines = []
        fpu = ""
        float_abi = ""
        for option in ET.parse(cproject).iter("option"):
            kind = option.get("superClass", "")
            if kind.endswith(".tool.c.compiler.option.definedsymbols") and not defines:
                defines = [value.get("value") for value in option.findall("listOptionValue")]
            elif kind.endswith(".fpu") and not fpu:
                fpu = option.get("value", "").rsplit(".value.", 1)[-1]
            elif kind.endswith(".floatabi") and not float_abi:
                float_abi = option.get("value", "").rsplit(".value.", 1)[-1]
        configurations.append((cproject.parent, {
            "defines": [value for value in defines if value and value != "DEBUG"],
            "fpu": fpu, "floatAbi": float_abi,
        }))
    for source in scripts:
        relative = source.relative_to(project).as_posix()
        target = destination / "linker" / entry["referenceName"] / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        compilation = next((settings for folder, settings in configurations
                            if folder == source.parent or folder in source.parents),
                           {"defines": [], "fpu": "", "floatAbi": ""})
        result["scripts"].append({
            "path": target.relative_to(destination).as_posix(),
            "sourceRelativePath": relative,
            "sha256": hash_file(target),
            "bytes": target.stat().st_size,
            **compilation,
        })
    ioc = project / f"{project.name}.ioc"
    if ioc.is_file():
        target = destination / "ioc" / f"{entry['referenceName']}.ioc"
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ioc, target)
        result["iocPath"] = target.relative_to(destination).as_posix()
        result["iocSha256"] = hash_file(target)
    return result

def run_batch(batch: list[dict], offset: int, work: Path, output: Path,
              java: Path, executable: Path, timeout: int) -> tuple:
    scratch_path = Path(tempfile.mkdtemp(prefix="cube-", dir=work)).resolve()
    try:
        if work.resolve() not in scratch_path.parents:
            raise RuntimeError("temporary project escaped workspace")
        commands = []
        projects = {}
        for number, entry in enumerate(batch):
            name = f"T{number:04d}"
            projects[entry["referenceName"]] = scratch_path / name
            commands.extend((f"load {entry['referenceName']}", f"project name {name}",
                             "project toolchain STM32CubeIDE",
                             f"project path {scratch_path.as_posix()}", "project generate"))
        commands.append("exit")
        script = scratch_path / "generate.txt"
        script.write_text("\n".join(commands) + "\n", encoding="ascii")
        try:
            completed = subprocess.run([str(java), "-jar", str(executable), "-q", str(script)],
                cwd=scratch_path, capture_output=True, text=True, errors="replace",
                timeout=timeout, check=False)
            log = (completed.stdout or "") + "\n" + (completed.stderr or "")
            returncode = completed.returncode
        except subprocess.TimeoutExpired as exc:
            def decode(value: bytes | str | None) -> str:
                if isinstance(value, bytes):
                    return value.decode("utf-8", errors="replace")
                return value or ""
            log = decode(exc.stdout) + "\n" + decode(exc.stderr) + "\n" + str(exc)
            returncode = -1
        results = {}
        failures = {}
        for entry in batch:
            reference = entry["referenceName"]
            try:
                results[reference] = extract_target(projects[reference], output, entry)
            except Exception as exc:
                failures[reference] = str(exc)
        return offset, returncode, log, results, failures
    finally:
        try:
            shutil.rmtree(scratch_path)
        except OSError:
            # A transient Windows file lock must not discard completed targets.
            pass

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cubemx", type=Path, required=True)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--qrc", type=Path)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--refresh", action="store_true",
                        help="Regenerate completed targets to refresh scripts and metadata")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--batch-size", type=int, default=10)
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--timeout", type=int, default=1800)
    parser.add_argument("--family", action="append", choices=FAMILIES, default=[],
                        help="Generate only this family; may be repeated")
    parser.add_argument("--reference-prefix", default="",
                        help="Generate or verify only matching reference names, e.g. STM32H743")
    args = parser.parse_args()
    if args.batch_size < 1 or args.limit < 0 or args.jobs < 1:
        parser.error("batch size and jobs must be positive; limit must be nonnegative")
    cube = args.cubemx.resolve()
    java = cube / "jre" / "bin" / "java.exe"
    executable = cube / "STM32CubeMX.exe"
    database = args.database.resolve()
    output = args.output.resolve()
    qrc = args.qrc.resolve() if args.qrc else None
    for needed in (java, executable, database, cube / "db" / "mcu" / "families.xml"):
        if not needed.is_file():
            parser.error(f"missing required input: {needed}")
    entries = inventory(database, cube / "db" / "mcu" / "families.xml")
    reference_prefix = args.reference_prefix.upper()
    generation_entries = [entry for entry in filter_entries(entries, args.family)
                          if entry["referenceName"].startswith(reference_prefix)]
    if not generation_entries:
        parser.error("no CubeMX references match the requested family and prefix")
    manifest_path = output / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {
        "generator": "STM32CubeMX 6.18.0",
        "toolchain": "STM32CubeIDE",
        "families": list(FAMILIES),
        "expectedReferenceCount": len(entries),
        "targets": {},
        "failures": {},
    }
    # CubeFinder revisions can add/remove references while retaining completed
    # byte-preserved targets. Reconcile the inventory instead of treating that
    # normal revision as corruption; verify-only still reports missing targets.
    manifest["expectedReferenceCount"] = len(entries)
    if args.verify_only:
        return 0 if verify_catalog(output, generation_entries, manifest,
                                   args.family, reference_prefix) else 1
    entry_by_name = {entry["referenceName"]: entry for entry in entries}
    for reference, result in manifest["targets"].items():
        result.update(entry_by_name[reference])
    output.mkdir(parents=True, exist_ok=True)
    pending = select_pending(entries, manifest, set(args.family), args.refresh,
                             args.limit, reference_prefix)
    work = output.parent / (output.name + "-work")
    work.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = []
        for offset in range(0, len(pending), args.batch_size):
            batch = pending[offset:offset + args.batch_size]
            futures.append(pool.submit(run_batch, batch, offset, work, output,
                                       java, executable, args.timeout))
        for future in as_completed(futures):
            offset, returncode, log, results, failures = future.result()
            manifest["targets"].update(results)
            for reference in results:
                manifest["failures"].pop(reference, None)
            manifest["failures"].update(failures)
            if returncode != 0 or failures:
                log_path = output / "logs" / f"batch-{offset:04d}.log"
                log_path.parent.mkdir(exist_ok=True)
                log_path.write_text(log, encoding="utf-8")
            save_manifest(manifest_path, manifest)
            if qrc:
                save_qrc(qrc, output, manifest)
            print(f"{len(manifest['targets'])}/{len(entries)} targets, "
                  f"{len(manifest['failures'])} unresolved", flush=True)
    selected = {entry["referenceName"] for entry in generation_entries}
    unresolved = selected - manifest["targets"].keys()
    failed = selected & manifest["failures"].keys()
    return 0 if not unresolved and not failed else 1

if __name__ == "__main__":
    raise SystemExit(main())
