# Verifier and old-class evidence

The plan's `07baa55:classsrc/classes.zip` path is absent. The tracked
`classsrc/classes.zip` supplies `java/util/ObserverList.class`, copied unchanged
to `tests/vm/old`. Sun 1.1.5 has no class of that name. Both VMs load it and
report one declared method; the existing inner, local and anonymous tests run
in the same fixture. This replaces the invalid historical extraction command.

`bad/BadStack.java` is compiled by Sun javac, then its unique bytes
`00 00 00 02 04 ac` (two-byte Code body, iconst_1/ireturn) are changed to
`00 00 00 02 01 ac` (aconst_null/ireturn). `BadVerify` runs with `-verify`.
Sun prints `java.lang.VerifyError: BadStack` without a verifier diagnostic.
The baseline instead printed its internal diagnostic and replaced the pending
VerifyError with ClassNotFoundException.

Rhapsody `java_lang_Class_forName` at 0x5c1083b8 only creates a not-found
exception when none is pending. Win32 CCerror at 0x1000a3cc and Rhapsody
0x5c106c58 print verifier details only when `verbose` is set. Those two
differences are corrected; this does not claim a complete verifier migration.

The static-initializer behavior differs from the plan's assumption: Sun's
second use does not raise NoClassDefFoundError. See the separate clinit evidence
and captured oracle, which take precedence over that assumption.
