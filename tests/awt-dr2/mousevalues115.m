#import <AppKit/AppKit.h>
#include <mach-o/dyld.h>
#include "jni.h"

/* Isolate conversion from window creation and hit-testing (tested separately). */
@interface MouseConversionView : NSObject { BOOL _inside; }
- (id)initInside:(BOOL)inside;
- (NSPoint)convertPoint:(NSPoint)point fromView:(id)view;
- (id)window;
- (id)deepestViewUnderLocation:(NSPoint)point dispatchingEventsOfType:(int)type;
@end
@implementation MouseConversionView
- (id)initInside:(BOOL)inside { self = [super init]; _inside = inside; return self; }
- (NSPoint)convertPoint:(NSPoint)point fromView:(id)view { return NSMakePoint(point.x - 7.5, point.y + 2.5); }
- (id)window { return self; }
- (id)deepestViewUnderLocation:(NSPoint)point dispatchingEventsOfType:(int)type { return _inside ? self : nil; }
@end
JNIEXPORT jobject JNICALL Java_MouseValues115_convert(JNIEnv *env, jclass cls, jobject source, jint type, jint flags, jboolean inside)
{
    typedef jobject (*Convert)(JNIEnv *, NSEvent *, jobject, id);
    id pool = [[NSAutoreleasePool alloc] init];
    NSEvent *event;
    id view;
    jobject result;
    Convert convert;
    if (!NSIsSymbolNameDefined("_nsMouseEventToJavaEvent")) {
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "missing native mouse conversion");
        [pool release];
        return NULL;
    }
    convert = (Convert)NSAddressOfSymbol(NSLookupAndBindSymbol("_nsMouseEventToJavaEvent"));
    if (type == 8 || type == 9)
        event = [NSEvent enterExitEventWithType:type location:NSMakePoint(-5.75, 30.75)
            modifierFlags:flags timestamp:1.0 windowNumber:0 context:nil eventNumber:1 trackingNumber:1 userData:NULL];
    else
        event = [NSEvent mouseEventWithType:type location:NSMakePoint(-5.75, 30.75)
            modifierFlags:flags timestamp:1.0 windowNumber:0 context:nil eventNumber:1 clickCount:3 pressure:0.5];
    view = [[MouseConversionView alloc] initInside:inside];
    result = convert(env, event, source, view);
    [view release];
    [pool release];
    return result;
}
