/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* JDK 1.1.5 string helpers recovered from javai_g.dll1004AEE0--1004B45F.
 * See tests/native/string-helpers115-evidence.md. */
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "utf.h"

static char *platform_encoding;

/* Sun1004B04E caches System.getProperty("file.encoding", "8859_1"). */
static void initialize_platform_encoding(void)
{
    ExecEnv *ee;
    HString *fallback, *key, *value;
    ClassClass *system;
    if (platform_encoding != NULL) return;
    ee = EE();
    fallback = makeJavaString("8859_1", 6);
    key = makeJavaString("file.encoding", 13);
    system = FindClass(ee, "java/lang/System", TRUE);
    value = (HString *)execute_java_static_method(ee, system, "getProperty",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", key, fallback);
    if (exceptionOccurred(ee)) {
        exceptionClear(ee);
        platform_encoding = "8859_1";
    } else {
        platform_encoding = allocCString(value);
    }
}

HString *makeJavaStringFromPlatformCString(char *bytes, int length)
{
    ExecEnv *ee = EE();
    HString *encoding;
    HObject *converter;
    HArrayOfByte *input;
    HArrayOfChar *output;
    ClassClass *cb;
    int count;
    extern HArrayOfByte *MakeByteString(char *, long);
    initialize_platform_encoding();
    if (!strcmp(platform_encoding, "8859_1")) return makeJavaString(bytes, length);
    encoding = makeJavaString(platform_encoding, strlen(platform_encoding));
    cb = FindClass(ee, "sun/io/ByteToCharConverter", TRUE);
    converter = (HObject *)execute_java_static_method(ee, cb, "getConverter",
        "(Ljava/lang/String;)Lsun/io/ByteToCharConverter;", encoding);
    if (!exceptionOccurred(ee)) {
        input = MakeByteString(bytes, length);
        output = (HArrayOfChar *)ArrayAlloc(T_CHAR, length);
        count = execute_java_dynamic_method(ee, converter, "convert", "([BII[CII)I",
            input, 0, length, output, 0, length);
        if (!exceptionOccurred(ee))
            return (HString *)execute_java_constructor(ee, "java/lang/String", NULL,
                "([CII)", output, 0, count);
    }
    exceptionClear(ee);
    return makeJavaString(bytes, length);
}

char *makePlatformCString(HString *string)
{
    ExecEnv *ee = EE();
    HString *encoding;
    HObject *converter;
    HArrayOfByte *output;
    ClassClass *cb;
    int length, capacity, count;
    initialize_platform_encoding();
    if (!strcmp(platform_encoding, "8859_1")) return makeCString(string);
    length = javaStringLength(string);
    encoding = makeJavaString(platform_encoding, strlen(platform_encoding));
    cb = FindClass(ee, "sun/io/CharToByteConverter", TRUE);
    converter = (HObject *)execute_java_static_method(ee, cb, "getConverter",
        "(Ljava/lang/String;)Lsun/io/CharToByteConverter;", encoding);
    if (!exceptionOccurred(ee)) {
        capacity = length * MB_CUR_MAX + 1;
        output = (HArrayOfByte *)ArrayAlloc(T_BYTE, capacity);
        count = execute_java_dynamic_method(ee, converter, "convert", "([CII[BII)I",
            unhand(string)->value, unhand(string)->offset,
            unhand(string)->offset + length, output, 0, capacity);
        if (!exceptionOccurred(ee)) {
            unhand(output)->body[count] = 0;
            return unhand(output)->body;
        }
    }
    exceptionClear(ee);
    return makeCString(string);
}

int javaStringUTFLength(HString *string)
{
    Classjava_lang_String *body;
    if (!string || !(body = unhand(string)) || !body->value) return -1;
    return unicode2utfstrlen(unhand(body->value)->body + body->offset, body->count) - 1;
}

char *javaString2UTF(HString *string, char *buffer, int length)
{
    Classjava_lang_String *body;
    if (string && (body = unhand(string)) && body->value)
        return unicode2utf(unhand(body->value)->body + body->offset, body->count,
                           buffer, length);
    if (length) buffer[0] = 0;
    return buffer;
}

HString *makeJavaStringUTF(char *utf)
{
    int length = utfstrlen(utf);
    HArrayOfChar *array = (HArrayOfChar *)ArrayAlloc(T_CHAR, length);
    if (!array) {
        SignalError(NULL, "java/lang/OutOfMemoryError", NULL);
        return NULL;
    }
    utf2unicode(utf, unhand(array)->body, length, &length);
    return (HString *)execute_java_constructor(EE(), NULL, classJavaLangString,
                                               "([C)", array);
}
