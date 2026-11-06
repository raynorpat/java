# Phase 3 initial Unix binary evidence

Primary input: vendor/rhapsody-dr2/System/Library/Frameworks/JavaVM.framework/
Libraries/libjava.A.dylib, inspected with IDA on 2026-11-06.

SHA-256: 49157508f1d66fbf8ed42bfffd33ce52b9b389522ec1a47a79f733e82275bc76.
The addresses below refer to this image, not Windows javai.dll.

## Existing implementation versus required backend

Current Unix thread_md.c/threads_md.h and mon_md.c/monitor_md.h unconditionally
use pthreads. They implement signal-based stop/resume, GC stack scanning,
interrupt wakeups, attachment and monitor wait queues. This is the working
Linux host adaptation, not an implementation of Mach/cthreads.

The staged Rhapsody framework supplies the original binary and symbols, but
the initial no-ignore file inventory found no cthreads.h, mach.h or
thread_status.h in vendor/rhapsody-dr2. SDK declarations still need to be
located before compiling a real Mach backend. This does not block additional
binary recovery. Running the rebuilt VM on DR2 remains a later milestone.

## Recovered entry points

| Function | Address |
|---|---|
| sysMonitorSizeof / Init / Destroy | 5C136060 / 5C13606C / 5C1360C4 |
| sysMonitorEnter / Entered / Exit | 5C1360E8 / 5C1361A0 / 5C1361D8 |
| sysMonitorNotify / NotifyAll / Wait | 5C136244 / 5C13629C / 5C1362F4 |
| sysMonitorDumpInfo / InUse | 5C1364AC / 5C1365EC |
| condvarInit / Destroy | 5C13660C / 5C13662C |
| condvarWait / TimedWait | 5C136654 / 5C13668C |
| condvarSignal / Broadcast | 5C1366C4 / 5C1366D8 |
| sysThreadStackPointer / Registers | 5C14487C / 5C1448C0 |
| sysThreadStackBase / SetStackBase / GetBackPtr | 5C1448D0 / 5C1448E0 / 5C1448F0 |
| sysThreadAlloc / Bootstrap | 5C144970 / 5C144A38 |
| sysThreadInitializeSystemThreads / Create | 5C144B64 / 5C144BB4 |
| sysThreadInit / Free / Exit | 5C144CFC / 5C144D0C / 5C144E24 |
| sysThreadYield / GetPriority / SetPriority | 5C144F90 / 5C144F9C / 5C144FDC |
| sysThreadVMSuspend / VMSuspendMe / VMUnsuspend | 5C145000 / 5C14500C / 5C145014 |
| sysThreadSuspend / Resume / Self | 5C145020 / 5C145160 / 5C1451FC |
| sysThreadEnumerateOver_unlocked / EnumerateOver | 5C145210 / 5C14524C |
| sysThreadPostException / Interrupt / IsInterrupted | 5C14536C / 5C145410 / 5C145424 |
| sysThreadSingle / Multi | 5C145440 / 5C1454B0 |
| sysInitializeLinker / DynamicLink | 5C13567C / 5C135758 |
| sysMapMem / UnmapMem / CommitMem | 5C1357C0 / 5C13580C / 5C135860 |
| sysGetMilliTicks / TimeMillis | 5C135BD0 / 5C135C28 |

## Verified layouts and behavior

sysThreadAlloc allocates and clears 0xC0 (192) bytes. Confirmed offsets are:
next +0, Java cookie +4, cthread pointer +12, stack base +16, execution state
+20, system-thread flag +28, interruption shadow +40, saved monitor recursion
+148 and current monitor +152. These are partial recovered fields, not a
complete proposed C structure. The cthread private-data slot is +116: Self
loads ur_cthread_self()->field_at_4, then that cthread's +116 slot. Allocation
publishes the record there, then links it and updates active/user counts while
holding _queue_lock. Do not substitute pthread_t layout for these accesses.

sysMonitorSizeof returns 68. Init initializes condition objects at +12 and +28,
recursion +8, entry-queue head +44/count16 +48, wait-queue head +52/count16 +56,
and owner +60. Enter queues a three-pointer stack node, records thread +152,
waits on the entry condition, unlinks the node and increments recursion.
Wait saves recursion in thread +148, releases ownership, waits, rejoins the
entry queue and restores ownership/recursion before returning. InUse tests
owner or wait-queue count. Padding and remaining fields need further recovery.

Condition waits are not merely calls to the standard cthreads condition API:
the 388-byte helper at 5C1366EC calls msg_receive, spin_lock and mutex locking;
signal helper 5C136870 and broadcast helper 5C1369D4 call msg_send. The wrappers
set the current thread's +20 state to the supplied wait state, call the helper,
then restore state 1. Infinite wait passes -1. Init clears the first three
words; Destroy broadcasts when its queue is nonempty and calls spin_lock.
Message contents, timeout conversion and queue lifetime require the next pass.

sysThreadStackPointer uses thread_get_state for another thread with count16,
flavor -1 and returns state word7; for the current thread it returns sentinel1.
Registers calls Self and returns zero. Single/Multi enumerate under _queue_lock
using separate suspend/resume helpers; their error/unwind behavior remains open.

One compatibility issue is explicitly unresolved: this Rhapsody image's
IsInterrupted clears +40 when clear==1 and then reads it, unlike the retained
Linux/Sun behavior that returns the pre-clear value. Do not silently propagate
that difference into Java Thread.interrupted(); resolve it against existing
interrupt tests and record the implementation decision in the next slice.

## Verification boundary

The entry-point inventory and the bodies described above were inspected in
IDA, not inferred from exported names alone. No Mach backend has been compiled
or executed in this checkpoint. Phase 2 pre/post-merge Windows Debug runs pass
106/106; the new branch preserves the identical production tree. A fresh Linux
Release baseline also passes 94 tests with the one expected audio-device skip
and zero failures (out/phase3-linux-baseline-tests.log).

## Mach condition-message primitives implemented

The next inventory pass found usable external declarations in
D:/RhapsodiOS/tmp/terminal-sdk/include/System/mach/cthreads.h and message.h.
Their mutex (8 bytes), condition (16 bytes), queue and legacy msg_header_t
(24 bytes) layouts agree with the original i386 accesses. The private cproc
prefix is corroborated by D:/RhapsodiOS/src/CoreOS/Libc-1/threads.subproj/
cthread_internals.h: next +0, incarnation +4, state +8, reply_port +12,
wait_port +16. Only this accessed prefix is declared by the reconstruction.
These external headers are not represented as an original DR2 SDK install.

External public header SHA-256 values:

- cthreads.h: 766dd71bc17d0d695d58a5fc8e752a0258c7c55207deeffe448103a595f515f5
- message.h: 6532113686317f639d97c920833c0336a1774cc7f38d8830e443ea43ccd2986c

src/unix/java/runtime/condition_md.c reconstructs condition_wait_timeout,
condition_signal_timeout and condition_broadcast_timeout. SDK declarations
also recover their names. The original helpers are authoritative: unlike the
later Libc implementation, the VM returns 0 on message, -3 on timeout, -1 on
other receive error, and supports unsigned -1 for infinite waits. No external
Libc implementation or SDK files are copied into this repository.

Recovered protocol:

- Enqueue the current cproc, mark state2, release condition spinlock and mutex.
- Receive a 24-byte message on wait_port. Timed mode uses RCV_TIMEOUT (256);
  infinite mode uses option0/timeout0. The exact special retry value is -21,
  not RCV_INTERRUPTED (-207).
- A timeout (-203) checks state3 both before and after locking. A notifier
  already owning the waiter requires another receive, not timeout return.
- A genuine timeout rebuilds the queue without that waiter, retaining the
  remaining FIFO order. Both success and timeout restore state0. Every return
  reacquires the caller's mutex, including the blocking-lock fallback.
- Notify removes one head, marks state3, unlocks and sends a simple message
  with SEND_TIMEOUT/timeout0. SEND_TIMED_OUT (-103) is tolerated.
- Broadcast recursively dequeues/marks every waiter before unlocking, then
  sends in reverse order. This preserves the original node-lifetime protocol.

Ruling: initialize the outgoing header's unused 24 bits to zero instead of
leaving original stack contents there. They have no defined IPC meaning;
zeroing them avoids exposing uninitialized bytes. Cost if wrong: an undocumented
kernel dependence on reserved bits would differ; no inspected code uses them.

The original fatal-receive path logs an error and reacquires the mutex without
removing the waiter or resetting its state. That behavior is retained in this
primitive; no assertion of successful recovery from a broken Mach receive port
is made. Thread/port teardown must be addressed in the lifecycle slice.

Verification: the new test first failed because condition_md.c was absent.
It then passed with GCC -m32 -std=c89 -pedantic -Wall -Wextra using test-only
Mach headers. It also passes at -O2 using the external SDK's real public
headers and fake IPC functions; GNU89 is needed for asm in those external
headers, while our source itself remains C89. This is header/protocol checking,
not Mach linking or execution on DR2.

The CTest mach115-condition target passes MSVC Release/Debug and Linux Release.
Full Windows Debug: 107/107 pass. Full Linux Release: 95 pass, one expected
audio-device skip, zero failures. Logs: out/phase3-condition-{win,linux}-tests.log.
An independent review compared all four original helper bodies, layouts and
test cases and reported no actionable findings.

The VM continues to use pthreads on Linux. This file is currently built only
by the protocol test; condvar init/destroy and VM state wrappers, full thread
records and backend selection remain open. Tests cover deterministic race
interleavings, not real Mach scheduling or concurrency.
