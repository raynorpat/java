#include "jni.h"
JNIEXPORT void JNICALL Java_DispatcherEntry115_callRun(JNIEnv *env, jclass cls)
{
    jclass dispatcher = (*env)->FindClass(env, "com/apple/rhapsody/awt/RDispatcher");
    jobject object;
    jmethodID method;
    if (!dispatcher) return;
    /* Its constructor starts an obsolete Java worker; invoke the native entry
     * synchronously so the donor's exception can be checked by the caller. */
    object = (*env)->AllocObject(env, dispatcher);
    method = (*env)->GetMethodID(env, dispatcher, "run", "()V");
    if (object && method) (*env)->CallVoidMethod(env, object, method);
    (*env)->DeleteLocalRef(env, object);
    (*env)->DeleteLocalRef(env, dispatcher);
}
