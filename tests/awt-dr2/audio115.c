#include "jni.h"
static jobject device(JNIEnv *env, jclass *cls)
{
    jfieldID field;
    *cls = (*env)->FindClass(env, "sun/audio/AudioDevice");
    if (!*cls) return NULL;
    field = (*env)->GetStaticFieldID(env, *cls, "device", "Lsun/audio/AudioDevice;");
    return field ? (*env)->GetStaticObjectField(env, *cls, field) : NULL;
}
JNIEXPORT jint JNICALL Java_Audio115_open(JNIEnv *env, jclass caller)
{
    jclass cls; jobject object = device(env, &cls);
    jmethodID method = object ? (*env)->GetMethodID(env, cls, "audioOpen", "()I") : NULL;
    return method ? (*env)->CallIntMethod(env, object, method) : -1;
}
JNIEXPORT void JNICALL Java_Audio115_close(JNIEnv *env, jclass caller)
{
    jclass cls; jobject object = device(env, &cls);
    jmethodID method = object ? (*env)->GetMethodID(env, cls, "audioClose", "()V") : NULL;
    if (method) (*env)->CallVoidMethod(env, object, method);
}
JNIEXPORT void JNICALL Java_Audio115_write(JNIEnv *env, jclass caller, jbyteArray samples, jint length)
{
    jclass cls; jobject object = device(env, &cls);
    jmethodID method = object ? (*env)->GetMethodID(env, cls, "audioWrite", "([BI)V") : NULL;
    if (method) (*env)->CallVoidMethod(env, object, method, samples, length);
}
