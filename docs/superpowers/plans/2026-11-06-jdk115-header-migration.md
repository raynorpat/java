# Task 3 header migration evidence

## Independent first slice

Adopted `jni.h` and Windows `jni_md.h` verbatim from the vendor distribution.
Unix `jni_md.h` follows the existing port's `<stdint.h>` convention, with ordinary
C calling convention and external linkage. It is marked as a deviation, not
presented as recovered Sun Unix source.

`tests/native/jni-layout.c` checks primitive widths and signedness, the 32-bit
invocation table and initialization argument layout. This does not implement JNI
or claim that a VM can yet be created through JNI.

## Binary evidence

Primary: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
IDA successfully opened this binary in a new database; an unrelated stale IDA
worker's protocol mismatch does not block reconstruction.

At VA `0x10026fcd`, `JNI_GetDefaultJavaVMInitArgs` writes version `0x10001`,
javaStackSize at offset 16, enableClassGC at 48, debugging at 64, and debugPort
at 68, consistent with the shipped 72-byte `JDK1_1InitArgs`.
Its platform helper at `0x100024ea` sets nativeStackSize to `0x20000`, minHeapSize
to `0x100000`, maxHeapSize to `0x1000000`, and classpath through `0x10002527`.
These are evidence for the later invocation implementation, not implemented yet.

## Coupled interfaces remaining

The vendor tree has 41 shared headers and 14 Windows headers. Before the JNI
addition, 28 shared names already existed and 13 were new (the plan says 29+13).

| Interface | Current runtime | Sun 1.1.5 |
| --- | --- | --- |
| `sysThreadBootstrap` | thread output | thread output plus cookie |
| `sysMonitorWait` | monitor and timeout | monitor, timeout and interruptibility |
| `sysDynamicLink` | symbol and library output | symbol only |
| thread state | current platform structures | new interrupt, VM suspend and monitor cache fields |
| saved registers | `sysThreadRegisters` in shared sys_api | preserve as an explicit platform deviation |

Migrate these with their callers and platform implementations. Do not make a
header compile by dropping arguments or inventing stubs without binary evidence.
Keep the existing GC fixes while changing thread enumeration and register access.

Remaining new shared headers: `alloc_cache.h`, `breakpoints.h`, `gc.h`,
`java_lang_ClassLoader.h`, `java_lang_String.h`, `java_lang_Thread.h`,
`java_lang_ThreadGroup.h`, `jcov.h`, `reflect.h`, `selftest.h`, `threads.h`, `wrap.h`.

Task 3 remains in progress; replacement of the coupled runtime headers is tied to
the Task 4 boot gate. This slice leaves the current runtime ABI intact.

## Bootstrap and monitor initialization slice

Migrated `sysThreadBootstrap(thread**, cookie)` and `sysMonitorInit(monitor*)`
across declarations, both platform implementations, shared callers and tool stubs.
The bootstrap cookie is stored before queue publication; the existing early
backpointer setup remains necessary while the old runtime is still in use.
`hpi-signatures` is a compile-only target that rejected both old signatures before
this change. This is an incremental API migration, not full platform reconstruction.

Windows debug VM: bootstrap at `0x1004c89e` forwards the cookie to thread allocation
at `0x1004c770`, which stores it at offset zero before publishing the thread.
Monitor initialization at `0x10038410` takes one argument and initializes the
critical section and condition variable. Current SRW/pthreads implementations are
still retained; only the unused cache flag is removed in this slice.

Rhapsody cross-check: `_sysThreadBootstrap` at `0x5c144a38` stores its cookie before
queue publication; `_sysMonitorInit` at `0x5c13606c` takes one argument.
`_sysMonitorDestroy` at `0x5c1360c4` destroys condition variables. It is not the
current runtime's thread-exit cleanup callback, so that migration must accompany
changes to shared monitor cleanup rather than simply dropping its owner argument.

The shipped Windows `monitor_md.h` includes `condvar_md.h`, which is absent from
the supplied Windows include directory. Reconstruct that dependency from binary
evidence before adopting the monitor layout; do not substitute the current layout.
