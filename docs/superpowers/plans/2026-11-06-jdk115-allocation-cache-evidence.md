# Per-thread allocation cache and JNI storage

Read-only Task 3 preparation, 2026-11-06. Primary: Win32 javai.dll and
javai_g.dll; original alloc_cache.h, interpreter.h, jni.h. Secondary:
Rhapsody DR2 libjava.A.dylib. No production changes.

## Activation is real on Win32

| Setting | Debug address/value | Release address/value | Rhapsody symbol/address/value |
|---|---|---|---|
| allocCacheSize | 1006F3D0 / 1024 | 1004B5F0 / 1024 | _allocCacheSize 5C151870 / 1024 |
| allocLocalSize | 1006F3D4 / 256 | 1004B5F4 / 256 | _allocLocalSize 5C151874 / 256 |
| allocHandleCount | 1006F3D8 / 42 | 1004B5F8 / 42 | _allocHandleCount 5C151878 / 42 |

These values were read from the actual binary data, not inferred from header
macros. References to the Debug setting words found only allocator reads;
no platform-startup write disabling the cache was found. Both actual object
allocators, Debug10027C61 and Release1001A5A0, invoke the cache when EE is
non-null and the **rounded total block size** `(requested+11)&~7` is strictly
less than allocLocalSize. Thus the threshold is not simply body size <=256.
Rhapsody allocator5C13C8E4 inlines the same fast path at5C13C928 onward.

Shipped struct alloc_cache is16 bytes: byte busy at0, three padding bytes,
long size at4, void* tail at8, void* handles at12. It resides at EE+60;
therefore the fields are EE+60/+64/+68/+72. InitializeExecEnv10017345 clears
all76 EE bytes, so each cache starts empty. No separately allocated cache
descriptor is required.

## Actual allocator algorithm

Debug10027FC3 / Release1001A7E0:

1. If busy is already set, return NULL and use ordinary allocation. Otherwise
   set busy, then execute Win32's locked XOR memory flush.
2. Require remaining cache_size strictly greater than rounded request. If
   insufficient, refill via10028219 / Release1001A8B0. Refill recursively calls
   the ordinary allocator with methods/tag40 (0x28), body size1024. Busy blocks
   recursive cache entry. The returned managed handle becomes cache_tail;
   cache_size is read from its allocation header. Failure returns -1.
3. Obtain an empty handle from cache_handles. If empty,100282CF /
   Release1001A8E0 takes up to42 handles from the global free-handle list under
   heap lock, links them using the handle methods word, and subtracts8 per
   handle from global free-handle bytes. It may obtain fewer or none.
4. Return the **old tail handle** as the newly allocated object. Its body stays
   at the old tail start; replace its methods word with the caller's methodtable
   or array tag. The popped empty handle becomes the **new tail handle**:
   body=start+rounded request, methods=40. Split the old block's size header
   into allocated and remainder headers; publish new tail and remaining size.
5. Clear busy on every normal exit. The cache region was zeroed at refill by
   ordinary allocation; the fast path does not memset each returned body.

This is not a raw malloc bump pointer. The tail is a GC handle to a tagged
managed block; preserving its tag, block headers, handle accounting and GC
interaction is essential. Strict `remaining > request` leaves a nonempty tail.

## GC and thread cleanup

Primary thread-root callback10029E43, invoked through10029E10, visits the EE:

- Busy cache: hard-mark the valid tail handle atEE+68. Leave cache state and
  private free handles intact; the in-flight allocator's native stack is also
  scanned. This protects allocation/refill interrupted by collection.
- Idle cache: clear tail and size; return each private empty handle to the
  global list, adding8 free bytes per handle; clear cache_handles. The abandoned
  tail region becomes normal collectible managed memory. GC does not root an
  idle tail indefinitely.

Standalone cleanup code exists at Debug100278F0 (verified disassembly; IDA
had not made it a function). It locks the heap, asserts !busy, clears tail/size,
and returns cached handles identically. No direct xrefs to it were found.
**Platform difference:** Win32 DeleteExecEnv10017453 and Release1000F310 do
not call this cleanup; they recycle stacks and delete JNI roots. Rhapsody
DeleteExecEnv5C114440 explicitly calls named allocCacheCleanup5C13C7E0 on
EE+60 before stack/JNI cleanup. Do not claim that Win32 performs this exit
cleanup merely because the function and header declaration exist. Adding it
for thread-lifetime resource correctness would need an explicit deviation
or the Rhapsody platform mapping, not fabricated primary evidence.

## JNI reserved fields: exact overlay, not an owner pointer

Original interpreter.h defines JNIEnv2EE by subtracting
offsetof(ExecEnv,nativeInterface), captured as20. FindClass1001A1CA and callback
dispatcher1001AE21 confirm env-20. No owner pointer is needed.

JNIEnv_ begins functions at0, reserved0 at4, reserved1[6] at8. The shipped
JNIEnvGetLocalRefs macro casts **reserved1** to JNIRefTable, whose four words
are elements/base/top/size. Therefore actual local storage is:

| Field | env offset | EE offset |
|---|---:|---:|
| functions | 0 | 20 |
| reserved0 (no established use) | 4 | 24 |
| local elements pointer | 8 | 28 |
| local base | 12 | 32 |
| local top | 16 | 36 |
| local capacity | 20 | 40 |
| remaining reserved1 words | 24,28 | 44,48 |

FindClass1001A1CA calls AddRefCell10019D94 with env+8. Native invocation
100131C0 saves/restores EE+32/+36 around a native frame. AddRefCell allocates
four8-byte cells initially and doubles capacity. InitializeJNIRootFrame10019D13
sets EE+28=NULL and EE+20=the initial JNI function table; DeleteJNIRootFrame
10019D45 requires base0 then frees EE+28. InitializeExecEnv's memset supplies
zeros for reserved0 and the remaining reserved1 words; no nonzero purpose for
those three words was established in this audit. In particular env+4 is **not**
the reference-table descriptor. Do not confuse it with JavaVM_.reserved0,
which the original JavaVMGetEE macro explicitly uses for the main EE.

## Migration implications

Adopt the actual inline cache and JNI overlays with the shipped EE layout.
Implement cache allocation, busy publication/fences, GC idle cleanup and
handle accounting together; zero-filled unused fields or disabling the
threshold would not reproduce Sun's active behavior. Preserve the existing
reserve-policy handling on slow/refill allocation. Test small allocations,
boundary sizes, collection during refill, cache draining and thread exit.
For JNI, use env-20 and the inline16-byte local-table descriptor; any external
side state for transitional invocation bookkeeping must be explicitly marked
as such and must not replace the original descriptor or masquerade as a
recovered reserved0 meaning. This document establishes behavior, not a claim
that the present collector already supports these cache invariants.

## Implemented cache regression and oracle

`tests/native/alloc-cache115.c` was compiled against **vendor headers only**
with MSVC x86 /MD /DDEBUG /DJCOV /DSUN_ORACLE. It loaded the original
`javai_g.dll`, called exported ArrayAlloc and JNI entrypoints, and used the
recovered private cleanup RVA278f0 and gc RVA2c9a9. Fresh result:
`allocation-cache=ok`, exit0.

The checks exercise the 1024-byte refill, exact 256-byte rounded-block boundary
(252-byte byte-array body bypasses, 244-byte body takes cache), busy reentry
bypass, busy-tail survival through GC, idle cache draining, and 80 JNI-rooted
arrays retaining zero initialization and written contents across repeated
refills and collection. JNI local refs intentionally root the arrays; raw C
handle arrays in an invocation host are not an API root contract.

The implementation uses the original EE inline cache, publication through
sysMemoryFlush, managed tail tag40, and global/private handle lists. Collector
thread scanning drains idle caches and marks busy tails. Handle diagnostics
count the global free list because reserved private handles have NULL bodies
but are not globally free. Windows thread-exit cleanup is an explicit deviation
supported by the Rhapsody path, preventing lost private handles on EE disposal.
Native CMake target `alloc-cache115` runs these same checks against the port;
its runtime integration gate is pending the coordinated class/EE migration.

After coordinated migration, Linux Release passed the cache, both managed-class
lifetime modes, JNI invocation and stress tests. The cache regression was then
extended with 30 native host attach/allocate/detach cycles concurrent with main
thread allocation; the original Sun Debug DLL passes this extension too.

The port's host-attach adapter allocates a Java Thread before publishing its
cookie, unlike primary `1002755f`, which consumes a serialized preallocated
Thread/EE prepared by `10027225`. Until that adapter is reconstructed fully,
the allocator bypasses local caching when a registered native thread has no
Java cookie, rather than borrowing DefaultExecEnv's cache concurrently. The
collector scans such a provisional thread's native stack/registers without
assuming a Java object or EE already exists. This is an explicit transitional
attachment deviation; normal bound EEs retain the active 1024/256/42 cache.
