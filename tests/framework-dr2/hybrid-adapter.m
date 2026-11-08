#import "classgen.h"
#import "dispatch.h"
Class createHybridFixture(ClassClass *clazz, Class parent) {
 return bridgeCreateJavaClass(clazz, parent, bridgeImplementationForMethod);
}
