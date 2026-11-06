# Win32 monitor reconstruction

Primary: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Inspected the functions below with IDA, using the already analyzed database.
`monitor_md.h` is copied verbatim from the shipped Windows header. `mon_md.c`
is a reconstruction, not original Sun source.

| VA | Function | Recovered behavior |
| --- | --- | --- |
| `10038410` | sysMonitorInit | Clears owner/count, initializes critical section and condvar; deletes critical section on condvar failure. |
| `1003848e` | sysMonitorDestroy | Deletes critical section, closes condvar, returns 0. |
| `100384dc` | sysMonitorEnter | Marks thread MONITOR_WAIT while entering critical section, then RUNNABLE. Recursive acquisition releases the extra critical-section acquisition and increments Java recursion. |
| `10038601` | sysMonitorEntered | True before ThreadsInitialized; otherwise requires current thread to own monitor. |
| `10038671` | sysMonitorExit | Validates owner; releases critical section only when recursion reaches zero. |
| `1003876b` | sysMonitorNotify | Requires Entered and maps condvarSignal boolean to 0/-1. |
| `100387e2` | sysMonitorNotifyAll | Same with condvarBroadcast. |
| `10038859` | sysMonitorWait | Saves recursion, validates ownership, checks interruption, releases ownership during condvar wait, restores recursion and owner, then checks interruption again. |
| `100389bf` | sysMonitorDumpInfo | Reports owner, recursion, waiters, outstanding signal tickets. |
| `10038ad1` | sysMonitorSizeof | Returns 48. |
| `10038ae6` | sysMonitorInUse | Owner, waiter count, or ticket count is nonzero. |

Layout is a 24-byte x86 CRITICAL_SECTION, 16-byte condvar, thread owner at
offset 40, and long recursion count at 44. This matches the vendor header.
The condition-variable counters require zero-filled initial storage: init
does not reset them. Inspected the current shared allocator: cached monitor
blocks, registry, cache lock, queue lock, class lock, heap/finalizer locks and
interpreter locks are zero-filled before registration. Registry initialization
calls monitorRegister, which initializes the lock before entering it.

Enter/exit return success without locking when no current thread exists before
ThreadsInitialized; they return SYS_ERR for unattached threads after initialization.
Wait returns SYS_ERR for no thread or wrong owner and SYS_INTRPT (-2) for an
interrupted thread. Timeout returns SYS_OK. Wait holds the Win32 critical section
once regardless of Java recursion, allowing the notifier to acquire it.

The third wait parameter is passed to sysThreadIsInterrupted at `1004d426`.
That routine always reads and returns the interrupted shadow flag; parameter
value 1 additionally clears the flag and resets the interrupt event. Thus the
parameter governs clearing, not whether interruption is observed.

## Explicit interim deviations

- `sysMonitorCleanup(mon, owner)` preserves this port's prior no-op thread-exit
  cleanup under a separate name. It must not be confused with the recovered
  one-argument resource destructor. Shared monitor cleanup remains old code.
- DumpInfo retains numeric Windows thread identity rather than dereferencing
  the Java thread cookie to obtain Sun's Java thread name.
- Assertions use the port's existing sysAssert macro. Debug diagnostics do not
  reproduce Sun's original source line numbers and DumpThreads/panic sequence.

## Native verification

`monitor-test.c` uses real Windows critical sections, events and worker threads.
Only thread identity and interrupt-flag inspection are supplied by the harness.
It checks the 48-byte layout, bootstrap semantics, unattached-thread errors,
ownership errors, recursive timed wait and recursive notification, interruption
before/during wait, clear versus preserve, and InUse's three independent fields.
The worker must acquire the monitor while the main thread has Java recursion
depth two, catching failure to release the underlying critical section fully.
It is an x86 test and requires `condvar_md.c` and `winmm.lib`.

MSVC x86 `/TC /W3` compilation and execution passed using the migrated source
headers. Command after `vcvars32.bat` (output directed to a temporary directory
during verification):

```bat
cl /nologo /W3 /TC /Isrc/win32/java/include /Isrc/share/java/include tests/native/monitor-test.c src/win32/java/runtime/mon_md.c src/win32/java/runtime/condvar_md.c /link winmm.lib
monitor-test.exe
```
