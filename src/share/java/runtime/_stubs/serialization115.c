/* Reconstructed Sun 1.1.5 serialization native stack adapters. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectOutputStream_outputClassFields_stub(stack_item *p, struct execenv *ee)
{
    extern void java_io_ObjectOutputStream_outputClassFields(void *, void *, void *, void *);
    java_io_ObjectOutputStream_outputClassFields(p[0].p, p[1].p, p[2].p, p[3].p);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectOutputStream_invokeObjectWriter_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectOutputStream_invokeObjectWriter(void *, void *, void *);
    p[0].i = java_io_ObjectOutputStream_invokeObjectWriter(p[0].p, p[1].p, p[2].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectInputStream_loadClass0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectInputStream_loadClass0(void *, void *, void *);
    p[0].p = java_io_ObjectInputStream_loadClass0(p[0].p, p[1].p, p[2].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectInputStream_inputClassFields_stub(stack_item *p, struct execenv *ee)
{
    extern void java_io_ObjectInputStream_inputClassFields(void *, void *, void *, void *);
    java_io_ObjectInputStream_inputClassFields(p[0].p, p[1].p, p[2].p, p[3].p);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectInputStream_allocateNewObject_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectInputStream_allocateNewObject(void *, void *, void *);
    p[0].p = java_io_ObjectInputStream_allocateNewObject(NULL, p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectInputStream_allocateNewArray_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectInputStream_allocateNewArray(void *, void *, long);
    p[0].p = java_io_ObjectInputStream_allocateNewArray(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectInputStream_invokeObjectReader_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectInputStream_invokeObjectReader(void *, void *, void *);
    p[0].i = java_io_ObjectInputStream_invokeObjectReader(p[0].p, p[1].p, p[2].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getClassAccess_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectStreamClass_getClassAccess(void *, void *);
    p[0].i = java_io_ObjectStreamClass_getClassAccess(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getMethodSignatures_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectStreamClass_getMethodSignatures(void *, void *);
    p[0].p = java_io_ObjectStreamClass_getMethodSignatures(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getMethodAccess_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectStreamClass_getMethodAccess(void *, void *, void *);
    p[0].i = java_io_ObjectStreamClass_getMethodAccess(NULL, p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getFieldSignatures_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectStreamClass_getFieldSignatures(void *, void *);
    p[0].p = java_io_ObjectStreamClass_getFieldSignatures(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getFieldAccess_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectStreamClass_getFieldAccess(void *, void *, void *);
    p[0].i = java_io_ObjectStreamClass_getFieldAccess(NULL, p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getFields0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_io_ObjectStreamClass_getFields0(void *, void *);
    p[0].p = java_io_ObjectStreamClass_getFields0(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_getSerialVersionUID_stub(stack_item *p, struct execenv *ee)
{
    extern int64_t java_io_ObjectStreamClass_getSerialVersionUID(void *, void *);
    Java8 temporary;
    SET_INT64(temporary, p, java_io_ObjectStreamClass_getSerialVersionUID(NULL, p[0].p));
    return p + 2;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_io_ObjectStreamClass_hasWriteObject_stub(stack_item *p, struct execenv *ee)
{
    extern long java_io_ObjectStreamClass_hasWriteObject(void *, void *);
    p[0].i = java_io_ObjectStreamClass_hasWriteObject(NULL, p[0].p);
    return p + 1;
}
