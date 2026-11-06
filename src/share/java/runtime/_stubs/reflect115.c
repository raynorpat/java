/* Reconstructed legacy native stack adapters; signatures from Sun 1.1.5 exports. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_isAssignableFrom_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Class_isAssignableFrom(void *, void *);
    p[0].i = java_lang_Class_isAssignableFrom(p[0].p, p[1].p);
    return p + 1;
}
JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_isInstance_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Class_isInstance(void *, void *);
    p[0].i = java_lang_Class_isInstance(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_isArray_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Class_isArray(void *);
    p[0].i = java_lang_Class_isArray(p[0].p);
    return p + 1;
}
JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getModifiers_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Class_getModifiers(void *);
    p[0].i = java_lang_Class_getModifiers(p[0].p);
    return p + 1;
}
JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getComponentType_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_Class_getComponentType(void *);
    p[0].p = java_lang_Class_getComponentType(p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getFields0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getFields0(void *, long);
    p[0].p = java_lang_Class_getFields0(p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getMethods0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getMethods0(void *, long);
    p[0].p = java_lang_Class_getMethods0(p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getConstructors0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getConstructors0(void *, long);
    p[0].p = java_lang_Class_getConstructors0(p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getField0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getField0(void *, void *, long);
    p[0].p = java_lang_Class_getField0(p[0].p, p[1].p, p[2].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getMethod0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getMethod0(void *, void *, void *, long);
    p[0].p = java_lang_Class_getMethod0(p[0].p, p[1].p, p[2].p, p[3].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Class_getConstructor0_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_Class_getConstructor0(void *, void *, long);
    p[0].p = java_lang_Class_getConstructor0(p[0].p, p[1].p, p[2].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getModifiers_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getModifiers(void *);
    p[0].i = java_lang_reflect_Field_getModifiers(p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Method_getModifiers_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Method_getModifiers(void *);
    p[0].i = java_lang_reflect_Method_getModifiers(p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Constructor_getModifiers_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Constructor_getModifiers(void *);
    p[0].i = java_lang_reflect_Constructor_getModifiers(p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Method_invoke_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Method_invoke(void *, void *, void *);
    p[0].p = java_lang_reflect_Method_invoke(p[0].p, p[1].p, p[2].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Constructor_newInstance_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Constructor_newInstance(void *, void *);
    p[0].p = java_lang_reflect_Constructor_newInstance(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getLength_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getLength(void *, void *);
    p[0].i = java_lang_reflect_Array_getLength(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_newArray_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Array_newArray(void *, void *, long);
    p[0].p = java_lang_reflect_Array_newArray(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_multiNewArray_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Array_multiNewArray(void *, void *, void *);
    p[0].p = java_lang_reflect_Array_multiNewArray(NULL, p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_get_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Field_get(void *, void *);
    p[0].p = java_lang_reflect_Field_get(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_set_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_set(void *, void *, void *);
    java_lang_reflect_Field_set(p[0].p, p[1].p, p[2].p);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getBoolean_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getBoolean(void *, void *);
    p[0].i = java_lang_reflect_Field_getBoolean(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setBoolean_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setBoolean(void *, void *, long);
    java_lang_reflect_Field_setBoolean(p[0].p, p[1].p, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getByte_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getByte(void *, void *);
    p[0].i = java_lang_reflect_Field_getByte(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setByte_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setByte(void *, void *, long);
    java_lang_reflect_Field_setByte(p[0].p, p[1].p, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getChar_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getChar(void *, void *);
    p[0].i = java_lang_reflect_Field_getChar(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setChar_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setChar(void *, void *, long);
    java_lang_reflect_Field_setChar(p[0].p, p[1].p, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getShort_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getShort(void *, void *);
    p[0].i = java_lang_reflect_Field_getShort(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setShort_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setShort(void *, void *, long);
    java_lang_reflect_Field_setShort(p[0].p, p[1].p, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getInt_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Field_getInt(void *, void *);
    p[0].i = java_lang_reflect_Field_getInt(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setInt_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setInt(void *, void *, long);
    java_lang_reflect_Field_setInt(p[0].p, p[1].p, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getLong_stub(stack_item *p, struct execenv *ee)
{
    extern int64_t java_lang_reflect_Field_getLong(void *, void *);
    Java8 temporary;
    SET_INT64(temporary, p, java_lang_reflect_Field_getLong(p[0].p, p[1].p));
    return p + 2;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setLong_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setLong(void *, void *, int64_t);
    Java8 temporary;
    java_lang_reflect_Field_setLong(p[0].p, p[1].p, GET_INT64(temporary, p + 2));
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getFloat_stub(stack_item *p, struct execenv *ee)
{
    extern float java_lang_reflect_Field_getFloat(void *, void *);
    p[0].f = java_lang_reflect_Field_getFloat(p[0].p, p[1].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setFloat_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setFloat(void *, void *, float);
    java_lang_reflect_Field_setFloat(p[0].p, p[1].p, p[2].f);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_getDouble_stub(stack_item *p, struct execenv *ee)
{
    extern double java_lang_reflect_Field_getDouble(void *, void *);
    Java8 temporary;
    SET_DOUBLE(temporary, p, java_lang_reflect_Field_getDouble(p[0].p, p[1].p));
    return p + 2;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Field_setDouble_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Field_setDouble(void *, void *, double);
    Java8 temporary;
    java_lang_reflect_Field_setDouble(p[0].p, p[1].p, GET_DOUBLE(temporary, p + 2));
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_get_stub(stack_item *p, struct execenv *ee)
{
    extern void * java_lang_reflect_Array_get(void *, void *, long);
    p[0].p = java_lang_reflect_Array_get(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_set_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_set(void *, void *, long, void *);
    java_lang_reflect_Array_set(NULL, p[0].p, p[1].i, p[2].p);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getBoolean_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getBoolean(void *, void *, long);
    p[0].i = java_lang_reflect_Array_getBoolean(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setBoolean_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setBoolean(void *, void *, long, long);
    java_lang_reflect_Array_setBoolean(NULL, p[0].p, p[1].i, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getByte_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getByte(void *, void *, long);
    p[0].i = java_lang_reflect_Array_getByte(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setByte_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setByte(void *, void *, long, long);
    java_lang_reflect_Array_setByte(NULL, p[0].p, p[1].i, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getChar_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getChar(void *, void *, long);
    p[0].i = java_lang_reflect_Array_getChar(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setChar_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setChar(void *, void *, long, long);
    java_lang_reflect_Array_setChar(NULL, p[0].p, p[1].i, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getShort_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getShort(void *, void *, long);
    p[0].i = java_lang_reflect_Array_getShort(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setShort_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setShort(void *, void *, long, long);
    java_lang_reflect_Array_setShort(NULL, p[0].p, p[1].i, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getInt_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_reflect_Array_getInt(void *, void *, long);
    p[0].i = java_lang_reflect_Array_getInt(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setInt_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setInt(void *, void *, long, long);
    java_lang_reflect_Array_setInt(NULL, p[0].p, p[1].i, p[2].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getLong_stub(stack_item *p, struct execenv *ee)
{
    extern int64_t java_lang_reflect_Array_getLong(void *, void *, long);
    Java8 temporary;
    SET_INT64(temporary, p, java_lang_reflect_Array_getLong(NULL, p[0].p, p[1].i));
    return p + 2;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setLong_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setLong(void *, void *, long, int64_t);
    Java8 temporary;
    java_lang_reflect_Array_setLong(NULL, p[0].p, p[1].i, GET_INT64(temporary, p + 2));
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getFloat_stub(stack_item *p, struct execenv *ee)
{
    extern float java_lang_reflect_Array_getFloat(void *, void *, long);
    p[0].f = java_lang_reflect_Array_getFloat(NULL, p[0].p, p[1].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setFloat_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setFloat(void *, void *, long, float);
    java_lang_reflect_Array_setFloat(NULL, p[0].p, p[1].i, p[2].f);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_getDouble_stub(stack_item *p, struct execenv *ee)
{
    extern double java_lang_reflect_Array_getDouble(void *, void *, long);
    Java8 temporary;
    SET_DOUBLE(temporary, p, java_lang_reflect_Array_getDouble(NULL, p[0].p, p[1].i));
    return p + 2;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_reflect_Array_setDouble_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_reflect_Array_setDouble(void *, void *, long, double);
    Java8 temporary;
    java_lang_reflect_Array_setDouble(NULL, p[0].p, p[1].i, GET_DOUBLE(temporary, p + 2));
    return p;
}
