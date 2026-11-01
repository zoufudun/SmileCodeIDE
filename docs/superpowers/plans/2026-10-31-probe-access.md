# Probe access repair

Scope authorized by the user's request: repair lifecycle conflicts across chip, project,
probe and toolchain changes, main IDE, plugin and standalone workbench.

## Findings
- Two verified orphaned OpenOCD servers held the attached ST-Link after their IDE exited.
- Per-controller cleanup cannot release a server belonging to another workbench instance.
- Debug configuration changes can retain an already-running server with obsolete settings.
- FlashProgrammer has no common hardware ownership gate across its six backends.
- Windows host-exit protection exists for debug servers, but not download tools.

## Implementation and validation
- [ ] Add common cross-module debug release and cross-process hardware exclusion.
- [ ] Apply host-exit protection to all hardware tool processes.
- [ ] Stop old debug sessions on all supported configuration transitions; reject changes during download/build.
- [ ] Preserve raw access errors and add actionable diagnostics; do not retry firmware writes.
- [ ] Test competing instances, abnormal exits, project/chip/probe/config transitions offline.
- [ ] Build main executable, plugin and standalone; verify attached probe using init/shutdown only.

Do not kill untracked third-party software or install/replace drivers automatically.
Do not claim to eliminate USB failures due to external software, drivers or physical hardware.
