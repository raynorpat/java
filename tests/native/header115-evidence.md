# Independent header migration

The vendor `include` directory contains 41 shared headers. The initial audit
found five text-identical current headers: java_lang_String.h,
java_lang_Thread.h, java_lang_ThreadGroup.h, jni.h, and zip.h.

Ten headers differed only in comments/whitespace and were copied verbatim:
bool.h, byteorder.h, common_exceptions.h, native.h, path.h, signature.h,
StubPreamble.h, timeval.h, typecodes.h, verify.h. Binary equality was checked.
In particular, native.h is not coupled to the ClassClass layout migration.

The next independent slice adopts config.h, utf.h, log.h, typedefs.h, and
tree.h with exact vendor text. Their narrow consumer adjustments are:

- utf.c uses the original signed int length parameters in both declarations
  and definitions. The conversion algorithm is unchanged.
- JNI float return bit conversion uses memcpy rather than the non-Sun added
  Java8.f union member. The original Java8 union is restored without a layout
  extension.
- log.h names jio_fprintf directly, which the invocation implementation now
  supplies. The previous guarded diagnostic routing was moved from typedefs.h
  into debug.h. A recursive include audit of C printf/fprintf/vfprintf callers
  found no caller previously reaching typedefs.h that fails to reach debug.h.
- debug.h adopts the vendor declarations but retains a marked export annotation
  and the marked transitional JNI diagnostic routing include. It is therefore
  not counted as byte-identical.

Deferred instead of installing unsupported contracts:

- decode.h adds PrintPublicOnly/PrintInternalSigs absent from the legacy decoder;
  current javap runs the shipped Java implementation instead.
- profile.h initially deferred; the later string/profile tranche restores its
  prof_file consumer and retains the existing shared prof_md.h no-op adapter.
  The vendor tree lacks prof_md.h; the repository fallback was initially missed.
- javaString.h initially deferred; its helpers were subsequently recovered
  from the primary DLL. See string-helpers115-evidence.md for that tranche.
- jmath.h's original _MATH_H_ guard may conflict with the host C math header.

The coupled oobj/interpreter/sys_api, collector/finalizer, monitor-cache and
thread-header work remains open. Existing functional gates do not establish
those layouts match Sun. This slice makes 20 of the 41 shared headers textually
identical; it does not claim whole-header or VM ABI fidelity.

Verification before integration: isolated x86 MSVC C and Linux C89 compilation
of utf.c and jni.c passed. Coordinated Windows and Linux builds and regressions
must follow.

After the coordinated Windows Debug rebuild succeeded, an isolated JRE bin
image passed Hello, JniHello and JniAbi, all exit 0. JniAbi reported abi ok,
wide=1234605616436508564, real=11.75, over=5/3, under=16, registered=109.
These exercise the restored Java8 declaration and JNI float-return bit copy.
Full Debug CTest is coordinated by the main agent to avoid fixture collisions.

Reviewer reports the coordinated Linux Release rebuild with these headers
passed all five JNI gates: jni-layout, JniHello, JniAbi, jni-invocation,
javah-jni. Subsequent JNI attachment refinement is separate from this slice.
