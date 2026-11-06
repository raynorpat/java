# Original monitor-cache migration oracle

Primary Debug DLL identity and RVAs are pinned in
monitor-finalizer115-header-evidence.md. New oracle probes are deliberately
not production DLL exports.

Captured from the actual Sun java_g.exe with MonitorCapacity115 and the
oracle-only native DLL reading monCount at RVA0x73EA8:

| Option | Capacity at Java main |
|---|---:|
| absent | 128 |
| -nm0 | 128 |
| -nm1 | 129 |
| -nm256 | 384 |

The launcher parses against the pre-initialization count0, then adds that delta
after finding the main class. Treating -nm256 as an after-init minimum256 would
be incorrect. Baseline reconstructed launcher rejects -nm256 with exit1;
Sun accepts it with exit0 and runs Launcher115 (inherited=1, nm).

monitor-cache115.c SUN_ORACLE mode loads the pinned Debug DLL, initializes a
VM, verifies initial128, calls original expandMonCache RVA0x370ED with257,
verifies385, then holds one monitor while entering/exiting4095 distinct keys.
The total remains385, proving reclamation and the held-monitor protection are
observable requirements. This probe passes against Sun. The production mode
will additionally require offsetof(monitor_t,mid)==8 and link private capacity
state through the existing core object target, without expanding DLL exports.

Recovered implementation boundary:

- monitor.h original key,next,mid has offset8 and sizeof12 on x86. Primary
  allocations stride sizeof(monitor_t)+sysMonitorSizeof, not offsetof(mid)+size.
- Cache lock is recursive and separate from a registered monitor. Windows
  original cache_lock_t is mutex_t, long entry_count, unsigned-long owner.
- lookup/create use per-thread8-slot cache and publish current key before lookup;
  key clears after the actual monitor operation. A memory flush on MP machines
  separates publishing/reading during concurrent reclamation.
- findMonitor0x10037EB3 finds/creates in the global hash under cache lock.
- Replenish0x10037588 drops cache lock before taking threadqueue lock, then
  reacquires cache. It serializes growth via an expanding-thread marker and
  waits on the queue monitor. Recursive allocation by that thread consumes
  reserved free monitors. Expansion quantum16, low-water mark5.
- Reclaim0x1003797E enumerates threads under queue+cache locks, clears their
  local caches, barriers, then marks each current key using bit0 of next.
  A monitor is retained when marked or sysMonitorInUse; otherwise key clears
  and it moves to the free list. No collector/object-layout change required.
- Cache enumeration masks pointer tags and saves next before callbacks.

GC owner confirmed current GC holds heap->hasfinal->finalme->queue across
stop-world and does not acquire cache lock or call monitorEnumerate. Thus the
recovered queue->cache order does not invert collector locks. Unlocked cache
enumeration requires its caller to hold cache; ordinary wrapper retains that
responsibility. Unix requires narrow per-thread cache fields and a recursive
pthread cache lock adapter; Windows already has original per-thread fields.

After checkpoint d067dd3, the recovered cache implementation and exact original
monitor.h/monitor_cache.h were adopted. The retained collector's polling macro
moved to platform sysmacros. Unix gets an explicitly native recursive mutex
adapter and eight per-thread cache entries; Windows uses the original lock
macros. No public DLL exports were added for capacity probes. Existing
thread-exit cleanup remains a port adapter, while reuse is now determined by
cache reclamation rather than immediate monitorDestroy.

The original hash allocation starts with128 buckets, grows repeatedly until
2*buckets > total monitors, and strides allocations by sizeof(monitor_t) plus
sysMonitorSizeof. Allocation failure while growing only the hash table retains
the old table and makes new monitors available, matching the primary.

Java MonitorCache115 runs four threads through1024 live locks, nested shared
ownership, yielding, and explicit collections. Original Sun output is
`monitor churn=2048`. This complements the native held-monitor reuse probe.
Integration verification after implementation:

- Linux Release rebuilt java/javai and native core probes. New monitor-cache115
  passes layout8, recursive cache-lock ownership, exact capacities128/385 and
  held-monitor reuse. Allocation-cache, both managed-class modes, and
  interrupt-state pass (five native cases total).
- Linux Release14 Java cases pass: MonitorCapacityOption115, MonitorCache115,
  NumericConversions115, Circularity115, Interrupt, FinalizeQueue115, five
  FinalizeExit modes, stress, monitors and threads.
- Coordinated Windows Release log out/platform-tests-win-release.log confirms
  monitor-cache115, both new VM monitor cases, monitors and threads pass.
  Its unrelated classpath normalization failure was handled separately.
- Windows Debug standalone hpi-monitor rebuilt and passed after adding native
  probe-only DumpThreads/panic diagnostics required by original sysAssert.
  The panic adapter prints and aborts; assertions remain active.
- Both monitor.h and monitor_cache.h were byte-compared equal to vendor originals.

Full four-configuration aggregate gates remain coordinated by the root task.
