# JNI reconstruction evidence

Primary: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Secondary: Rhapsody DR2 `libjava.A.dylib`, symbol-bearing 32-bit x86 image.
No modern JNI implementation was used as an implementation source.

## Recovered ABI

The primary native interface table at `100643d8` has exactly the 220 slots
declared in the shipped 1.1.5 `jni.h`, including its reserved slots. The secondary
table is at `5c152908`. Invocation table: primary `10064748`, secondary
`5c152fe8`. The public header is unchanged.

| Operation | Primary | Secondary / recovered behavior |
| --- | --- | --- |
| Add local/global reference | `10019d94` | `jni_AddRefCell`, `5c116af8`; counted indexed cells, initial capacity 4, doubling growth |
| JNI native invocation | `100131c0` | `invokeJNINativeMethod`, `5c10dd50`; save frame base/top, translate reference arguments, resolve reference result before restoring frame |
| Synchronized JNI invocation | `100135f7` | monitor surrounds native invocation |
| Lazy native dispatch | `10013a46` | old stub lookup first, JNI lookup second, under class monitor |
| Native symbol lookup | `10013c0f` | `dynoLinkJNI`, `5c10e5a8`; short and signature-qualified names, JNI escapes, stdcall decorations |
| Native x86 call | `10053218` | `sysInvokeNative`, `5c13c720`; reversed stack slots, environment/class prefix, EAX/EDX or x87 result |
| Java callback dispatch | `1001ae21` | validates receiver/declaring class, selects virtual/nonvirtual/static invocation, creates interpreter frame |
| DefineClass | `10019fb1` | rejects duplicate name/loader, parses class, initializes class metadata |
| FindClass | `1001a1ca` | `FindClassFromClass(ee,name,TRUE,NULL)` |
| AllocObject | `1001a995` | rejects abstract/interface classes, allocates without constructor |
| NewString / NewStringUTF | `1001e581`, `1001ea34` | character array then String constructor; modified UTF encoding |
| GetStringChars | `1001e728` | pins backing character array, isCopy=false |
| GetStringUTFChars | `1001eb51` | allocated UTF buffer, isCopy=true |
| NewObjectArray | `1001ecbb` | component metadata in trailing array word; validates initial value |
| Primitive array elements | e.g. `1001f50a` release | direct pinned storage; release mode ignored, pin reference count decremented |
| Register / UnregisterNatives | `100201ca`, `100202f0` | native method code/invoker replacement; reset to lazy invoker |
| GetDefaultJavaVMInitArgs | `10026fcd`, platform `100024ea` | version 10001, 128 KiB native stack, 400 KiB Java stack, 1/16 MiB heaps, verify remote |
| Create / enumerate VM | `10027088`, `10027393` | one VM per process, configure hooks/options then bootstrap runtime |
| Attach / detach | `1002755f`, `100277e0` | per-host-thread native registration + Java Thread, priority 5 before init; main detach rejected |
| DestroyJavaVM | `100274f9`, `1000290b` | waits for other user threads, returns -1; teardown unsupported in the original |
| FatalError | `1001a604` | diagnostic via jio hook then sysExit(1) |
| Output / exit / abort hooks | `1004e3de`, `1004bf67`, `1004c04c` | configured callbacks replace corresponding native operation, including returning callbacks |

JNI references are signed integer indices encoded in the public pointer type:
positive local, negative global, zero null. They are **not** old Java handles.
Method/field IDs use this tree's corresponding metadata pointers. Native-call
local frames restore their previous base/top on both success and exception.

## Explicit transitional deviations

- The current ExecEnv layout remains the earlier representation. An appended
  `jni_state` points to the function table, indexed reference cells, and owner EE;
  Sun places that metadata directly in the EE. No binary EE-layout compatibility
  is claimed. The public `JNIEnv *` and table ABI match.
- The existing heap lock protects JNI reference/pin registries instead of adding
  Sun's separate reference-table locks. The old stop-the-world collector enumerates
  those roots directly. Pinned objects mark both handle and body to prevent body
  relocation. This avoids suspending a thread while it owns a separate JNI lock.
- The Java callback adapter uses `do_execute_java_method_vararg` to create the
  existing interpreter's pseudo-frame, rather than copying Sun's internal frame
  layout. Supported platforms are both 32-bit x86. Unix native calls additionally
  align the outgoing stack to 16 bytes for contemporary compiler-generated code.
- JNI host-thread attachment uses the current platform queue/TLS layer and
  pthreads on Unix. It attaches the caller; it does not create a replacement thread.
- Launcher/invocation startup share the extracted existing bootstrap sequence.
  Runtime diagnostic stdio is routed through the recovered jio callback wrappers
  using internal `jni_io.h`; shipped JNI headers are not modified.
- The old collector roots all classes. `enableClassGC=1` therefore still lacks
  Sun's class unloading semantics. Full Sun GC/header reconstruction remains a
  Phase 1 gate; passing these JNI fixtures does not close that gate.
- FatalError walks the existing interpreter frames through `pc2string` with
  Sun's `JNI panic:` and `\tat` prefixes. Compiled/JIT frame decoding remains
  dependent on the separate compiler/frame-layout migration.

## Differential tests

`JniHello` was first run with a DLL built against the vendor headers on the Sun
VM and captured; the reconstructed VM initially failed `UnsatisfiedLinkError`.

`JniAbi` covers primitive/object/static fields, 64-bit and floating native
arguments/results, Java callbacks (variadic and A), static/nonvirtual calls,
counted local/global references, modified UTF including embedded NUL, all eight
primitive array families, object-array checks, pinned array access across GC,
pending exceptions, registered natives, overload lookup and underscore escaping.
`DefineClass` consumes the exact Sun-javac bytes for `JniDefined.java`, embedded
in `tests/native/jnidefined.h` so the test does not depend on resource lookup.

`jniinvoke` loads the VM dynamically, enumerates before/after startup, installs a
property, calls `Hello115.main`, attaches a separate native host thread, calls
Java there, detaches it, checks vfprintf/exit/abort hooks, then observes Sun's
DestroyJavaVM=-1. Expected output was captured from the original executable
using the same harness. Host stdout is explicitly flushed before Java output.

Fresh Windows Release verification: `ctest --test-dir out -C Release -R
"Jni|jni-invocation" --output-on-failure`: 3/3 passed, including the expanded
array/DefineClass/hook cases. Fresh Linux Release targeted builds and both
`ctest --test-dir out/linux115 -R vm-Jni --output-on-failure` (2/2) and
`-R jni-invocation` (1/1) passed, including the actual Thread constructor on
native attachment and its generated Thread-N name. Debug verification is
coordinated by the parent agent and must be recorded separately.
