# GC allocation-state and VM suspension evidence

Primary: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Secondary: symbol-bearing Rhapsody DR2 `libjava.A.dylib`.

## Allocation reserves

| Primary address | Recovered operation |
| --- | --- |
| `1002cfc3` | InitializeAlloc: reject min+red+yellow > max, include reserves in initial commit, allocate reserve arrays, return success |
| `1006f3c0`, `1006f3c4` | red/yellow reserve byte counts; both default zero; Rhapsody names barriersize_red/yellow |
| `1006f3b8`, `1006f3bc` | reserve array tags 12 and 13, unsigned byte and unsigned short; yellow count is bytes >> 1 |
| `10028de3` | manageAllocFailure: GC, finalization, soft-ref clearing, expansion, then release yellow/red reserve |
| `10029675` | direct reserve body+handle release, update free-space counters, clear reserve handle |
| `100296f4` | tryLowerAllocState: after a reclaiming collection, recreate red first, then yellow; reserve attempts never recursively collect |
| `10083190`, `10082e40` | reclaim generation and saved generation; collection increments generation when objects were reclaimed |
| `10027c61` | allocation wrapper snapshots state, processes reserve recovery, publishes state notifications after releasing heap lock; one retry after state-rise notification on failed allocation |
| `1002984c` | VM.asChange(old,new), delivering real Java notification-queue events |
| `1002c9a9`, `1002c97b` | explicit GC processes recovery; getState triggers GC when not green |

States are 1=green, 2=yellow, 3=red. These are supported by real arrays rooted
in the collector, not inferred from arbitrary free-memory thresholds. A failed
allocation consumes yellow first if configured, then red. Rebuilding reserves
after reclamation moves back toward green. With both reserves disabled, the
original behavior remains green even when an ordinary allocation fails.

## VM suspension: verified native-thread no-ops

Shared code enumerates Java threads, consults `ThreadGroup.vmAllowSuspension`,
and maintains a real global flag (`1007f620`). A rise in allocation state sets
that flag. `unsuspendSomeThreads` visits non-suspendable groups without clearing
it; `unsuspendThreads` visits all threads and clears it. Returning to green does
not itself clear the flag.

The platform VM operations are demonstrably empty in **both** binaries:

- Win32 `1004cdaa` (VMSuspend) and `1004cdc7` (VMUnsuspend) contain a prologue,
  `xor eax,eax`, and epilogue/return, with no calls or memory writes.
- Win32 `1004cdbc` (VMSuspendMe) contains only prologue/epilogue/return.
- Rhapsody names confirm `sysThreadVMSuspend` at `5c145000` returns zero and
  `sysThreadVMSuspendMe` at `5c14500c` is empty. Its VMUnsuspend is likewise empty.

These implementations intentionally do not invoke ordinary `Thread.suspend`
or the GC stop mechanism. This is recovered native-thread behavior, not a
placeholder implementation of those distinct operations.

## Explicit deviations preserving this port's GC fixes

- This slice retains the existing collector rather than replacing its whole
  object/header/cache representation. Reserves use the original array tags and
  storage sizes within the existing allocator.
- The reclaim-generation counter advances for each freed object rather than
  once per collection that freed objects. Only equality with a saved generation
  is observed by reserve recovery, preserving that condition.
- Existing synchronous-finalizer suppression during allocator failure remains:
  invoking arbitrary finalizers there would expose the older interpreter's
  cached operand top. The normal finalizer thread still processes its queue.
- For the required synchronous VM.asChange notification, temporarily publish the
  current Java frame's full operand-stack extent before creating the callback
  frame, then restore the cached top. This protects live operands from both GC
  omission and callback-frame overwrite.
- Win32 `threadUnsuspendSuspendables` (`1004dd22`) and its Rhapsody counterpart
  (`5c130c54`) enter the queue monitor again at the end instead of releasing it.
  The reconstruction balances the lock to preserve queue safety; it does not
  reproduce that lock-recursion leak.

## Regression oracle

`VMReserve115`, with `-ms1m -mx4m -mr256k -my256k`, retains arrays until the
reserves are consumed, then releases them and collects. Package helper
`sun.misc.VMState115` exposes the package-private native suspension flag.

Original output, captured with `tools/capture-expected.cmake`:

```
initial=1
suspended=false
yellow
red
recovered=1
suspended=true
some=true
all=false
```

The fixture calls System.exit(0) because loading Sun's VM class creates two
non-daemon notification threads that survive main's return. The initial
reconstruction run failed on the unsupported `-mr256k` option, before any
implementation edits. Runtime verification is recorded after the coordinated
builds complete.

## Bounded marking recovery

Linux Debug `stress` failed with SIGSEGV inside `markChildren`, with 1,573
recursive marker frames beneath `scanHandles` (`out/gdb-stress.log`). The
reconstructed invocation defaults exposed the old unbounded marker on Sun's
128 KiB native thread stack; increasing the stack would hide the missing
collector behavior.

Primary `1002A7FF` caps recursive depth at 30. `1002ABF1` queues deferred handles;
`1002A7AB` removes them; `1002A77C` drains them at depth zero. `1002CEF7`
initializes exactly 100 linked cells. Queue exhaustion sets `10083178`, and
`1002A622` repeats the complete handle scan until that flag remains clear.
Rhapsody `5C13DEB8`, `5C13EB30`, and `5C13ED4C` corroborate the depth cap,
worklist draining, and rescan following finalization marking.

The reconstruction retains the existing class representation and marking
strength semantics, but adopts this bounded traversal. A fixed 100-element
array LIFO represents the primary linked-cell freelist; both perform identical
push/pop ordering without allocation during collection. This representation
choice is a deviation, not a claim of source identity. The native stack default
remains 128 KiB.

## Conservative-root fixture scope

The initial fixture allocated its pressure graph in the long-lived main
interpreter invocation. It reached red correctly but remained red after its
Java static field was cleared. The same allocations in a worker that is joined
before clearing the static produce `recovered=1` on both original Sun and the
reconstruction. This isolates the allocation-state test from conservative
native stack/register retention across different C compilers. It does not
establish which stale native or operand slot retained the initial graph.

The adopted fixture keeps all eight original expected output lines unchanged;
the original main-pressure source remains in `out/VMReserve115-original.java`
as diagnostic evidence. No interpreter local-clearing workaround was added.
A Linux Debug GDB run of the worker fixture exits normally and matches the
oracle (`out/gdb-reserve.log`).

Verification: Linux Debug `stress` changes from the recorded SIGSEGV to PASS
in 30.54 seconds with the 128 KiB default unchanged. Linux Debug
`vm-VMReserve115` passes in 0.16 seconds. Logs: `out/marker-stress-linux.log`
and `out/gdb-reserve.log`. Full paired configuration gates are recorded below
as they complete.

Linux Release rebuilt from the bounded-marker source and passed all 63 CTest
cases with no exclusions (49.31 seconds), including stress, reserve recovery,
JNI and the expanded self-hosted javac suite.
Logs: `out/marker-release-build.log`, `out/marker-release-tests.log`.
