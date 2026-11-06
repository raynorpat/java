# Runtime ABI export audit

Primary source: vendor/jdk1.1.5/bin/javai_g.dll, IDA instance1493265a537f.
The full Debug export manifest is tools/exports-sun115-debug.txt, captured from
idautils.Entries():287 actual exports. IDA also lists the PE startup entry
DllEntryPoint with ordinal equal to its address268842176; that non-export is
excluded. Comparing all names against the286-name Release manifest shows one
Debug-only entry, `_set_debugger_hooks@12` (ordinal274, address10002980), and no
Release-only entries. Configuration-specific gates must use these separate
manifests; the expected set is not the reconstructed DLL's current exports.
The missing-name list in out/tests-win.log was compared against
tools/exports-sun115.txt and current definitions. Existing global runtime
functions are exposed through javai.def. ProcedureFindThrowTag's internal
linkage was removed; its four-argument shape agrees with Sun1001864C. This
establishes symbol availability, not binary structure compatibility: this VM
still uses its older ClassClass and JavaFrame layouts.

New exports115.c helpers:

- MakeClassSticky1000D811 sets the sticky class bit. Current CCF_Sticky0x200
  replaces Sun0x400; the existing collector already honors the current bit.
- FindStickySystemClass10012355 resolves through FindClassFromClass with null
  context and marks the result sticky. Current ClassClass pointers replace Sun
  class handles, consistent with this tree's existing FindClass export.
- Win32 sleep1004D3DC calls Sleep(seconds*1000) and returns0. POSIX supplies its
  existing libc sleep implementation.

Recovered native entry points:

| Operation | Evidence |
| --- | --- |
| set_single_step_thread | 10002A15 updates Thread.single_step and global count only on changes |
| set_breakpoint / get_opcode / clear_breakpoint | 10002A6D / 10002C31 / 10002DA0 |
| breakpoint lookup / restore | 10002AFA / 10002E0A; doubling native table, pc+saved opcode |
| JDB_loadclass_locked | 10002EB2 queries actual class-load monitor ownership |
| lock_for_debugger / unlock_for_debugger | 1002D2C0 / 1002D36C acquire/release coordinated VM locks |
| VM.getState | 1002C97B reads GC policy state and forces collection when not1 |
| VM.threadsSuspended | 1004DDF0 reads platform-wide suspension flag |
| VM.unsuspendThreads / unsuspendSomeThreads | 1004DE05 / 1004DE15 enumerate platform threads |
| VM.resumeJavaMonitor / suspendJavaMonitor | 10039D08 / 10039D39 |
| VM.resetJavaMonitor / writeJavaMonitorReport | 10039D4E / 10039D7A |

breakpoints115.c now implements the native table and hook callbacks. Debug
interpreter dispatch, quickening, exception notification, and single stepping
use that implementation rather than the older Java BreakpointHandler storage.
Sun10002980 registers step/breakpoint/exception hooks;10002C80 dispatches saved
opcodes and detects phantom breakpoints;10002D39 preserves rewritten opcodes.
JDB lock queries use actual monitor ownership. The coordinated debugger lock
maps to current class/heap/finalizer/thread/cache/registry/code locks; the current
heap lock also protects strings/JNI. Sun's separate exit and allocator locks
have no counterparts, so this remains a documented synchronization mapping.

Release differs from Debug: explicitly opened javai.dll instance09acfa8fc72a
shows get_opcode10002320 returning the byte and set_breakpoint10002330,
clear_breakpoint10002340, set_single_step_thread10002350 as empty functions.
Those Release no-ops are direct binary behavior, not substitute implementations.
The harness ignores their unspecified return registers.

Debug115 invokes a real Java target while its first bytecode is patched, checks
restoration and error types, and tests debugger class-lock ownership. The same
JNI harness runs on Sun using its methodblock.code offset24, verified at1001864C;
the current VM uses offsetof(methodblock,code). Actual Sun Release exits0 with
false/true/true/false/false/true; actual Sun Debug exits0 with all six true, both
with empty stderr. Debug115.expected and Debug115-debug.expected are captured
separately. Sun java_g inserts _g into the explicitly loaded library filename,
so capture needs an identical debug115_g.dll copy.

Profiler helpers now operate on profiler.c's real table. Heap-lock protection
coordinates reset/resume with active writers and report traversal instead of
Sun's separate profiling lock. Profiler115 verifies the report contains the
native suspend call and reset removes it. It calls System.exit because Sun's
sun.misc.VM initializes non-daemon notification threads. The reconstructed
Windows Debug VM exits0 and exactly matches normalized oracle output.
VM GC-state and
thread-suspension routines require deeper migration of policy/suspension state.
Fixed green/false values or empty functions would not reconstruct these APIs.

Release DumpThreads/DumpMonitors now compile their real existing diagnostic
implementations. The stack dump prints each current frame before advancing;
the former DEBUG-only code advanced first and could dereference a null frame.
jio_printf1004E364 and jio_fprintf1004E3A4 delegate to jio_vfprintf1004E3DE, which
uses an embedding callback or libc vfprintf. The callback is JNICALL as required
by JDK1_1InitArgs. Local stdio macros are undefined around the libc fallback.

Legacy extra native adapters currently support the old class-library baseline.
Before hiding them from the DLL export table, introduce a built-in native-name
lookup table, have native binding consult it before external dynamic lookup,
then remove dllexport from legacy-only adapters. Verify both Sun115 and old
baseline tests after each step. Removing exports alone would break native binding.

String interning review found no actionable normal-path GC or locking defect:
native stack/register roots protect live handles, and the heap lock serializes
the intern table with GC marking. A suspected Str2ID allocation-failure recovery
problem was withdrawn because out_of_memory aborts rather than returning.
