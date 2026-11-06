# ResourceBundle class-context native

Primary evidence: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.

The exported static stub at `0x1006221e` calls native `0x1004027c` with a null
receiver, stores one reference in the operand stack, and returns stack + 1.
The native directly calls the class-context walker at `0x10040113`.
SecurityManager.getClassContext at `0x1003ffb5` invokes the same walker only
after its receiver initialization check succeeds. ResourceBundle has no such
check.

The walker counts frames having a current method, allocates a Class array,
sets its trailing component-class metadata to java.lang.Class, and fills it
with each frame's declaring class handle, from the current frame outward.
Machine-compiled frames use CompiledFramePrev; empty frames use frame->prev.
Allocation failure raises OutOfMemoryError. The existing SecurityManager
implementation already matched this walk, so it is now shared by the two
native wrappers without changing its frame traversal or allocation behavior.

Rhapsody DR2 names `_java_util_ResourceBundle_getClassContext` at `0x5c12e8a8`
and confirms the same traversal and array construction.

Before implementation, Dates against Sun's 1.1.5 classes failed with
UnsatisfiedLinkError for java/util/ResourceBundle.getClassContext, reached
through Calendar.setWeekCountData and ResourceBundle.getLoader.
