#import <AppKit/AppKit.h>
#include "jni.h"
extern NSCursor *javaToNSCursor(JNIEnv *, jobject);
@interface NSObject (CursorTest)
+ (id)appkitProxy;
@end
JNIEXPORT jboolean JNICALL Java_Cursors115_matches(JNIEnv *env, jclass cls, jobject value, jint type)
{
    static char *selectors[] = {"arrowCursor", "_crosshairCursor", "IBeamCursor", "_waitCursor",
        "_bottomLeftResizeCursor", "_bottomRightResizeCursor", "_topLeftResizeCursor", "_topRightResizeCursor",
        "_verticalResizeCursor", "_verticalResizeCursor", "_horizontalResizeCursor", "_horizontalResizeCursor",
        "_handCursor", "_moveCursor"};
    id pool = [[NSAutoreleasePool alloc] init];
    id cursor = javaToNSCursor(env, value);
    BOOL result = cursor == nil;
    if (type >= 0) {
        id expected = [[NSCursor appkitProxy] performSelector:NSSelectorFromString([NSString stringWithCString:selectors[type]])];
        result = [cursor isEqual:expected];
    }
    [pool release];
    return result;
}
