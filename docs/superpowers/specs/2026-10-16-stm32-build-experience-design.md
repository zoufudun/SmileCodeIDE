# STM32 build output and diagnostics experience

Status: approved by the user (complete proposal). Scope: the existing STM32 workbench in the plugin and standalone application.

## User outcome

The user supplied Keil's Build Output as the reference. The workbench should make the compiled files, final result, program size, errors and warnings easy to see, and allow navigation to actionable source locations. Reuse the existing dark theme and download-log conventions.

## Proposed experience

1. Default output is concise: target/configuration, `compiling filename...`, `assembling filename...`, `linking...`, HEX/BIN generation, actual program-size figures and a final success/failure, error/warning count and elapsed time. Include a clear no-work result for an up-to-date build. Only show stages actually reported by the build.
2. A `详细日志` switch shows original commands, tool paths and complete tool output; switching views preserves the session and diagnostics. Information is neutral, warnings amber, errors red, and successful completion green. A successful build with warnings keeps the warnings visible and reports their nonzero count.
3. A compact `问题` view/tab beside build output contains severity, file, line/column and message, with All / Errors / Warnings filters and counts. Clicking a source diagnostic opens its file at the specified line through the workbench's existing navigation signal. Add previous/next diagnostic actions without taking focus or opening files automatically during compilation. Diagnostics without a usable source location remain visible; navigation is disabled for those rows.
4. Build output retains clickable diagnostics. Compiler notes, include context, source excerpts and caret lines remain available in full output; warning/error messages remain visible in concise output. The issue list is the primary place for scanning actionable diagnostics.
5. A new build clears the prior build's output, problem entries and summary. Clean and rebuild have correct session transitions: clean is not labelled compilation success, and a successful clean phase followed by compilation produces one final build result. Failure before launching make still has an explicit failure result. Cancelled or failed processes never show success. A new download/debug session does not destroy the stored build problems or let late events overwrite another active view.

## Architecture and data flow

- Add a focused build-log/diagnostic model to buffer stdout and stderr independently, normalize ANSI color escapes and line endings, classify complete lines, retain raw details, expose concise entries and structured diagnostics. Flush final partial lines before summary. Share one diagnostic parse/count source between the backend and the UI rather than parsing/counting each diagnostic twice.
- Extend BuildManager's existing diagnostic/result signals as needed while retaining existing public consumers. Resolve relative file paths against the session's project directory. Support GCC `file:line:column`, `file:line`, fatal errors, warnings/notes and Windows drive-letter/Chinese paths. Treat linker undefined references, missing files, region overflow, collect2 and make failures as visible tool failures even when no source location is available.
- Counts represent actual compiler/linker diagnostics. Follow-on make/collect2 failure summaries must not double-count an already recorded underlying error; if no underlying diagnostic was parsed, expose a tool failure instead of claiming zero errors. Nonzero exit or abnormal termination remains a failed build independently of counts. Repeated diagnostics from different source locations are not silently dropped.
- Give generated Makefiles stable, small stage announcements for compilation/assembly/link/HEX/BIN steps while retaining full commands for detailed output. Preserve quoting, prerequisites, dependency generation and command failure propagation for long Chinese Windows paths. Incremental builds only announce commands that actually execute. Unknown output stays accessible in details, and warning/error-like failures remain visible by default.
- Keep the size parser's existing memory-usage signal and display real `text`, `data`, `bss`, Flash (`text+data`) and RAM (`data+bss`). Do not fabricate Keil-specific Code/RO-data partitions from insufficient data. Do not show stale sizes as the current result after a failed build.
- The workbench renders model entries and the issue list; build diagnostic navigation also needs to be wired through the actual IDE host/plugin interface, not only emitted as an unhandled signal. Inspect existing host APIs before implementation and use the available source-open/line-navigation integration. The standalone retains its existing navigation signal for external consumers and must expose a clear fallback if it has no source editor.
- Keep the existing download logger, probe serial fix and hardware programming behavior intact. No firmware writes are needed for this change.

## Verification and delivery

Use recorded/fixture build output and a harmless local fake make executable to cover successful builds, warnings, compiler errors, linker-only errors, startup failure, cancellation, no-work and clean/rebuild transitions. Cover split chunks/final partial lines, ANSI coloring, Windows/Chinese paths, count consistency, view switching, filters, keyboard diagnostic navigation and host source navigation. Check that generated Makefiles still build a representative temporary STM32 project with the real local compiler; do not introduce intentional errors into the user's project. Run existing build/probe/download regressions affected by the change. Save and inspect success, warning and failure UI screenshots. Build the plugin and standalone, smoke-test startup, then back up/install after the IDE is closed and verify hashes.

## Alternatives

- Recommended: concise build output plus a separate diagnostics view. Slightly more UI work, but gives both the Keil-style progress and fast error/warning navigation requested by the user.
- Smaller: concise output with clickable colored lines only. Easier to implement, but finding a particular warning in a long build remains cumbersome.
- Larger: redesign the entire terminal/debug workspace. Out of scope for this request and would disturb the established workflow.
