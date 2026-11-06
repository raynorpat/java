# Thread and ClassLoader header adoption

The shipped threads.h, exceptions.h, and java_lang_ClassLoader.h are now
installed verbatim in src/share/java/include. Core consumers use threads.h;
javaThreads.h remains a compatibility forwarding header for the unconverted
AWT source and gc.c, which was deliberately left untouched in this tranche.
JavaStackSize's definition is long and threadSleep takes int, matching Sun's
header. The additions to interrupt macros use existing implemented HPI APIs.

threadTryVMSuspend remains an unused declaration present in the original
header. No implementation was invented. In primary javai_g.dll the VM-suspended
flag at1007F620 has only three references: suspension1004DC32,
unsuspension1004DD22, and getter1004DDF0. The neighboring1004DC22 is the distinct
threadSuspendMe wrapper calling the Win32 no-op1004CDBC. Secondary Rhapsody
libjava has threadSuspendMe at5C130BD8 but no threadTryVMSuspend symbol. There
are no current core callers requiring the unused declaration's implementation.

The original ClassLoader layout is initialized at offset0 and classes at offset4
(eight bytes on the supported x86 ABI). Existing class.c and security.c access
initialized only; object allocation and GC reference slots come from the loaded
Java metadata. Three real C entrypoints defineClass0/resolveClass0/findSystemClass0
now forward the retained parser/resolver functions, and the corresponding
native adapters invoke those original C names. These forwards are explicitly
transitional until ClassClass and class parsing migrate; they are not claims
of recovered parser implementation. The private checkInitialized declaration
is retained in class.c rather than changing the original shipped header.

Verification before the subsequent class-handle migration: full Windows Release
build passed, followed by ten targeted gates covering thread layout, HPI monitor,
interrupt state, Java interrupt, threads, monitors, Core115, Resources, JniHello,
and JniAbi. Linux paired build/gates are recorded below when complete.
Linux Release full build and paired Core115, Resources, Interrupt and monitors gates passed before the subsequent layout migration.
