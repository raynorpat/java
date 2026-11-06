# System stream setters and bootstrap

Read directly from vendor/jdk1.1.5/bin/javai_g.dll with IDA instance
1493265a537f. No guessed setter semantics are attributed to Sun.

| Stub | Native implementation | JNI helper | Field/signature |
| --- | --- | --- | --- |
| setIn0 10061462 | 1004B8E5 | 1004B80A | in, Ljava/io/InputStream; |
| setOut0 10061485 | 1004B95E | 1004B853 | out, Ljava/io/PrintStream; |
| setErr0 100614A8 | 1004B9D7 | 1004B89C | err, Ljava/io/PrintStream; |

Each legacy stub passes a null receiver and stack[0] to the native function,
then returns the original stack pointer (void return). Each native obtains
the JNIEnv at EE()+20, uses JNI FindClass for java/lang/System, converts the
incoming native handle to a local reference using 10019D94, calls its helper,
and deletes both local references. The helper uses GetStaticFieldID (JNI
table byte offset 576) then SetStaticObjectField (offset 616). FindClass is
offset 24; DeleteLocalRef is offset 92. Null stream values are not rejected.

system115.c maps these operations to current native handles and static-field
storage. This is explicitly marked a deviation from Sun's JNI path; replacing
it with that path depends on the completed JNI runtime. Field selection uses
the exact name, signature, and static flag. System declares all three fields,
so no superclass field search is needed for these setters.

Integration signatures (all void):

```
java_lang_System_setIn0(struct Hjava_lang_System *, struct Hjava_io_InputStream *);
java_lang_System_setOut0(struct Hjava_lang_System *, struct Hjava_io_PrintStream *);
java_lang_System_setErr0(struct Hjava_lang_System *, struct Hjava_io_PrintStream *);
```

Add system115.c to the runtime and register/export their legacy native stubs.
The source uses forward declarations for handle tags and requires no new
generated object-layout headers. Existing Unicode fixture exercises setOut
and restoration; ordinary boot exercises all three initial setters.

The initializeSystemClass string at 10068AA0 has one xref, 10002841, inside
bootstrap function 100025E3. Its recovered sequence is:

1. Initialize exec environment, heap, and runtime infrastructure.
2. Initialize the main Java thread via 1004D5A4, fail on null.
3. Set that thread's name to MakeString("main", 4) via 1004D759.
4. FindClass(ee, "java/lang/System", TRUE), fail on null.
5. execute_java_static_method(ee, system, "initializeSystemClass", "()V").
6. FindClass(ee, "java/lang/Compiler", TRUE).
7. Optional debugger startup and remaining thread startup.

Sun's shipped System.java confirms initializeSystemClass is private and called
after thread initialization. It creates Properties, calls initProperties,
constructs streams around FileDescriptor.in/out/err, and calls the three
setters. Therefore running only System's class initializer intentionally
leaves its streams null. Compatibility with the current 1.0 boot path requires
conditional invocation when this private method exists; that integration is
owned by the main agent. No reconstructed-VM execution was performed for this
isolated source before integration.
