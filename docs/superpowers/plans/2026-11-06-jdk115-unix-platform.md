# JDK 1.1.5 Unix Platform (Phase 3) Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for inline execution.

**Goal:** Reconstruct the Rhapsody Mach/cthreads platform backend alongside the
retained pthreads backend, keeping the Phase 1/2 Linux image operational.

**Architecture:** Follow the already approved Unix thread-model decision:
conditional platform implementations, with pthreads selected on Linux. Recover
cthreads data layouts and Mach operations from the original binary before
writing them. Keep the shared VM and its established bug fixes intact.

**Tech Stack:** C89, 32-bit CMake/CTest on Windows/Linux; Mach/cthreads for DR2.

**Spec:** ../specs/2026-11-06-jdk115-reconstruction-design.md

## Constraints and baseline

- C89, 32-bit only. No edits to classes.zip or the shipped demos.
- Preserve existing GC, interruption and attachment fixes as marked deviations.
- Source layout stays under src/unix for the shared Unix backend selection;
  later DR2 build files and AppKit work belong to the Rhapsody milestone.
- Actual DR2 execution is not established by Linux tests or a mocked Mach API.
- Phase 2 merged to master at be7d0a4, fast-forward from bdedb1a. Pre-merge and
  post-merge Windows Debug suites passed 106/106. Phase 2 had already established
  Linux Release/Debug 94 passed plus the explicit missing-audio-device skip.
- Work continues in the existing checkout on build/jdk115-unix-platform to keep
  the established vendor/build paths. No additional worktree is needed for this
  sequential continuation; no remote is configured.

## Files and responsibility

- src/unix/java/include/threads_md.h and monitor_md.h: backend selection and
  recovered native records, without conflating pthread and cthreads layouts.
- src/unix/java/runtime/thread_md.c and mon_md.c: retain the Linux behavior and
  select the reconstructed implementation with the platform guard.
- Focused Unix condvar/mutex files: original Mach message-backed wait protocol.
- src/unix/java/runtime/gc_md.c, link_md.c and system_md.c: audit OS services
  after thread/monitor contracts are established.
- tests/native: layout, wait/interrupt ordering and native lifecycle probes.
- Existing CTest Java suites: observable VM semantics and support-library gates.

## Execution tasks

- [x] Start binary inventory and compare it to the existing Unix implementation.
  Verify recovered addresses, allocation sizes and field accesses in IDA;
  record results in 2026-11-06-jdk115-unix-platform-evidence.md.
- [ ] Recover remaining thread/monitor fields and Mach condition-message format.
  Obtain matching SDK declarations before choosing types. Add 32-bit layout
  tests and native contract cases for init/destroy, notify-one/broadcast,
  timeout and interrupted waits. Observe failing cases before implementation.
  Implement the recovered condvar path, clearly separating OS mocks from real
  platform validation. Keep Linux pthread selection as the default.
- [ ] Reconstruct thread bootstrap, allocation, create/exit, attachment, priority,
  enumeration and suspend/resume, including partial-failure cleanup. Verify
  queue ownership, Java back pointers and stack/register roots with native
  probes and existing thread/GC/JNI tests. Commit the verified slice.
- [ ] Reconstruct monitor recursion, ownership, entry/wait queues, timeout and
  interrupt reacquisition. Compare recovered control flow with the binary and
  run monitor, interruption, stop/close and contention regressions. Preserve
  established Java-visible bug fixes with explicit evidence. Commit the slice.
- [ ] Audit remaining Unix OS services against Rhapsody: memory reservation,
  commit/decommit, loader symbol resolution, clocks and process/file paths.
  Add only binary-supported differences and necessary host adaptations, each
  with its own failing regression or exact ABI/layout check before changes.
- [ ] Run Windows/Linux Release/Debug full suites and installed-image checks.
  Review the complete branch independently. Record SDK compilation and actual
  DR2 execution separately; never claim either from Linux-only results.

## Review focus

- A thread suspends while holding a VM queue lock: GC must not deadlock or lose roots.
- Timeout races with notify/interrupt: reacquire the monitor and preserve recursion.
- Failed thread or message-port allocation: no leaked queue member or OS resource.
- Attached native threads and thread exit: cthread TLS must not retain freed records.
- Backend selection: Linux must not include Mach headers or change its current ABI.

## Ledger

Phase 3 started with primary-binary inspection, not a claim that Linux already
passing closes the cthreads requirement. Initial recovered facts are committed
in the evidence document; runtime implementation is still open. No production
behavior changed in this initial inventory checkpoint.

Task 2, first implementation slice: Mach condition-message primitives are
implemented and independently reviewed. RED: protocol test could not link the
missing source. GREEN: strict C89/i386 test-double run, optimized external-SDK
header run, MSVC Release/Debug target checks, Windows Debug full107/107 and
Linux Release full95 passed plus one expected device skip. Production pthread
selection is unchanged. Task 2 remains open for complete records and VM
condvar wrappers; see the evidence document for protocol details and the
reserved-message-bits ruling.
