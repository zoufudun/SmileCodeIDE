# STM32 H7 catalog and linker migration implementation plan

> **For agentic workers:** Execute this plan in the current session with executing-plans. Preserve all pre-existing workspace changes.

**Goal:** Make the legacy-linker error actionable, reset failed build status, and finish the existing CubeMX four-family offline package including H7.

**Architecture:** Keep default generation non-destructive. Expose an explicit migration API that checks the bundled script hash, preserves the old bytes in a unique backup, and atomically replaces only a recognized legacy script. BuildManager emits a migration request; the widget asks the user before migration and retry. Complete the existing catalog through actual CubeMX generation, preserving dual-core contexts and exact bytes.

**Tech Stack:** Qt C++17, Qt Test, Python unittest, STM32CubeMX 6.18.0, qmake/MinGW.

## Task 1: Migration and failed status

- [ ] Add failing Qt regression tests for successful backup/migration, incorrect hash, custom-script preservation, repeated backup names, and migration-needed build signaling.
- [ ] Compile and run targeted tests, confirming expected failures before implementation.
- [ ] Add migration API to `Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.*` using QSaveFile, exact legacy marker detection and unique `.bak` filenames.
- [ ] Connect migration signaling in `Core/BuildManager.*` and confirmation/retry in `UI/STM32WorkbenchWidget.*` for Build and Rebuild.
- [ ] Update build completion state for all early failures and normal completion.
- [ ] Run migration tests and existing script-preservation/hash/ambiguity/core regression tests.

## Task 2: Complete catalog

- [ ] Locate current CubeFinder SQLite and CubeMX/firmware installations; inspect existing generation process before resuming.
- [ ] Run existing full-family coverage test to document missing H7.
- [ ] Resume `tools/generate_cube_catalog.py` with the verified inventory and two workers, persisting manifest/qrc after each batch.
- [ ] Run `--verify-only`, requiring 650/650 targets, H7 200/200, correct aliases, hashes and core metadata.

## Task 3: Build, verify, deliver

- [ ] Rebuild/run Python tests and full Qt toolchain/probe tests with current resources.
- [ ] Build plugin and standalone with the installed compatible Qt toolchain.
- [ ] Verify a copied H743 project with legacy script migration; preserve the user's original E: project.
- [ ] Deploy verified plugin, retaining a recoverable prior DLL; report restart requirement.
- [ ] Update walkthrough with actual outcomes and any remaining limitations.
