#import <AppKit/AppKit.h>
#include "jni.h"
extern NSEvent *makeWrappedPaintEvent(JNIEnv *, id, int, NSRect);
@interface NSObject (PaintCoverTest)
- (jobject)javaEvent;
- (void)releaseJavaEvent:(JNIEnv *)env;
- (BOOL)isSynthetic;
@end
@interface PaintPeerProbe : NSObject { jobject _peer; }
- (id)initWithPeer:(jobject)peer;
- (jobject)peer;
@end
@implementation PaintPeerProbe
- (id)initWithPeer:(jobject)peer { self = [super init]; _peer = peer; return self; }
- (jobject)peer { return _peer; }
@end
JNIEXPORT jobject JNICALL Java_PaintCover115_make(JNIEnv *env, jclass cls, jobject source, jint type, jfloat x, jfloat y, jfloat w, jfloat h)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RCanvasPeer");
    jobject peer, result;
    jfieldID field;
    NSEvent *event;
    id view;
    jobjectArray root;
    if (!peerClass) { [pool release]; return NULL; }
    peer = (*env)->AllocObject(env, peerClass);
    field = (*env)->GetFieldID(env, peerClass, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) { [pool release]; return NULL; }
    (*env)->SetObjectField(env, peer, field, source);
    view = [[PaintPeerProbe alloc] initWithPeer:peer];
    event = makeWrappedPaintEvent(env, view, type, NSMakeRect(x, y, w, h));
    if (![event isSynthetic])
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "paint cover not synthetic");
    /* JNI 1.1 has no NewLocalRef; array access gives an independent local root. */
    root = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/Object"), [event javaEvent]);
    result = (*env)->GetObjectArrayElement(env, root, 0);
    [event releaseJavaEvent:env];
    [view release];
    [pool release];
    return result;
}
