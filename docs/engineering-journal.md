# Engineering journal

## Milestone 0

Implemented a minimal executable and target-scoped CMake configuration. Chose
borrowed string_view comparisons for argv with a lifetime bounded by main; no
heap ownership, worker thread or application class is necessary yet.

Verified on Linux with GCC 13.3.0, CMake 4.4.3 and Ninja 1.13.2:
- Debug configuration and compilation with all specified warnings and -Werror.
- Executable output and the six-case CTest CLI contract.

The environment initially lacked CMake/Ninja; installed these build tools before
validation. No application defect or optimization has been investigated yet.
Do not describe this as a concurrency, debugging or performance story in interviews.

## Milestone 1

Implemented typed PID parsing, filesystem discovery, owned complete/partial/error
results, a core library target, and --list-pids with an injectable --proc-root.
Version advanced to 0.2.0. Added Catch2 3.8.1 fixture tests and sanitizer options.

Verification with GCC 13.3.0 and warnings-as-errors:
- Debug build: both CTest targets passed; 47 Catch2 assertions passed across eight
  executed cases; one permission test explicitly skipped because execution was root.
- CLI contract: nine scenarios, including deterministic fixture output and missing root.
- ASan/UBSan build: both targets passed with ASAN_OPTIONS=detect_leaks=0 and
  UBSAN_OPTIONS=halt_on_error=1. Default LeakSanitizer aborted opening /proc/PID/task;
  leak checking remains unverified. No sanitizer suppression was added to CMake.
- Live proc smoke test passed using /proc/self identity for this mounted view.

Actual regression: ENOTDIR was initially mistaken for a vanished entry because
file_type::not_found was accepted despite an error_code. The test failed before the
classification fix. A second test exposed getpid versus proc-view identity mismatch.
See milestone-1.md for reproduction, diagnosis, fix and limits. No GDB, Valgrind,
strace, perf, race debugging or benchmark claims are supported by this milestone.
