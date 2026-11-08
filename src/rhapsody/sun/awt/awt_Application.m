/* DR2 libawt.A.dylib: 54002828, 54002858, 54002f6c--54003080. */
#import "awt_Application.h"
#import "awt_Dispatcher.h"
#import <objc/objc-runtime.h>

static JNIEnv *appkitEnv;
static NSThread *appkitThread;
static NSConnection *kitConnection;
static NSString *portName;

@implementation NSApplication (AWTAdditions)
- (id)proxyForClassByName:(NSString *)name
{
    return objc_getClass([name cString]);
}
- (id)proxyForObjectByAddress:(int)address
{
    return (id)address;
}
- (void)buildConnectionForDO
{
    int index;
    NSString *registeredName = nil;
    kitConnection = [[NSConnection alloc] init];
    [kitConnection setRootObject:self];
    for (index = 0; index < 10; index++) {
        NSString *name = [NSString stringWithFormat:@"Super-secret backdoor hack port #%d", index];
        if ([kitConnection registerName:name]) {
            registeredName = [name retain];
            break;
        }
    }
    if (!registeredName)
        [NSException raise:@"DO error" format:@"Can't register AppKit connection (all names in use)."];
    portName = registeredName;
    [kitConnection addRequestMode:NSModalPanelRunLoopMode];
    [kitConnection addRequestMode:NSEventTrackingRunLoopMode];
}
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env
{
    appkitEnv = env;
    [self commonAwtInitialization];
}
- (void)commonAwtInitialization
{
    appkitThread = [NSThread currentThread];
    [self buildConnectionForDO];
    [AwtDispatcher setDefaultDispatcher:[[AwtDispatcher alloc] initForConnection:kitConnection]];
}
- (NSString *)portName { return portName; }
- (BOOL)inAppkitThread { return appkitThread == [NSThread currentThread]; }
- (JNIEnv *)appkitEnv { return appkitEnv; }
@end
