# STM32 download information and probe serial design

## Goal

Automatically show the actual ST-Link serial and make successful download output clear and calm, following the concise stage-oriented presentation requested by the user.

## Current defects

- Windows identifies the current legacy ST-Link/V2 as USB VID0483 PID3748 instance C. That suffix is truncated and cannot be used as an OpenOCD serial. Previous fix correctly rejected it but left the displayed serial empty.
- OpenOCD writes ordinary information to stderr. The current frontend equates stderr with error severity and paints the entire successful download red.

## Approaches

1. Recommended: read the real USB serial and show concise download stages by default with an optional detailed log. Preserve all diagnostic text.
2. Smaller option: keep every raw line visible and only fix severity colors. Still exposes verbose startup and command output.
3. External helper: invoke st-info to obtain serial. Avoid because this adds installation/path dependencies and cannot reliably map multiple devices from its display text.

## User-facing behavior

- Refresh devices reads the actual USB serial through native Windows USB APIs when the probe is available. Handle legacy binary serials with the same representation OpenOCD accepts. No hardcoded device serial.
- A busy/unavailable device leaves its serial unknown; an explicitly entered valid serial is retained. Windows instance identity is never substituted for the serial.
- The device information label, device selection text, and serial field use the same resolved serial.
- Default download view contains file, target and connection metadata, then only confirmed stages. Example:

  ```text
  下载文件  stm32f429igtx.bin  ·  6.07 KB
  目标芯片  STM32F429IGT6  ·  0x08000000
  下载器    ST-Link V2  ·  63003700090000383036544E

  连接成功
  开始下载…
  编程完成
  校验通过
  下载完成，程序已复位运行。耗时 1.2 秒
  ```

- Do not invent a completed erase or reset stage from a start message. Only process completion can finalize overall success.
- Normal text uses the existing neutral foreground; confirmed successes green, warnings amber, real errors red.
- A visible detailed-log toggle shows the complete original output including command, banner and informational messages, with severity determined by content rather than output stream.
- Preserve multiline/chunked output and show the last partial line on process completion. Retain errors and warnings in the concise view.
- Build/compiler output and clickable source diagnostics retain their existing behavior.

## Implementation boundaries

- ProbeDetector owns USB descriptor decoding and serial lookup. Native reads are bounded and close handles immediately; no reset or firmware write is involved.
- A small flash-log presentation model owns line buffering, severity classification, stage recognition and raw/concise entries. UI only renders it and provides the toggle.
- FlashProgrammer routes both process streams through the same severity/progress handling; stderr by itself does not mean failure.
- Reuse existing Qt palette, console area and status bar; do not redesign unrelated controls.

## Validation

- Tests for legacy binary and standard USB serial descriptors, malformed/truncated data and existing probe selection behavior.
- Live read of the attached ST-Link serial without hardcoded expected selection logic.
- Recorded successful and failed OpenOCD sessions, mixed severities in a single chunk, partial lines, repeated stage events, and disabled verify/reset options.
- Widget-level checks that informational stderr is not red, detailed logs remain accessible and compiler diagnostics remain unaffected.
- Render Qt screenshots for visual inspection of serial display, concise successful output, detailed output and a genuine error.
- Compile plugin and standalone, retain DLL backup, install after IDE closes and verify hashes.

## Scope

This task reads probe metadata and replays recorded sessions for validation. It does not require another firmware write; any later hardware write follows the user's existing authorization and explicit scope.
