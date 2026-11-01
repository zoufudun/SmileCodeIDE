# STM32 flash experience implementation plan

**Goal:** Display the actual probe serial and concise, correctly colored download logs with full details available.

**Architecture:** Native Windows USB descriptor reads in ProbeDetector; an independent FlashLogModel buffers and classifies process lines; the existing workbench renders concise or full entries. Compiler output retains its current renderer.

**Tech stack:** C++17, Qt6, SetupAPI/ConfigManager and Windows USB hub IOCTLs, QtTest.

The user approved the concise/default plus details-toggle design. Spec review approved. Existing unrelated workspace changes remain intact. No firmware write is needed for this implementation.

## Task 1: Actual USB serial

- [x] Add a live regression that requires an attached legacy ST-Link with a short Windows instance suffix to resolve a full serial; run and observe failure.
- [x] Add tested descriptor decoding for standard and legacy ST-Link serial formats, malformed data and embedded NULs.
- [x] Read descriptor bytes through the parent USB hub for the selected physical device/port; validate VID/PID, bound native I/O and close handles. This avoids claiming the debug interface.
- [x] Update ProbeDetector::scan and FlashConfigDialog field/info display consistently, retaining manual fallback when unavailable.
- [x] Run serial/selection regressions and inspect the attached probe result.

Files: Core/ProbeDetector.cpp/.h, UI/FlashConfigDialog.cpp, tests/ProbeDetectorTest.cpp.

## Task 2: Download log presentation model

- [x] Create Core/FlashLogModel.h/.cpp and focused QtTest cases.
- [x] Separate stdout/stderr line buffers, flush partial final lines, classify severity by contents, retain full raw lines and concise stage entries.
- [x] Keep warnings/errors visible; deduplicate recognized stages; do not claim erase/reset/overall success prematurely.
- [x] Cover recorded successful and failed OpenOCD output, split chunks and disabled verify/reset flags.

## Task 3: Integration and UI

- [x] Route FlashProgrammer output through shared content-based classification and progress handling for both streams, report process startup failures clearly.
- [x] Add a compact download log toolbar with a detailed-log toggle to STM32WorkbenchWidget; initialize per download with file/device/address metadata.
- [x] Render neutral information, green success, amber warnings and red errors; preserve full details and existing build/source links.
- [x] Handle flashFinished to flush partial output and finalize summary/status only after process completion.
- [x] Add widget replay checks, including informational stderr color and real errors.

Files: Core/FlashProgrammer.cpp/.h; UI/STM32WorkbenchWidget.cpp/.h; plugin/standalone/test .pro files.

## Task 4: Verification and delivery

- [x] Run all probe, log and UI replay tests. Review code changes independently.
- [x] Save and inspect Qt screenshots: serial dialog, concise success, details and failure.
- [x] Build plugin and standalone; smoke-test startup without programming hardware.
- [x] After user closes IDE, back up and replace active DLL; verify SHA256.
- [x] Report results and any remaining limitations.

Build environment: Qt6.11.2 with D:/Soft/Qt/Tools/mingw1310_64/bin first on PATH. Existing build/f429-fix/plugin and standalone directories can be reused. Tests/verification artifacts go in build/flash-experience-fix. K: shell writes require sandbox escalation; apply_patch can edit workspace source.
