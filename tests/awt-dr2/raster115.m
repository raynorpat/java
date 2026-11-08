#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (RasterTest)
+ (id)appkitProxy;
@end
@interface RasterProbe : NSObject
+ (int)pixelForView:(int)view x:(int)x y:(int)y;
@end
@implementation RasterProbe
+ (int)pixelForView:(int)view x:(int)x y:(int)y
{
    NSColor *color;
    float red, green, blue, alpha;
    [(NSView *)view lockFocus];
    color = [NSReadPixel(NSMakePoint(x, y)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    [color getRed:&red green:&green blue:&blue alpha:&alpha];
    [(NSView *)view unlockFocus];
    return ((int)(red * 255 + 0.5) << 16) | ((int)(green * 255 + 0.5) << 8) | (int)(blue * 255 + 0.5);
}
@end
JNIEXPORT jint JNICALL Java_Raster115_pixel(JNIEnv *env, jclass cls, jobject frame, jint x, jint y)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    int view = (*env)->GetIntField(env, peer, field);
    int result = [[RasterProbe appkitProxy] pixelForView:view x:x y:y];
    [pool release];
    return result;
}
