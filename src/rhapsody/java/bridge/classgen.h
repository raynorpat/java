#import "class-map.h"

/* Dispatch selection is supplied by the object bridge. No method is installed
 * unless the caller supplies its actual implementation. */
typedef IMP (*BridgeMethodImplementation)(struct methodblock *, SEL);
Class bridgeCreatePureJavaClass(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation);
