# STM32 build experience implementation plan

> Execute with subagent-driven-development for isolated model/navigation work and inline integration. Approved spec: ../specs/2026-10-16-stm32-build-experience-design.md.

**Goal:** Keil-style build results with concise/details output and actionable errors/warnings.

**Architecture:** A shared BuildLogModel owns parsing, retained output and diagnostics. BuildManager feeds it before notifying consumers; the widget renders its output and filtered problems. Source navigation connects to the actual host editor, with a read-only source preview fallback in standalone mode.

**Tech stack:** C++17, Qt6 Widgets/QtTest, GNU Make/GCC, existing QScintilla host.

## Task 1: Parsing model

Files: new Core/BuildDiagnostic.h, BuildLogModel.h/.cpp and tests/BuildLogModelTest.cpp/.pro.

- [x] Write and run failing tests for split streams, ANSI/CRLF, source/line/column diagnostics (Windows and Chinese paths), tool/linker-only errors, follow-on summary deduplication, stages, no-work, size output, cancellation/nonzero exit and warnings on successful exit.
- [x] Implement a focused model with reset(projectDir, action), append/flush returning newly parsed diagnostics, retained detailed/concise entries and diagnostics, counts and finish result. Shared entries carry severity and optional source location. Keep existing BuildDiagnostic source compatibility through its extracted header.
- [x] Verify model tests using Qt6.11.2 qmake and matching MinGW13.1. Store output in build/build-experience-fix/model-tests.

## Task 2: Source navigation

Files: new UI/SourceNavigator.h/.cpp and tests; plugin factory; targeted host MainWindow/AppHubWindow integration.

- [x] Inspect actual AppHub/IDE launch APIs and add failing navigation/fallback tests.
- [x] Add host invocation for opening a file at line/column, preserving modified editors and reusing open files. Do not change the IPluginContext vtable/ABI.
- [x] Add read-only source preview fallback for standalone/unavailable host and a useful message for missing files.
- [x] Test navigation independently, compile the host if its sources change, and smoke-test without touching user files.

## Task 3: BuildManager and Makefile integration

Files: Core/BuildManager.h/.cpp, MakefileGenerator.cpp, plugin/standalone/test .pro files.

- [x] Add harmless fake-make regressions covering success/warnings, linker-only failure, startup failure, cancellation, clean and rebuild. Observe relevant baseline failures.
- [x] Reset session before preflight; forward every complete line through the model once; emit shared diagnostic/result/log signals, drain trailing lines and use process outcome plus parsed errors for final success. Keep existing public output/memory signals.
- [x] Add ASCII stage markers `[BUILD:CC]`, `[BUILD:CXX]`, `[BUILD:AS]`, `[BUILD:LD]`, `[BUILD:HEX]`, `[BUILD:BIN]` to generated recipes while preserving echoed raw commands and command failure propagation. Verify existing quoting/source prerequisites remain intact.
- [x] Preserve real text/data/bss/Flash/RAM signals and no-work results; do not treat clean completion as compiled firmware. Rebuild clean-to-build transitions produce the final build summary.

## Task 4: Workbench UI

Files: UI/STM32WorkbenchWidget.h/.cpp and new tests/BuildExperienceWidgetTest.cpp/.pro.

- [x] Add failing replay tests for concise/details, warning/error colors/counts, issue filters, source activation, previous/next, sessions and existing flash/build interaction.
- [x] Add output/problems tabs, filtered diagnostic rows/counts, previous/next actions, shared details toggle. Keep problems until the next build, even when downloading/debugging.
- [x] Render actual model entries with clickable source locations; preserve full context in details and avoid repainting another task's view on late events. Wire navigation host/fallback. Keep the existing flash logger regression behavior.
- [x] Inspect screenshots for success, warnings and failure with representative source messages.

## Task 5: Verification and installation

- [x] Run model, build/backend, UI, source-navigation and affected existing download/probe/build suites. Check final diff for unrelated changes.
- [x] Build a temporary representative STM32 project with real local GCC/make, then incrementally rebuild; leave user project and MCU firmware unchanged.
- [x] Independent review, fix findings and rerun affected checks.
- [x] Build plugin, standalone and host if changed; startup smoke; request IDE closure only once tested binaries are ready. Back up and install, then compare SHA256 hashes.

Use existing build/f429-fix plugin/standalone build directories; dedicated test artifacts under build/build-experience-fix. Preserve all pre-existing working-tree changes and unrelated root planning files. User approved the complete design, so continue without another design/implementation permission prompt.
