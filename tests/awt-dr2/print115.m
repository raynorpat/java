#import <AppKit/AppKit.h>
#import "../../src/rhapsody/sun/awt/awt_Print.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
#include "jni.h"
static int liveJobs, liveGraphics;
static BOOL failConstruction;
@interface PrintJobProbe : AwtPrintJob
@end
@implementation PrintJobProbe
- (id)initWithTitle:(NSString *)title disposition:(int)disposition destination:(NSString *)destination copies:(int)copies orientation:(int)orientation paperName:(NSString *)paper
{
    liveJobs++;
    self = [super initWithTitle:title disposition:disposition destination:destination copies:copies orientation:orientation paperName:paper];
    if (failConstruction) [NSException raise:@"PrintProbeFailure" format:@"Print constructor failure"];
    return self;
}
- (void)dealloc { liveJobs--; [super dealloc]; }
@end
@interface PrintGraphicsProbe : AwtPrintGraphics
@end
@implementation PrintGraphicsProbe
- (id)initWithPrintSurface:(id)surface { liveGraphics++; return [super initWithPrintSurface:surface]; }
- (void)dealloc { liveGraphics--; [super dealloc]; }
@end
@interface NSObject (PrintTest)
+ (id)appkitProxy;
- (NSRect)awtBounds;
- (NSRect)awtVisibleBounds;
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (id)awtColor;
- (BOOL)_beginDrawingWithPaint:(BOOL)paint;
- (void)_endDrawing;
- (void)printPage;
- (void)cleanUp;
@end
@interface PrintTest : NSObject
+ (void)arm;
+ (void)cancel:(NSTimer *)timer;
+ (void)disposeJob:(int)address;
+ (int)liveObjects;
@end
@implementation PrintTest
+ (int)liveObjects
{
    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
        modifierFlags:0 timestamp:1 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
    return liveJobs * 100 + liveGraphics;
}
+ (void)disposeJob:(int)address { [(id)address cleanUp]; [(id)address release]; }
+ (void)arm
{
    NSTimer *timer = [NSTimer timerWithTimeInterval:0.1 target:self selector:@selector(cancel:) userInfo:nil repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSModalPanelRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
}
+ (void)cancel:(NSTimer *)timer
{
    NSPrintPanel *panel = [[NSPrintOperation currentOperation] printPanel];
    if ([panel isVisible]) {
        JNIEnv *env = [NSApp appkitEnv];
        jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/Print115");
        jmethodID method = (*env)->GetStaticMethodID(env, cls, "modalTick", "()V");
        unichar key = 27;
        NSString *text = [NSString stringWithCharacters:&key length:1];
        int type;
        (*env)->CallStaticVoidMethod(env, cls, method);
        (*env)->DeleteLocalRef(env, cls);
        for (type = NSKeyDown; type <= NSKeyUp; type++)
            [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0) modifierFlags:0
                timestamp:1 windowNumber:[panel windowNumber] context:[NSApp context]
                characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:0] atStart:NO];
        [timer invalidate];
    }
}
@end
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Print115_track(JNIEnv *env, jclass cls)
{ [PrintJobProbe poseAsClass:[AwtPrintJob class]]; [PrintGraphicsProbe poseAsClass:[AwtPrintGraphics class]]; }
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_Print115_liveObjects(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; int result = [[PrintTest appkitProxy] liveObjects]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Print115_failConstruction(JNIEnv *env, jclass cls, jboolean fail)
{ failConstruction = fail; }
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_Print115_constant(JNIEnv *env, jclass cls, jstring name)
{
    jclass control = (*env)->FindClass(env, "com/apple/rhapsody/awt/RPrintControl");
    const char *text; jfieldID field;
    if (!control) return 0;
    text = (*env)->GetStringUTFChars(env, name, NULL);
    field = (*env)->GetStaticFieldID(env, control, text, "I");
    (*env)->ReleaseStringUTFChars(env, name, text);
    return field ? (*env)->GetStaticIntField(env, control, field) : 0;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Print115_cancelPanel(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[PrintTest appkitProxy] arm]; [pool release]; }
/* The donor's nDispose aborts in NSInvocation cleanup. Release the donor job
 * through a class receiver so its properties and dialog remain usable as an oracle. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Print115_donorDispose(JNIEnv *env, jclass cls, jobject job)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass jobClass = (*env)->GetObjectClass(env, job);
    jfieldID field = (*env)->GetFieldID(env, jobClass, "nNativePrintJob", "I");
    [[PrintTest appkitProxy] disposeJob:(*env)->GetIntField(env, job, field)];
    (*env)->SetIntField(env, job, field, 0);
    [pool release];
}
JNIEXPORT jboolean JNICALL Java_com_apple_rhapsody_awt_Print115_surfaceGraphics(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    id surface = [[NSClassFromString(@"AwtPrintSurface") alloc] init];
    jclass graphicsClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RPrintGraphics");
    jobject graphics = (*env)->NewObject(env, graphicsClass, (*env)->GetMethodID(env, graphicsClass, "<init>", "()V"));
    id nativeGraphics = (id)(*env)->CallIntMethod(env, graphics, (*env)->GetMethodID(env, graphicsClass, "nCreateNativeGraphics", "(I)I"), (int)surface);
    BOOL valid = surface && nativeGraphics && NSEqualRects([surface awtBounds], NSZeroRect) &&
        NSEqualRects([surface awtVisibleBounds], NSZeroRect) && ![surface awtCanDraw] &&
        [surface awtLockFocus] && ![surface awtColor] && [nativeGraphics _beginDrawingWithPaint:YES];
    [surface awtUnlockFocus]; [nativeGraphics _endDrawing];
    if (nativeGraphics) (*env)->CallVoidMethod(env, graphics, (*env)->GetMethodID(env, graphicsClass, "nPrintPage", "(I)V"), (int)nativeGraphics);
    [nativeGraphics release]; [surface release]; [pool release];
    return valid;
}
