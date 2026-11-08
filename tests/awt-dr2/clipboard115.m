#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_Clipboard.h"
static int liveClipboards, failClipboard;
@interface ClipboardLifetimeProbe : AwtClipboard
@end
@implementation ClipboardLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveClipboards++; return value; }
- (void)dealloc { liveClipboards--; [super dealloc]; }
- (id)initWithPeer:(int)peer
{
    self = [super initWithPeer:peer];
    if (failClipboard) { failClipboard = 0; [NSException raise:NSInvalidArgumentException format:@"clipboard construction failure"]; }
    return self;
}
@end
@interface ClipboardProbe : NSObject
+ (void)trackAndFail;
+ (int)liveClipboards;
+ (NSString *)readNativeString;
+ (void)writeNativeString:(NSString *)text;
@end
@implementation ClipboardProbe
+ (void)trackAndFail { [ClipboardLifetimeProbe poseAsClass:[AwtClipboard class]]; failClipboard = 1; }
+ (int)liveClipboards { return liveClipboards; }
+ (NSString *)readNativeString { return [[NSPasteboard generalPasteboard] stringForType:NSStringPboardType]; }
+ (void)writeNativeString:(NSString *)text
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard declareTypes:[NSArray arrayWithObject:text ? NSStringPboardType : @"test/unsupported"] owner:nil];
    if (text) [pasteboard setString:text forType:NSStringPboardType];
}
@end
JNIEXPORT jstring JNICALL Java_Clipboard115_readNative(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jstring result = nsToJavaString(env, [[ClipboardProbe appkitProxy] readNativeString]);
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Clipboard115_writeNative(JNIEnv *env, jclass cls, jstring text)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ClipboardProbe appkitProxy] writeNativeString:text ? javaToNSString(env, text) : nil];
    [pool release];
}

JNIEXPORT void JNICALL Java_Clipboard115_trackAndFail(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[ClipboardProbe appkitProxy] trackAndFail]; [pool release]; }
JNIEXPORT jint JNICALL Java_Clipboard115_liveClipboards(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; int result = [[ClipboardProbe appkitProxy] liveClipboards]; [pool release]; return result; }
