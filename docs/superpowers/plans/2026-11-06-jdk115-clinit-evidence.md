# Static initializer exception recovery

Primary evidence: `vendor/jdk1.1.5/bin/javai_g.dll`,
SHA-256 `1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.

`RunStaticInitializers` is at VA `0x10013e04`, identified by its static
initializer diagnostic and `ExceptionInInitializerError` string references.
After invoking `<clinit>`, it calls `free_clinit_memory` at `0x1000fe54`,
which clears the method code pointer, and changes the invoker to the abstract
method handler at `0x10013b4c`.

At `0x10013f88`, the pending exception is tested with `is_instance_of` against
global `0x10083594`. `Locked_InitializeClass` at `0x1001082a` initializes this
global with the sticky system `java/lang/Error` class. If the test succeeds,
the original exception remains pending unchanged. Otherwise the exception flag
is cleared and `execute_java_constructor` is called with
`java/lang/ExceptionInInitializerError`, signature `(Ljava/lang/Throwable;)`,
and the original exception as its argument. Disassembly at `0x10013f9f` through
`0x10013fb8` confirms the fifth argument omitted by the initial decompiler
prototype. The wrapper is thrown only if construction did not itself throw.

Rhapsody DR2 `libjava.A.dylib` names `_RunStaticInitializers` at `0x5c10e6e0`,
`_classJavaLangError` at `0x5c1538a4`, and the `free_clinit_memory` call. Its
initializer routine confirms the same Error-subclass test, constructor signature,
original throwable argument, and preservation of constructor failure exceptions.
Rhapsody also has separate initialization state flags; these are not substituted
for the primary Windows binary's behavior in this slice.

The old runtime returned the pending exception's class name to the resolver,
which then cleared `Resolved` and set `Error`. Sun's initializer routine instead
communicates failure through the pending exception. The compatibility adapter
therefore returns NULL to the current resolver, leaving the class resolved.

Two explicit integration deviations remain: the current string-return function
interface is retained, and initializer bytecode allocated from the class arena
is not individually freed. Its pointer is cleared and its invoker disabled as
in Sun's binary; the arena retains the allocation until class unloading.

## Verification

The existing `ClinitFail` oracle was independently rerun against Sun's VM and
matched its committed expected output. Before this change, the reconstructed VM
printed a raw IllegalStateException on first access, then NoClassDefFoundError
for every subsequent access and construction. Sun instead wraps the first
failure, returns zero on the second static-field access, and allows a second
construction of the other failed-initializer class.

After implementation and the matching sticky Error global addition, the Windows
Release VM matches `ClinitFail.expected` exactly through `run-java-test.cmake`.
A temporary `ErrorClinitReview` fixture in the ignored build directory throws a
previously saved LinkageError from an initializer: both Sun and the reconstructed
VM print `java.lang.LinkageError same=true`, then `value=0`. This checks that
Error subclasses retain object identity and are not wrapped.
