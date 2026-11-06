/* Reconstructed legacy adapters for Sun 1.1.5 file natives. */
#include "StubPreamble.h"
JAVA_PUBLIC_API(stack_item *)
Java_java_io_FileOutputStream_openAppend_stub(stack_item *p, struct execenv *ee)
{
    extern void java_io_FileOutputStream_openAppend(void *, void *);
    java_io_FileOutputStream_openAppend(p[0].p, p[1].p);
    return p;
}
JAVA_PUBLIC_API(stack_item *)
Java_java_io_File_rmdir0_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_File_rmdir0(void *);
    p[0].i = java_io_File_rmdir0(p[0].p);
    return p + 1;
}
