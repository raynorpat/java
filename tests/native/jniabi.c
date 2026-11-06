#include <jni.h>
#include <string.h>
#include <stdio.h>
#include "jnidefined.h"
#define CHECK(test) do { if (!(test)) return (*e)->NewStringUTF(e, "FAILED " #test); } while (0)
static jint JNICALL registered(JNIEnv *e, jclass c, jint value) { return value + 100; }
JNIEXPORT jstring JNICALL Java_JniAbi_check(JNIEnv *e, jclass cls, jobject object)
{
    jfieldID id;
    jmethodID method;
    jvalue args[3];
    jobject global, local, value;
    jstring string;
    jchar chars[4] = { 'a', 0, 0x8123, 'z' };
    const jchar *wide;
    const char *utf;
    jboolean copy;
    jintArray array;
    jint data[4] = { 1, 2, 3, 4 }, *elements;
    jobjectArray objects;
    jclass stringClass, exceptionClass;
    JNINativeMethod native;
    JavaVM *vm;
    JNIEnv *same;
    jclass defined;
    CHECK((*e)->GetVersion(e) == 0x10001);
    CHECK((*e)->GetJavaVM(e, &vm) == 0);
    CHECK((*vm)->AttachCurrentThread(vm, &same, 0) == 0 && same == e);
    global = (*e)->NewGlobalRef(e, object);
    CHECK((*e)->IsSameObject(e, global, object));
    CHECK((*e)->IsInstanceOf(e, 0, cls));
    local = (*e)->GetObjectClass(e, global);
    CHECK((*e)->IsSameObject(e, local, cls));
    (*e)->DeleteLocalRef(e, local);
    CHECK((*e)->MonitorEnter(e, global) == 0 && (*e)->MonitorExit(e, global) == 0);
    defined = (*e)->DefineClass(e, "JniDefined", NULL, (const jbyte *)definedClass, sizeof(definedClass));
    CHECK(defined != 0);
    method = (*e)->GetStaticMethodID(e, defined, "answer", "()I");
    CHECK((*e)->CallStaticIntMethod(e, defined, method) == 73);
#define FIELD(Name, Type, FieldName, Sig, Expected, New) \
    id = (*e)->GetFieldID(e, cls, FieldName, Sig); CHECK(id != 0); \
    CHECK((*e)->Get##Name##Field(e, object, id) == (Type)(Expected)); \
    (*e)->Set##Name##Field(e, object, id, (Type)(New)); \
    CHECK((*e)->Get##Name##Field(e, object, id) == (Type)(New));
    FIELD(Boolean, jboolean, "z", "Z", 1, 0)
    FIELD(Byte, jbyte, "b", "B", -11, -20)
    FIELD(Char, jchar, "c", "C", 0x8123, 0xffff)
    FIELD(Short, jshort, "s", "S", -1234, -30000)
    FIELD(Int, jint, "i", "I", 1234567, -456)
    FIELD(Long, jlong, "j", "J", ((jlong)0x12345678 << 32) | 0x9abcdef0, -3)
    FIELD(Float, jfloat, "f", "F", 1.25, 5.5)
    FIELD(Double, jdouble, "d", "D", -2.5, 8.25)
    id = (*e)->GetFieldID(e, cls, "o", "Ljava/lang/Object;");
    value = (*e)->GetObjectField(e, object, id);
    CHECK(value != 0);
    (*e)->SetObjectField(e, object, id, global);
    CHECK((*e)->IsSameObject(e, (*e)->GetObjectField(e, object, id), object));
    id = (*e)->GetStaticFieldID(e, cls, "sj", "J");
    CHECK((*e)->GetStaticLongField(e, cls, id) == (((jlong)0x11223344 << 32) | 0x55667788));
    (*e)->SetStaticLongField(e, cls, id, -17);
    CHECK((*e)->GetStaticLongField(e, cls, id) == -17);
    id = (*e)->GetStaticFieldID(e, cls, "sd", "D");
    CHECK((*e)->GetStaticDoubleField(e, cls, id) == 8.5);
    (*e)->SetStaticDoubleField(e, cls, id, 9.5);
    CHECK((*e)->GetStaticDoubleField(e, cls, id) == 9.5);
    method = (*e)->GetMethodID(e, cls, "number", "(I)I");
    CHECK((*e)->CallIntMethod(e, object, method, 6) == 9);
    args[0].i = 7;
    CHECK((*e)->CallNonvirtualIntMethodA(e, object, cls, method, args) == 10);
    method = (*e)->GetMethodID(e, cls, "wideCallback", "(JDF)J");
    CHECK((*e)->CallLongMethod(e, object, method, (jlong)0x1122334455, 2.5, 3.5) == (jlong)0x112233445a);
    args[0].j = (jlong)0x1122334455; args[1].d = 2.5; args[2].f = 3.5f;
    CHECK((*e)->CallLongMethodA(e, object, method, args) == (jlong)0x112233445a);
    method = (*e)->GetMethodID(e, cls, "realCallback", "(DF)D");
    CHECK((*e)->CallDoubleMethod(e, object, method, 2.5, 3.5) == 6.0);
    method = (*e)->GetStaticMethodID(e, cls, "staticCallback", "(I)I");
    CHECK((*e)->CallStaticIntMethod(e, cls, method, 6) == 42);
    string = (*e)->NewString(e, chars, 4);
    CHECK((*e)->GetStringLength(e, string) == 4 && (*e)->GetStringUTFLength(e, string) == 7);
    wide = (*e)->GetStringChars(e, string, &copy);
    CHECK(wide && !memcmp(wide, chars, sizeof(chars)));
    utf = (*e)->GetStringUTFChars(e, string, &copy);
    CHECK(utf && copy && !memcmp(utf, "a\300\200\350\204\243z", 8));
    value = (*e)->NewStringUTF(e, utf);
    CHECK((*e)->GetStringLength(e, value) == 4);
    (*e)->ReleaseStringUTFChars(e, string, utf);
    (*e)->ReleaseStringChars(e, string, wide);
    array = (*e)->NewIntArray(e, 4);
    (*e)->SetIntArrayRegion(e, array, 0, 4, data);
    elements = (*e)->GetIntArrayElements(e, array, &copy);
    CHECK(elements && elements[3] == 4);
    elements[2] = 17;
    local = (*e)->FindClass(e, "java/lang/System");
    method = (*e)->GetStaticMethodID(e, local, "gc", "()V");
    (*e)->CallStaticVoidMethod(e, local, method);
    CHECK(elements[2] == 17);
    (*e)->ReleaseIntArrayElements(e, array, elements, 0);
    (*e)->GetIntArrayRegion(e, array, 0, 4, data);
    CHECK(data[2] == 17);
#define ARRAY(Name, Type, Value) \
    { Type values[2] = { 0, (Type)(Value) }; Type *raw; \
      Type##Array a = (*e)->New##Name##Array(e, 2); \
      (*e)->Set##Name##ArrayRegion(e, a, 0, 2, values); \
      raw = (*e)->Get##Name##ArrayElements(e, a, NULL); \
      CHECK(raw && raw[1] == (Type)(Value)); \
      (*e)->Release##Name##ArrayElements(e, a, raw, 0); \
      values[1] = 0; (*e)->Get##Name##ArrayRegion(e, a, 0, 2, values); \
      CHECK(values[1] == (Type)(Value)); }
    ARRAY(Boolean, jboolean, 1)
    ARRAY(Byte, jbyte, -123)
    ARRAY(Char, jchar, 0x8123)
    ARRAY(Short, jshort, -30000)
    ARRAY(Long, jlong, ((jlong)0x12345678 << 32) | 0x9abcdef0)
    ARRAY(Float, jfloat, 2.25)
    ARRAY(Double, jdouble, -8.5)
    stringClass = (*e)->FindClass(e, "java/lang/String");
    objects = (*e)->NewObjectArray(e, 3, stringClass, string);
    CHECK((*e)->GetArrayLength(e, objects) == 3);
    value = (*e)->GetObjectArrayElement(e, objects, 2);
    CHECK((*e)->IsSameObject(e, value, string));
    (*e)->SetObjectArrayElement(e, objects, 1, object);
    CHECK((*e)->ExceptionOccurred(e) != 0);
    (*e)->ExceptionClear(e);
    (*e)->GetIntArrayRegion(e, array, 4, 1, data);
    CHECK((*e)->ExceptionOccurred(e) != 0);
    (*e)->ExceptionClear(e);
    method = (*e)->GetStaticMethodID(e, cls, "throwsCallback", "()V");
    (*e)->CallStaticVoidMethod(e, cls, method);
    value = (*e)->ExceptionOccurred(e);
    CHECK(value != 0);
    (*e)->ExceptionClear(e);
    exceptionClass = (*e)->FindClass(e, "java/lang/IllegalArgumentException");
    CHECK((*e)->IsInstanceOf(e, value, exceptionClass));
    native.name = "registered"; native.signature = "(I)I"; native.fnPtr = registered;
    CHECK((*e)->RegisterNatives(e, cls, &native, 1) == 0);
    (*e)->DeleteGlobalRef(e, global);
    return (*e)->NewStringUTF(e, "abi ok");
}
JNIEXPORT jlong JNICALL Java_JniAbi_wide(JNIEnv *e, jclass cls, jlong a, jdouble b, jfloat c, jobject d, jint n)
{ return a + (jlong)b + (jlong)c + n + (d != 0); }
JNIEXPORT jdouble JNICALL Java_JniAbi_real(JNIEnv *e, jclass cls, jdouble a, jfloat b, jlong c)
{ return a + b + c; }
JNIEXPORT jint JNICALL Java_JniAbi_over__I(JNIEnv *e, jclass cls, jint n) { return n + 1; }
JNIEXPORT jint JNICALL Java_JniAbi_over__Ljava_lang_String_2(JNIEnv *e, jclass cls, jstring s)
{ return (*e)->GetStringLength(e, s); }
JNIEXPORT jint JNICALL Java_JniAbi_under_1score(JNIEnv *e, jclass cls, jint n) { return n * 2; }
