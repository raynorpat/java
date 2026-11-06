# Monitor/finalizer header dependencies and launcher audit

Primary source: javai_g.dll SHA256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.

## Bounded adopted behavior

registeredEnumerate_unlocked at 0x10036FF4 iterates MonitorRegistry without
acquiring its lock; 0x10037030 is the locked wrapper. monitorEnumerate_unlocked
at 0x100380F5 asserts cache ownership, visits buckets in descending order and
saves each next pointer before calling the callback. 0x10038199 wraps it with
cache locking. Current implementations now expose this split without changing
the existing caller lock scopes. The port's cache does not tag pointer bits;
masking the low bit belongs to the later cache-reclamation migration.

monitor.h adopts original declarations and global-lock macros, but explicitly
retains the old monitor_t flags/uninit_count fields and collector polling macro.
No definition was invented for the original global-lock declarations which have
no consumers yet. monitor_cache.h retains its legacy lock contract until the
cache implementation changes; the unlocked enumeration declaration was added.

finalize.h adopts the original public function/global names but marks the
retained allocated finalizer nodes. finalizeOnExit (0x10016CCB) names the existing
recovered body; bool finalize_on_exit names the Java-controlled flag. The port's
sysExit callback still gates by flag and status==0; that policy did not move into
the canonical body. runFinalization's original declaration is now in the header.

The panic declaration in debug.h is original again. DLL export is already
controlled by javai.def. The guarded jni_io include remains a real compatibility
deviation: removing it would bypass invocation diagnostic callbacks in many
legacy callers. Restoring debug.h exactly requires migrating those callers to
jio_* or explicitly including the routing adapter in the actual callers.

Isolated Windows x86 MSVC and Linux x86 C89 compilation of monitor.c,
monitor_cache.c and finalize.c passes. Full integration/runtime gates remain
pending the concurrent class/collector work; no stale binary run is offered as
verification of these edits.

## Bounded next migration design

- Monitor cache: current mid offset12 vs original8, and current immediate reuse
  based on flags/uninit_count vs original sysMonitorInUse and local-cache
  reference tracing. Migrate monitor.c, monitor_cache.c, platform cache locks,
  thread cacheKey/monitorCache consumers and reclamation together. Primary
  findMonitor 0x10037EB3, lookup 0x10037D41, reclaim 0x10037588, init0x10037BDD.
  Do not implement findMonitor as a wrapper over legacy createMonitor: its entry
  count semantics differ. First gate structure offsets, then ownership/wait/
  notify and monitor churn while GC scans threads, on both platforms.
- Finalizers: original finalizer_t contains only a next JHandle stored after
  object instance bytes; queues are JHandle pointers and have a tail pointer.
  Current nodes contain next-node+handle and are independently allocated.
  gc.c allocation, mark/sweep queue transfer, object sizing, finalizer thread
  dequeue and exit draining must migrate together. Preserve FIFO behavior and
  finalizer exception clearing; gate resurrection, nested runFinalization and
  exit ordering. Header-only adoption cannot establish this ABI.
- Global lock declarations now match original, but actual current locking is
  still spread among legacy subsystems. Original declarations alone must not be
  counted as implementation of those locks.

## Launcher options (audit only, no launcher edits)

java_main0x100012D5 uses exact strings `-prof=jcov` at0x10068188 and
`-prof=jcov:` at0x100681AC. `-prof:jcov` is ordinary profiling to filename jcov,
not code coverage, in both original and current parser.

For equals forms, primary sets cov_file to java.jcov or the suffix, disables
class GC and registers exit callback0x100021D4. Callback invokes
jcov_write_data0x1003A02F, retrying return1 up to five attempts then reporting
failure. Current parser rejects equals forms. The current tree only has public
jcov declarations and Debug class metadata parsing (AbsoluteSourcePath and
TimeStamp); no coverage_if/switch/lookupswitch, dump/writer implementations or
interpreter coverage hooks were found. This is a coverage subsystem gap, not
just an option spelling change.

Primary `-nmN` parses N via atomi, retains max(N-currentMonitorCount,0), then
calls expandMonCache0x100370ED after FindClass(main), before Compiler loading
and main resolution. Primary init0x10037BDD allocates128 monitors; current
legacy init allocates32. Current parser rejects -nm. Current expandMonCache
already exists but is static and uses legacy cache storage. Implementing this
option requires a supported capacity entry point and checking requested counts
against the recovered initialization timing, not silently ignoring the option.
