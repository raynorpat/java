/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Primary evidence: javai_g.dll 1000BCBE, 1001272A, 1000ED18, 1000BA4A.
 * Names and primitive metadata follow Sun's shipped interpreter.h/oobj.h.
 * See tests/vm/PRIMITIVE-EVIDENCE.md for transitional layout dependencies. */

#include "tree.h"
#include "javaString.h"
#include "exceptions.h"

extern ClassClass *allocClassClass(void);

static struct {
    char *name;
    char signature;
    unsigned char typecode;
    unsigned char slotsize;
    unsigned char elementsize;
    ClassClass *clazz;
} primitive_classes[] = {
    {"void",    'V', T_VOID,    0, 0, NULL},
    {"boolean", 'Z', T_BOOLEAN, 4, 1, NULL},
    {"byte",    'B', T_BYTE,    4, 1, NULL},
    {"char",    'C', T_CHAR,    4, 2, NULL},
    {"short",   'S', T_SHORT,   4, 2, NULL},
    {"int",     'I', T_INT,     4, 4, NULL},
    {"long",    'J', T_LONG,    8, 8, NULL},
    {"float",   'F', T_FLOAT,   4, 4, NULL},
    {"double",  'D', T_DOUBLE,  8, 8, NULL}
};

ClassClass *
createPrimitiveClass(char *name, char sig, unsigned char typecode,
                     unsigned char slotsize, unsigned char elementsize)
{
    ClassClass *cb = allocClassClass();

    /* deviation: report allocation failure instead of the debug VM's assert. */
    if (cb == NULL) {
        SignalError(0, JAVAPKG "OutOfMemoryError", NULL);
        return NULL;
    }
    cbMajorVersion(cb) = JAVA_VERSION;
    cbMinorVersion(cb) = JAVA_MINOR_VERSION;
    cbName(cb) = strdup(name);
    if (!cbName(cb)) {
        SignalError(0, JAVAPKG "OutOfMemoryError", NULL);
        return NULL;
    }
    cbSuperName(cb) = JAVAPKG "Object";
    cbConstantPool(cb) = NULL;
    cbConstantPoolCount(cb) = 0;
    cbLoader(cb) = NULL;
    cbAccess(cb) = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;
    CCSet(cb, Primitive);
    cbTypeSig(cb) = sig;
    cbTypeCode(cb) = typecode;
    cbSlotSize(cb) = slotsize;
    cbElementSize(cb) = elementsize;
    /* Primitive classes are sticky in the primary VM. */
    cbFlags(cb) |= CCF_IsSticky;
    lock_classes();
    AddBinClass(cb);
    unlock_classes();
    return cb;
}

ClassClass *
FindPrimitiveClass(char *name)
{
    unsigned int i;
    ClassClass *cb;
    char *detail = NULL;

    for (i = 0; i < sizeof(primitive_classes) / sizeof(primitive_classes[0]); i++) {
        if (strcmp(name, primitive_classes[i].name) == 0) {
            cb = primitive_classes[i].clazz;
            if (cb == NULL) {
                cb = createPrimitiveClass(primitive_classes[i].name,
                    primitive_classes[i].signature, primitive_classes[i].typecode,
                    primitive_classes[i].slotsize, primitive_classes[i].elementsize);
                if (cb == NULL)
                    return NULL;
                if (InitializeClass(cb, &detail) != NULL)
                    return NULL;
                if (ResolveClass(cb, &detail) != NULL)
                    return NULL;
                primitive_classes[i].clazz = cb;
            }
            return cb;
        }
    }
    return NULL;
}

ClassClass *
java_lang_Class_getPrimitiveClass(ClassClass *unused, HString *name)
{
    char buffer[256];
    ClassClass *cb;

    if (name == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", NULL);
        return NULL;
    }
    javaString2CString(name, buffer, sizeof(buffer));
    cb = FindPrimitiveClass(buffer);
    return cb == NULL ? NULL : cbHandle(cb);
}

long
java_lang_Class_isPrimitive(ClassClass *self)
{
    return CCIs(self, Primitive);
}
