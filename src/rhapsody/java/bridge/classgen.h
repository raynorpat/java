#import "class-map.h"
void ClassgenInitialization(void);

/* Dispatch selection is supplied by the object bridge. No method is installed
 * unless the caller supplies its actual implementation. */
typedef IMP (*BridgeMethodImplementation)(struct methodblock *, SEL);
Class bridgeCreatePureJavaClass(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation);

Class bridgeCreateJavaClass(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation);
/* Shared builder keeps the standalone pure-class API independent of lifetime
 * and KVC dispatch modules. Overrides are copied after declared Java methods. */
Class bridgeCreateJavaClassWithOverrides(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation, const struct objc_method *overrides,
    unsigned overrideCount);
Class bridgeCreateNamedJavaClass(const char *, ClassClass *, Class, BridgeMethodImplementation);
Class bridgeCreateNamedJavaClassWithOverrides(const char *, ClassClass *, Class,
    BridgeMethodImplementation, const struct objc_method *, unsigned);
