# Runtime ABI export audit

Primary source: vendor/jdk1.1.5/bin/javai_g.dll, IDA instance1493265a537f.
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

Recovered but intentionally NOT supplied as placeholders:

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

Debugger integration needs a native breakpoint table connected to interpreter
dispatch, quickening, and thread stepping. Current BREAKPTS-disabled builds
treat breakpoint opcode as nop; merely exporting a table setter would not
implement debugger support. The older enabled path instead uses Java
BreakpointHandler storage. JDB lock queries must reflect actual monitor
ownership, not the global class_lock_depth counter.

Profiler helpers can map to profiler.c's real table, but reset/resume must
coordinate with active writers before freeing the table. VM GC-state and
thread-suspension routines require deeper migration of policy/suspension state.
Fixed green/false values or empty functions would not reconstruct these APIs.

Legacy extra native adapters currently support the old class-library baseline.
Before hiding them from the DLL export table, introduce a built-in native-name
lookup table, have native binding consult it before external dynamic lookup,
then remove dllexport from legacy-only adapters. Verify both Sun115 and old
baseline tests after each step. Removing exports alone would break native binding.

String interning review found no actionable normal-path GC or locking defect:
native stack/register roots protect live handles, and the heap lock serializes
the intern table with GC marking. A suspected Str2ID allocation-failure recovery
problem was withdrawn because out_of_memory aborts rather than returning.
