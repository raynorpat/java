#import "../../src/rhapsody/sun/awt/awt_View.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
static volatile int views;
static int failConstruction;
@interface ChildLifetimeProbe : AwtView
@end
@implementation ChildLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id view = [super allocWithZone:zone]; if (view) views++; return view; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font
        backgroundColor:background foregroundColor:foreground cursor:cursor];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"child construction failure"];
    return self;
}
- (void)dealloc { views--; [super dealloc]; }
@end
@interface ChildStateProbe : NSObject
+ (BOOL)parent:(int)parent child:(int)child frame:(NSRect)frame visible:(BOOL)visible enabled:(BOOL)enabled;
@end
@implementation ChildStateProbe
+ (BOOL)parent:(int)parent child:(int)child frame:(NSRect)frame visible:(BOOL)visible enabled:(BOOL)enabled
{
    AwtView *view = (AwtView *)child;
    return NSEqualRects([view frame], frame) && [view superview] == (visible ? (id)parent : nil) &&
        (!visible || [view isAwtEnabled] == enabled);
}
@end
static int nativeView(JNIEnv *env, jobject component)
{
    jclass cls = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, cls, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, component, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT jboolean JNICALL Java_ChildViews115_check(JNIEnv *env, jclass cls, jobject parent, jobject child,
    jint x, jint y, jint width, jint height, jboolean visible, jboolean enabled)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ChildStateProbe appkitProxy] parent:nativeView(env, parent) child:nativeView(env, child)
        frame:NSMakeRect(x,y,width,height) visible:visible enabled:enabled];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_ChildViews115_track(JNIEnv *env, jclass cls) { [ChildLifetimeProbe poseAsClass:[AwtView class]]; }
JNIEXPORT jint JNICALL Java_ChildViews115_liveViews(JNIEnv *env, jclass cls) { return views; }
JNIEXPORT void JNICALL Java_ChildViews115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
