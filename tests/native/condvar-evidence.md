# Win32 condition variable reconstruction

Primary evidence: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Examined through IDA pseudocode and disassembly, without modifying names or types.
The missing header and implementation are reconstructed source, not original Sun
files. Function and field names below express their observed roles; original
spelling and exact source-level return types cannot be recovered from machine code.

| VA | Reconstructed function | Evidence |
| --- | --- | --- |
| `10014da0` | `condvarInit` | Creates unnamed manual-reset, initially unsignaled event; returns 0 on failure, writes only offset 0 and returns 1 on success. |
| `10014de2` | `condvarDestroy` | Returns `CloseHandle` result for offset 0. |
| `10014df9` | `condvarWait` | Arguments: thread, condition variable, critical section. Waits on event and thread offset 24 interrupt event, releasing/reacquiring mutex. |
| `10014ed3` | `condvarTimedWait` | Same three arguments plus DWORD timeout; remaining timeout measured with `timeGetTime`. |
| `10015010` | `condvarSignal` | Unsigned waiters > signals check; sets event before incrementing signals and generation; 0 on SetEvent failure, otherwise 1. |
| `1001505e` | `condvarBroadcast` | Nonzero waiter check; sets event, copies waiters to signals, increments generation; boolean success. |

The x86 layout is HANDLE at 0, generation at 4, unsigned waiter count at 8,
unsigned outstanding signal count at 12. The monitor initializer at `10038410`
passes monitor+24 to init; owner and recursion count are at 40 and 44. This
confirms a 16-byte condition variable following the 24-byte CRITICAL_SECTION.
The shipped monitor header independently confirms the member order.

Both wait functions snapshot generation and increment waiters before unlocking.
A retry sleeps 1 ms after unlocking. After reacquiring the mutex, an interrupt
wait result (1) decrements waiters and returns without clearing the interrupt
event or consuming a signal. Otherwise a signal is consumed only when the count
is nonzero and generation changed; the last consumed signal resets the event.
Timeout also only decrements waiters. Wait results are not otherwise checked.
The monitor wait caller at `10038859` ignores both wait function return registers
and checks thread interruption itself; these reconstructed APIs therefore return
void, rather than exposing incidental register values as invented status codes.

Timed wait preserves the original unsigned clock-wrap expression:
`previous + 0xffffffff - now` when `now < previous`. Disassembly at the wrapped
branch loads `0xffffffff`, subtracts now, then adds previous. This unusual
arithmetic is deliberately not corrected to modular subtraction.

Initialization does not reset the three counters. Storage must be zeroed before
first initialization, as in the existing monitor allocator. Wait requires the
critical section held exactly once; Java recursion is maintained by the monitor
layer. Signal and broadcast require that same lock. No extra error handling or
FIFO scheduling has been added.

## Integration and verification

Compile `src/win32/java/runtime/condvar_md.c` into the Windows runtime and link
`winmm.lib`. It needs the Sun-compatible `threads_md.h` interrupt_event field and
`mutex_md.h` CRITICAL_SECTION typedef. Header declarations use a forward struct
declaration so the condition-variable layout does not depend on thread internals.

`condvar-test.c` must be built for x86: it asserts the recovered field offsets and
size, exercises timeout, one-of-two signal, multiwaiter broadcast, saturated
signals, event reset after the last waiter, and both interrupt wait paths.
It first failed compilation because the missing condition-variable header had
not yet been implemented. After implementation it compiled with MSVC /TC /W3 and
passed on Windows using the shipped thread/mutex headers. A second build with
source include directories first also passed against the migrated thread header
(with the vendor directory supplying the as-yet uncopied mutex header).
The real Windows tests
do not exercise the 49-day clock wrap or inject failing Win32 calls.

Standalone command after running `vcvars32.bat` (place output in a temporary
directory or the build tree):

```bat
cl /nologo /W3 /TC /Ivendor/jdk1.1.5/include/win32 /Ivendor/jdk1.1.5/include /Isrc/win32/java/include tests/native/condvar-test.c src/win32/java/runtime/condvar_md.c /link winmm.lib
condvar-test.exe
```

The vendor-first include order permits standalone verification before the parent
thread-layout migration; the integrated build should use its normal source
include directories once that migration is complete.
