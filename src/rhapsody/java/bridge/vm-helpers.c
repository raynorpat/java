/* Exported DR2 VM helpers, libObjCJava 0x53d0c5ec and 0x53d0c87c-0x53d0ca1c. */
#include "oobj.h"
#include "interpreter.h"
#include "javaString.h"
#include "typecodes.h"
#include <string.h>

struct fieldblock *JAVAFieldLookup(ExecEnv *env, ClassClass *clazz,
    const char *name, const char *signature)
{
    unsigned i;
    if (!clazz) {
        SignalError(EE(), "java/lang/NullPointerException", "Null class specified");
        return NULL;
    }
    makeslottable(clazz);
    for (i = 0; i < cbSlotTableSize(clazz); ++i) {
        struct fieldblock *field = cbSlotTable(clazz)[i];
        if (field && !strcmp(fieldname(field), name)
            && (!signature || !strcmp(fieldsig(field), signature))) return field;
    }
    return NULL;
}

Hjava_lang_String *JAVANewString(ExecEnv *env, const unicode *characters, unsigned count)
{
    HArrayOfChar *array = (HArrayOfChar *)ArrayAlloc(T_CHAR, count);
    if (!array) return NULL;
    memcpy(unhand(array)->body, characters, count * sizeof(unicode));
    return (Hjava_lang_String *)execute_java_constructor(env, NULL,
        classJavaLangString, "([C)", array);
}

HObject *JAVANewScalarArray(ExecEnv *env, unsigned length, const char *signature)
{
    int type;
    switch (*signature) {
    case 'Z': type = T_BOOLEAN; break;
    case 'B': type = T_BYTE; break;
    case 'C': type = T_CHAR; break;
    case 'S': type = T_SHORT; break;
    case 'I': type = T_INT; break;
    case 'J': type = T_LONG; break;
    case 'F': type = T_FLOAT; break;
    case 'D': type = T_DOUBLE; break;
    default:
        SignalError(EE(), "java/lang/IllegalArgumentException", "Invalid array type.");
        return NULL;
    }
    return ArrayAlloc(type, length);
}

HArrayOfObject *JAVANewObjectArray(ExecEnv *env, unsigned length, ClassClass *clazz)
{
    HArrayOfObject *array = (HArrayOfObject *)ArrayAlloc(T_CLASS, length);
    if (array) unhand(array)->body[length] = (HObject *)clazz;
    else SignalError(EE(), "java/lang/OutOfMemoryException", NULL);
    return array;
}

bool_t JAVAIsSubclassOf(ExecEnv *env, ClassClass *clazz, ClassClass *superclass)
{
    for (; clazz; clazz = cbSuperclass(clazz))
        if (clazz == superclass) return TRUE;
    return FALSE;
}
