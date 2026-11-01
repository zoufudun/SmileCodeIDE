# STM32 CubeMX catalog implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a versioned offline STM32 MCU target/linker catalog from STM32CubeMX 6.18.0 and make the workbench use exact supplied linker bytes; validate ST-Link V2 and CMSIS-DAP flashing.

**Architecture:** A Python packaging tool reads CubeMX's device inventory, invokes CubeMX for canonical generated projects, and writes an audited manifest plus byte-preserved scripts. A Qt repository component resolves exact target/context and script, while the existing build and flashing engines consume the resolved result and fail explicitly on missing mappings.

**Tech Stack:** Python 3 standard library, STM32CubeMX 6.18.0 CLI, Qt 6/C++17, qmake, OpenOCD, GNU Arm toolchain.

---

## Files and responsibilities

- Create `Plugins/STM32WorkbenchPlugin/tools/build_cubemx_catalog.py`: inventory, generation, hash/coverage report, deterministic packaging.
- Create `Plugins/STM32WorkbenchPlugin/data/cubemx-6.18.0/`: manifest, original linker files, license and coverage report.
- Create `Plugins/STM32WorkbenchPlugin/Core/LinkerScriptRepository.h/.cpp`: manifest lookup, hash verification and byte copy.
- Modify `Plugins/STM32WorkbenchPlugin/Core/STM32Profile.h/.cpp`: strict target lookup and catalog metadata.
- Modify `Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.cpp`: choose project script or repository script; no synthetic fallback.
- Modify `Plugins/STM32WorkbenchPlugin/UI/STM32WorkbenchWidget.cpp` and `UI/ToolchainConfigDialog.cpp`: searchable exact target/context selection.
- Modify `Plugins/STM32WorkbenchPlugin/Core/FlashProgrammer.cpp` and `UI/FlashConfigDialog.cpp`: adapter/target validation.
- Update `STM32WorkbenchPlugin.pro` and `STM32WorkbenchStandalone.pro` to ship data and repository component.
- Extend `tests/ToolchainDetectorTest.cpp`, `tests/ProbeDetectorTest.cpp`, and add a Python packaging test.

## Task 1: Prove CubeMX generation and package semantics

- [ ] Write a minimal generation script for H743VITx in a disposable workspace and invoke `D:/Soft/STM32CubeMX/jre/bin/java.exe -jar D:/Soft/STM32CubeMX/STM32CubeMX.exe -q <script>`.
- [ ] Inspect generated `.ld`, compiler flags, project metadata, and compare the script bytes with CubeIDE linkerdb's H743XI template using SHA-256 and `fc /b` or Python bytes.
- [ ] Record the canonical CLI input and any required firmware package/configuration. If generation fails, diagnose before scaling to all devices.
- [ ] Commit the proved invocation and notes in the packaging tool; do not claim full coverage from the 317 linkerdb source files.

## Task 2: Catalog generator and coverage report

- [ ] Add a Python unit test that parses a fixture `families.xml`, excludes STM32MP, keeps exact `RefName` and RPN, and rejects duplicate/conflicting target identities. Run it and confirm the intended failure.
- [ ] Implement inventory parsing and run the test green. Assert local inventory counts (2666 non-MP entries, 1643 unique RPNs) against CubeMX 6.18.0 without hard-coding them into runtime behavior.
- [ ] Add a failing test for byte preservation, SHA-256 manifest generation, context key collisions, and missing `.ld` coverage.
- [ ] Add dual-core and TrustZone fixtures. Enumerate expected core IDs, Secure/NonSecure states and FLASH/RAM variants from CubeMX/device metadata and generation options. Compare the expected context set with captured artifacts; a default-only generation must fail when more contexts are expected.
- [ ] Extract each target/context's CPU, ABI/FPU flags and defines from CubeMX-generated toolchain files. Cross-check core, FPU and memory-reporting fields against device XML; store field-level provenance and fail on missing or contradictory values.
- [ ] Implement canonical per-target/context generation and manifest assembly; preserve full file bytes, canonical CLI settings and provenance. Deduplicate only after byte comparison.
- [ ] Run the whole 2666-entry generation. Write a machine-readable coverage report and stop with nonzero status if any target/context or compiler metadata lacks an audited mapping. Investigate failures and rerun until clean; otherwise state exactly which entries remain uncovered.
- [ ] Include the ST license text and notices. Verify packaged scripts against generation output, not only linkerdb source.

## Task 3: Strict runtime lookup and linker selection

- [ ] Write Qt tests that unknown/ambiguous targets fail, H743VI resolves exact context, existing project scripts are untouched, repository bytes are copied unchanged, and corrupt/missing scripts fail before Makefile creation. Run red.
- [ ] Add `LinkerScriptRepository` with manifest/hash handling; change `STM32ProfileManager` to load exact catalog entries and remove F103 fallback; run green.
- [ ] Change `MakefileGenerator` to use explicit project script or repository script. For old SmileCodeIDE generated scripts, require explicit migration and backup. Run green and ensure custom scripts remain untouched.
- [ ] When multiple project-owned `.ld` candidates exist, require an explicit selection. Show script source, target and context in the UI and build log; test both behaviors.
- [ ] Add search/filter and context choice in both selectors; persist exact target/context. Use separate build outputs and linker choices for different cores. Verify with an automated Qt test where possible and manual UI inspection.
- [ ] Build standalone and plugin and inspect deploy payload paths.

## Task 4: ST-Link V2 and CMSIS-DAP flash path

- [ ] Add failing tests for both interface configs, serial selection, missing target/interface scripts, offline/mismatched probes, unknown target and no F1 fallback.
- [ ] Implement explicit OpenOCD target compatibility table and preflight validation; retain custom script overrides and current verify/reset/address behavior. Run green.
- [ ] Run OpenOCD syntax/config checks for representative target families. If physical ST-Link V2 and CMSIS-DAP probes are available, flash and verify on a compatible board; otherwise report hardware validation pending.

## Task 5: End-to-end gate

- [ ] Run full catalog coverage/hash validation; confirm zero missing mappings before claiming all CubeMX 6.18.0 non-MP targets.
- [ ] Byte-compare packaged scripts with generated output for single-core, H743 split-RAM, dual-core, TrustZone and newest-family samples.
- [ ] Run Qt tests, standalone/plugin builds, and H743VI real project build. Read fresh output and verify `_estack` resolves and vector SP is plausible.
- [ ] Inspect Git diff for only task files, inspect generated package and license notices, update README and walkthrough with achieved coverage and any limitations.
- [ ] Commit only task files. Do not clean or reset unrelated pre-existing worktree changes.
