#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_FileDialog.h"
static int liveDialogs, failConstruction;
@interface FileDialogLifetimeProbe : AwtDummyFileDialogView
@end
@implementation FileDialogLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveDialogs++; return value; }
- (void)dealloc { liveDialogs--; [super dealloc]; }
- (id)initWithPeer:(int)peer isSavePanel:(BOOL)save title:(NSString *)title file:(NSString *)file directory:(NSString *)directory wantsToFilter:(BOOL)filter
{
    self = [super initWithPeer:peer isSavePanel:save title:title file:file directory:directory wantsToFilter:filter];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"file dialog construction failure"];
    return self;
}
@end
@interface FileDialogProbe : NSObject
+ (void)armPanelResponse:(BOOL)approve;
+ (void)track;
+ (int)liveDialogs;
+ (BOOL)filterForView:(int)address path:(NSString *)path;
+ (void)respondToPanel:(NSTimer *)timer;
@end
@implementation FileDialogProbe
+ (void)track { [FileDialogLifetimeProbe poseAsClass:[AwtDummyFileDialogView class]]; }
+ (int)liveDialogs { return liveDialogs; }
+ (BOOL)filterForView:(int)address path:(NSString *)path
{
    NSSavePanel *panel = [NSSavePanel savePanel];
    BOOL result;
    [panel setRequiredFileType:@"app"];
    result = [(AwtDummyFileDialogView *)address filterAcceptsFilename:path forPanel:panel];
    [panel setRequiredFileType:nil];
    return result;
}
+ (void)armPanelResponse:(BOOL)approve
{
    [[NSRunLoop currentRunLoop] addTimer:[NSTimer timerWithTimeInterval:0.5
        target:self selector:@selector(respondToPanel:) userInfo:[NSNumber numberWithBool:approve] repeats:NO]
        forMode:NSModalPanelRunLoopMode];
}
+ (void)respondToPanel:(NSTimer *)timer
{
    NSWindow *panel = [NSApp modalWindow];
    unichar key = [[timer userInfo] boolValue] ? 13 : 27;
    NSString *text = [NSString stringWithCharacters:&key length:1];
    int type;
    for (type = NSKeyDown; type <= NSKeyUp; type++)
        [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0) modifierFlags:0
            timestamp:1 windowNumber:[panel windowNumber] context:[NSApp context]
            characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:0] atStart:NO];
}
@end
JNIEXPORT void JNICALL Java_FileDialog115_armResponse(JNIEnv *env, jclass cls, jboolean approve)
{ id pool = [[NSAutoreleasePool alloc] init]; [[FileDialogProbe appkitProxy] armPanelResponse:approve]; [pool release]; }

JNIEXPORT void JNICALL Java_FileDialog115_track(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[FileDialogProbe appkitProxy] track]; [pool release]; }
JNIEXPORT jint JNICALL Java_FileDialog115_liveDialogs(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; int result = [[FileDialogProbe appkitProxy] liveDialogs]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_FileDialog115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
JNIEXPORT jboolean JNICALL Java_FileDialog115_filter(JNIEnv *env, jclass cls, jobject peer, jstring path)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    BOOL result = [[FileDialogProbe appkitProxy] filterForView:(*env)->GetIntField(env, peer, field) path:javaToNSString(env, path)];
    [pool release]; return result;
}
