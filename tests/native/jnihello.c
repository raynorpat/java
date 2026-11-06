#include <string.h>
#include "jni.h"

JNIEXPORT jint JNICALL Java_JniHello_add(JNIEnv *env, jclass type, jint a, jint b)
{
    return a + b;
}

JNIEXPORT jstring JNICALL Java_JniHello_greet(JNIEnv *env, jclass type, jstring name)
{
    const char *text = (*env)->GetStringUTFChars(env, name, NULL);
    char buffer[64];
    if (text == NULL) return NULL;
    strcpy(buffer, "hello ");
    strncat(buffer, text, sizeof(buffer) - strlen(buffer) - 1);
    (*env)->ReleaseStringUTFChars(env, name, text);
    return (*env)->NewStringUTF(env, buffer);
}

JNIEXPORT jint JNICALL Java_JniHello_callback(JNIEnv *env, jclass type, jint value)
{
    jmethodID method = (*env)->GetStaticMethodID(env, type, "twice", "(I)I");
    if (method == NULL) return 0;
    return (*env)->CallStaticIntMethod(env, type, method, value);
}

JNIEXPORT void JNICALL Java_JniHello_fail(JNIEnv *env, jclass type)
{
    jclass error = (*env)->FindClass(env, "java/lang/IllegalStateException");
    if (error != NULL) (*env)->ThrowNew(env, error, "from C");
}
