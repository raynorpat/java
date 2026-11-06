# Intrusive finalizer layout recovery

Primary: `vendor/jdk1.1.5/bin/javai_g.dll`; shipped `include/finalize.h`.
The collector and finalizer implementation now use this representation.
Integrated runtime gates are recorded below as they become available.

`ObjAlloc` at `100286f0` adds exactly `sizeof(finalizer_t)` (four bytes) to
the requested instance body when `cbFinalizer(cb)` is non-null. After normal
managed allocation it prepends the handle to HasFinalizerQ. The next handle
is stored at `handle->obj + cbInstanceSize(obj_classblock(handle))`, matching
the shipped FINALIZER_T macro. No independent native node is allocated.

`prepareFinalization` at `1002b34b` removes unreachable objects from the live
queue and reverses the newly selected batch while appending it behind any
already pending batch. FinalizeMeQLast tracks the tail. All pending objects
are traced in a separate pass; BeingFinalized is traced independently even
when HasFinalizerQ is empty. Both object and class metadata tracing retain
the bounded marker worklist and rescan fallback.

`runFinalization` at `10016a10` pops the pending head and clears the tail if
the queue becomes empty. It retains the current owner/reentrancy behavior,
runs the Java finalizer outside the queue lock, clears BeingFinalized, and
notifies waiters. There is no native node to free afterward.

`finalizeOnExit` at `10016ccb` first collects and runs pending finalizers,
then moves the remaining live queue in reversed batches to the pending tail,
releasing locks while executing Java callbacks. The existing port's exit
status policy and reentrant-finalization protection remain separate from
this representation migration.

The change must update gc.c, finalize.c and finalize.h together: allocation
size, live/pending queue links, tail maintenance, roots, debug dumps and node
reclamation. A copied one-word header with the existing two-word native nodes
would be invalid. Preserve the existing synchronous-in-allocator finalizer
suppression until interpreter callback safety is separately established.

The original Win32 GC platform header is present at
`vendor/jdk1.1.5/include/win32/gc_md.h` (ignored vendor paths require
`rg --files --no-ignore`). Its contiguous-heap configuration requires no
platform allocation macros. gc.c can consume the original gc.h and remove
duplicate heap-lock, block-header, marking and overflow macros; Unix needs an
explicit contiguous-heap gc_md.h mapping. This header-only integration should
be verified before changing finalizer representation.

## Implementation and focused verification

`finalize.h` and Win32 `gc_md.h` are copied byte-for-byte from the shipped
headers. `gc.c` consumes the original `gc.h`; its duplicate lock, block,
marking and overflow definitions were removed. The Unix platform header
explicitly selects the existing contiguous mapped-heap implementation and
is a port mapping, not a recovered Sun Unix file. The original unused paged
heap macros are not evidence of a new paged-heap implementation.

The managed allocation cache includes the extra finalizer word in its
rounded allocation size. Queue traversal recomputes FINALIZER_T from the
stable handle, so object relocation does not leave native node pointers.
The bounded marker, conservative stack/register scan, class/cache roots,
exit-status policy and allocator callback suppression remain unchanged.
Monitor diagnostics use the migrated cache's nonzero key to identify an
assigned entry, as agreed with the cache implementer. No lock order changed:
heap, live-finalizer queue, pending-finalizer queue, thread queue, stop-world.

The class-layout115 native ABI test now requires sizeof(finalizer_t)==4 and
next at offset zero. Compiling it against the prior committed finalize.h
fails with MSVC C2118 (negative subscript); the same probe against shipped
Sun headers compiles and runs with exit zero. Isolated MSVC x86 Debug and
GCC x86 Debug compilation of gc.c/finalize.c passes after migration.

FinalizeQueue115 is compiled with the original javac. Three retained objects
are finalized on exit; the first drops the shared root. Every callback runs
GC and recursively requests finalization, then verifies its payload through
printed values. The first callback also creates a fourth object, requiring
a later live-queue batch. Fresh primary Release and Debug runs both return
zero and print exactly main, final=1:1, final=2:2, final=3:3, final=4:4.
This checks observable queue order and callback GC safety. It does not claim
that no unrelated VM object remains in HasFinalizerQ: unconditional pending
and BeingFinalized root scanning is established separately by primary
1002b34b. No artificial mutation of VM global queues is used by this fixture.
