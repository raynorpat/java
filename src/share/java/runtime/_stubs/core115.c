/* Sun 1.1.5 stack adapters; included by stubs.c. */
#include "StubPreamble.h"
JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_runFinalizersOnExit0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_Runtime_runFinalizersOnExit0(void *, long);
    java_lang_Runtime_runFinalizersOnExit0(NULL, p[0].i); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Class_getSigners_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_Class_getSigners(void *);
    p[0].p = java_lang_Class_getSigners(p[0].p); return p + 1;
}
JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Class_setSigners_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_Class_setSigners(void *, void *);
    java_lang_Class_setSigners(p[0].p, p[1].p); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_java_lang_SecurityManager_currentLoadedClass0_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_SecurityManager_currentLoadedClass0(void *);
    p[0].p = java_lang_SecurityManager_currentLoadedClass0(p[0].p); return p + 1;
}
JAVA_PUBLIC_API(stack_item*)
Java_java_io_FileDescriptor_sync_stub(stack_item *p, struct execenv *ee)
{
    extern void java_io_FileDescriptor_sync(void *);
    java_io_FileDescriptor_sync(p[0].p); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_java_io_File_canonPath_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_io_File_canonPath(void *, void *);
    p[0].p = java_io_File_canonPath(p[0].p, p[1].p); return p + 1;
}
