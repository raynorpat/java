/* DR2 NSNibOutletConnector hook, donor 0x53d08c74-0x53d08db7. */
#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#import "object-map.h"
#import "key-value.h"
#import "bridge-exceptions.h"

/* NSNibConnector's DR2 instance layout, without linking AppKit. */
typedef struct {
    Class isa;
    id source;
    id destination;
    NSString *label;
} BridgeNibConnector;
static IMP oldEstablishConnection;

static void JavaEstablishConnectionMethod(BridgeNibConnector *connector, SEL selector)
{
    HObject *handle;
    Class clazz = connector->source->isa;
    if (clazz->info & 0x200) {
        handle = NSJavaLookupId(connector->source);
        if (!handle) {
            NSLog(@"ObjCJava: no java object for %@", connector->source);
            NSLog(@"          can't set connection %@", connector->label);
        } else if (NSJavaTakeValueForKey(handle, connector->destination, connector->label)) {
            bridgeRaisePendingJavaException(EE());
            return;
        }
    } else if (clazz->info & 0x400) {
        objc_msgSend(connector->source, @selector(takeValue:forKey:), connector->destination, connector->label);
        return;
    }
    oldEstablishConnection((id)connector, selector);
}

void NSJavaNibInitialization(void)
{
    Class clazz;
    SEL selector;
    Method method;
    if (oldEstablishConnection) return;
    clazz = objc_lookUpClass("NSNibOutletConnector");
    selector = sel_getUid("establishConnection");
    if (clazz && selector) {
        method = class_getInstanceMethod(clazz, selector);
        oldEstablishConnection = method->method_imp;
        method->method_imp = (IMP)JavaEstablishConnectionMethod;
    }
}
