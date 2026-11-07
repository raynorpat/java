# JDK 1.1.5 Phase 3 completion

Phase 3 is complete on `build/jdk115-unix-platform`. This reconstructs the
Mach/cthreads platform backend alongside the working Linux pthread backend.
It does not establish native DR2 execution; that remains the Rhapsody milestone.
The final implementation and review corrections are committed as `7a9d858`.

## Delivered

- Binary-backed thread/monitor layouts, Mach message condition waits,
  lifecycle, JNI attachment, enumeration, priorities and suspend/resume.
- Register/stack GC roots, allocation/capture rollback and fatal resume handling.
- Recursive monitors, persistent notify eligibility across polling gaps,
  timeout/interrupt/stop reacquisition, concurrent contention coverage.
- Mach heap allocation, dyld library/symbol loading, cthreads service locks
  and pipe/exec adapter; clocks/files/path contracts audited against DR2.
- `JDK115_UNIX_THREADS=cthreads` selects the reconstructed backend.
  Linux continues to default to `pthreads`.

## Final verification

| Gate | Result |
|---|---|
| Windows Debug | 114/114 passed |
| Windows Release | 114/114 passed |
| Linux Debug | 108 passed, one expected missing-audio-device skip |
| Linux Release | 108 passed, one expected missing-audio-device skip |
| Installed Windows/Linux Debug/Release images | All four passed |
| Complete VM object compilation with external Mach SDK headers | Passed, with documented host/SDK compatibility overlay |
| Independent whole-branch review | Two blocking findings fixed with RED/GREEN regressions; none deferred |

Full build/test logs are `out/phase3-final-{win,linux}-{debug,release}-{build,tests}.log`.
Installed-image logs are `out/phase3-final-image-{win,linux}-{debug,release}-{install,tests}.log`.
SDK logs are `out/phase3-sdk-{config,build}.log`. The installed-image check is
`tests/tools/phase2-image115.cmake`, supplied ROOT, IMAGE, OUT, PROBE_DIR and
GENERATOR (the matching configuration's jpeg115-fixtures executable).

The GC/monitor composition probe freezes a simulated peer while holding the
native queue mutex, then exercises real Single/enumeration/Multi code.
The notify tests deliver notify and notify-all in the native timeout/reacquire
gap. Both failures were reproduced before correction. Real Linux worker threads
exercise the reconstructed monitor logic over pthread-backed OS doubles.

## Limits and decisions

The [evidence ledger](2026-11-06-jdk115-unix-platform-evidence.md) records all
binary addresses, reconstruction deviations, test provenance and review fixes.
In particular, the available SDK cthread TLS offset differs from DR2's binary;
the implementation uses binary-confirmed offsets. The complete-object SDK
check mixes Linux libc with external Mach declarations and a jump-buffer
overlay; it is not a native DR2 link/run or full-libc ABI certification.

Native Rhapsody packaging, Foundation/AppKit integration and actual DR2 runtime
validation remain Phase 7. Solaris 1.1.8 was inventoried, with the owner's order
preserved: X11/Motif AWT first, then full Solaris reconstruction. No vendor
classes.zip or demo content was modified.
