#import <AppKit/AppKit.h>
#include <mach-o/dyld.h>
#include "jni.h"
JNIEXPORT jobject JNICALL Java_KeyValues115_convert(JNIEnv *env, jclass cls, jobject source,
    jint type, jint flags, jchar character, jchar plain)
{
    typedef jobject (*Convert)(JNIEnv *, NSEvent *, jobject);
    id pool = [[NSAutoreleasePool alloc] init];
    NSEvent *event;
    jobject result;
    Convert convert;
    if (!NSIsSymbolNameDefined("_nsKeyEventToJavaEvent")) {
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "missing native key conversion");
        [pool release];
        return NULL;
    }
    convert = (Convert)NSAddressOfSymbol(NSLookupAndBindSymbol("_nsKeyEventToJavaEvent"));
    event = [NSEvent keyEventWithType:type location:NSMakePoint(0, 0) modifierFlags:flags
        timestamp:1 windowNumber:0 context:nil characters:[NSString stringWithCharacters:&character length:1]
        charactersIgnoringModifiers:[NSString stringWithCharacters:&plain length:1] isARepeat:NO keyCode:0];
    result = convert(env, event, source);
    [pool release];
    return result;
}
