# javah -jni output reconstruction

Oracle: `vendor/jdk1.1.5/bin/javah.exe` with the supplied JDK classes.zip.
JNI_Test.java is compiled by that distribution's javac. Both checked-in
`*.jni.expected.h` files are actual oracle output, not hand-authored expectations.

Observed behavior implemented in javah.c:

- Emits `<jni.h>`, C++ extern guards, per-native class/method/signature comments,
  JNIEXPORT/JNICALL prototypes and JNIEnv plus jobject/jclass receiver.
- Emits native methods declared by the class, not inherited methods or fields.
  The primitive and String constants in JNI_Test deliberately produce no macros.
- Maps all eight primitive arrays and reference/multidimensional arrays; maps
  String/Class specially and Throwable subclasses (including IOException) to
  jthrowable.
- Native overloads use signature-qualified JNI names. A nonnative overload does
  not cause a native method's name to gain a signature suffix in this release.
- JNI function names use `_1`, `_2`, `_3`, `_0xxxx` escapes. The older header
  filename/guard spelling instead escapes an underscore as `_0005f` and maps
  package separators to underscores. Unicode method comments are escaped while
  ordinary underscores in method comments remain unchanged.
- `-jni -stubs` fails with `The JNI does not need stub files`.

Windows Release generated headers match both Sun oracle files after newline
normalization. The legacy header/stub path remains selected without -jni.
This implements header generation only; it does not claim a working JNI VM.
