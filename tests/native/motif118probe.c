/* Test-only JNI access to real peer fields; never part of libawt. */
#include "StubPreamble.h"
#include "sun_awt_motif_MButtonPeer.h"

/* Legacy stubs receive VM handles; JNI references are opaque table indices. */
JAVA_PUBLIC_API(stack_item*)
Java_sun_awt_im_InputContextTest115_swapTarget_stub(stack_item *p, struct execenv *ee)
{
    struct Hsun_awt_motif_MButtonPeer *button = p[0].p;
    struct Hjava_awt_Component *old = unhand(button)->target;
    unhand(button)->target = p[1].p;
    p[0].p = old;
    return p + 1;
}

JNIEXPORT void JNICALL Java_sun_awt_im_InputContextTest115_bind
  (JNIEnv *env, jclass owner, jobject context, jobject method, jboolean failed)
{
    jclass type = (*env)->GetObjectClass(env, context);
    jfieldID value = (*env)->GetFieldID(env, type, "inputMethod", "Lsun/awt/im/InputMethod;");
    jfieldID failure = (*env)->GetFieldID(env, type, "inputMethodCreationFailed", "Z");
    if (!value || !failure) return;
    (*env)->SetObjectField(env, context, value, method);
    (*env)->SetBooleanField(env, context, failure, failed);
}

JNIEXPORT void JNICALL Java_sun_awt_im_InputContextTest115_verifyLayout
  (JNIEnv *env, jclass owner)
{
    jclass button_class = (*env)->FindClass(env, "sun/awt/motif/MButtonPeer");
    jclass canvas_class = (*env)->FindClass(env, "java/awt/Canvas");
    jobject button, canvas, actual;
    jmethodID swap;
    jfieldID target;
    if (!button_class || !canvas_class) return;
    target = (*env)->GetFieldID(env, button_class, "target", "Ljava/awt/Component;");
    if (!target) return;
    button = (*env)->AllocObject(env, button_class);
    canvas = (*env)->AllocObject(env, canvas_class);
    if (!button || !canvas) return;
    swap = (*env)->GetStaticMethodID(env, owner, "swapTarget", "(Ljava/lang/Object;Ljava/awt/Component;)Ljava/awt/Component;");
    if (!swap) return;
    (*env)->CallStaticObjectMethod(env, owner, swap, button, canvas);
    actual = (*env)->GetObjectField(env, button, target);
    if (!(*env)->IsSameObject(env, canvas, actual)) {
        (*env)->ThrowNew(env, (*env)->FindClass(env,"java/lang/RuntimeException"), "generated target field offset");
        return;
    }
    (*env)->SetObjectField(env, button, target, NULL);
    if ((*env)->CallStaticObjectMethod(env, owner, swap, button, canvas))
        (*env)->ThrowNew(env, (*env)->FindClass(env,"java/lang/RuntimeException"), "runtime target field offset");
}
