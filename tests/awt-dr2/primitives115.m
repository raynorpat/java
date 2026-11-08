#import <AppKit/AppKit.h>
#include "jni.h"
#import "../../src/rhapsody/sun/awt/awt_Graphics.h"
static int liveGraphics;
@interface GraphicsLifetimeProbe : AwtScreenGraphics
@end
@implementation GraphicsLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveGraphics++; return value; }
- (void)dealloc { liveGraphics--; [super dealloc]; }
@end
@interface NSObject (PrimitiveTest)
+ (id)appkitProxy;
@end
@interface PrimitiveProbe : NSObject
+ (int)checksum:(int)view;
+ (void)track;
+ (int)live;
@end
@implementation PrimitiveProbe
+ (void)track { [GraphicsLifetimeProbe poseAsClass:[AwtScreenGraphics class]]; }
+ (int)live { return liveGraphics; }
+ (int)checksum:(int)view
{
    NSBitmapImageRep *bitmap;
    unsigned char *data;
    unsigned int hash = 0;
    int row, column, stride, width;
    [(NSView *)view lockFocus];
    bitmap = [[NSBitmapImageRep alloc] initWithFocusedViewRect:NSMakeRect(5, 25, 150, 100)];
    [(NSView *)view unlockFocus];
    data = [bitmap bitmapData];
    stride = [bitmap bytesPerRow];
    width = ([bitmap pixelsWide] * [bitmap bitsPerPixel] + 7) / 8;
    for (row = 0; row < [bitmap pixelsHigh]; row++)
        for (column = 0; column < width; column++) hash = hash * 31 + data[row * stride + column];
    [bitmap release];
    return hash;
}
@end
JNIEXPORT void JNICALL Java_Primitives115_track(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[PrimitiveProbe appkitProxy] track]; [pool release]; }
JNIEXPORT jint JNICALL Java_Primitives115_live(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; int result = [[PrimitiveProbe appkitProxy] live]; [pool release]; return result; }
JNIEXPORT jint JNICALL Java_Primitives115_checksum(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    int view = (*env)->GetIntField(env, peer, field);
    int result = [[PrimitiveProbe appkitProxy] checksum:view];
    [pool release];
    return result;
}
