/* DR2 libawt.A.dylib: 54013e88, javaOtherEventToNSEvent 54006034. */
#import "awt_Application.h"
#import "awt_Event.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_REventQueue_postSyntheticOtherEvent(JNIEnv *env, jobject queue, jobject event)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSEvent *cover = [SyntheticEvent coverEventForJavaEvent:event inEnv:env];
        [[NSApp appkitProxy] postEvent:[cover appkitProxy] atStart:NO];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
