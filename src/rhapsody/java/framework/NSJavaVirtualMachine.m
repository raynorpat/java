/* Reconstructed from DR2 JavaVM, methods at 0x5cd00f60–0x5cd01388. */
#import "JavaVM.h"
#include <mach-o/dyld.h>

NSString *NSInvalidJavaClassException = @"Not a Java Class";
static NSMutableArray *allVirtualMachines;

static NSString *bridgePath(void)
{
    /* The original absolute /usr/lib/java path is private-framework relative. */
    return [[[NSBundle bundleForClass:[NSJavaVirtualMachine class]] bundlePath]
        stringByAppendingPathComponent:@"Libraries/libObjCJava.dylib"];
}

static void *bridgeFunction(const char *name)
{
    if (!NSIsSymbolNameDefined(name)) return NULL;
    return NSAddressOfSymbol(NSLookupAndBindSymbol(name));
}

@implementation NSJavaVirtualMachine
+ (void)initialize
{
    if (!allVirtualMachines) allVirtualMachines = [[NSMutableArray alloc] init];
}
+ (NSArray *)allVirtualMachines
{
    return allVirtualMachines;
}
+ (id)defaultVirtualMachine
{
    if (![allVirtualMachines count])
        [[[NSJavaVirtualMachine allocWithZone:NULL]
            initWithClassPath:[[self class] defaultClassPath]] autorelease];
    return [allVirtualMachines count] ? [allVirtualMachines objectAtIndex:0] : nil;
}
+ (NSString *)defaultClassPath
{
    NSString *path = [[[NSProcessInfo processInfo] environment] objectForKey:@"CLASSPATH"];
    if (!path) path = [[NSJavaConfiguration defaultConfiguration] valueForKey:NSDefaultJavaClassPathKey];
    return path;
}
- init
{
    return [self initWithClassPath:[[self class] defaultClassPath]];
}
- initWithClassPath:(NSString *)classPath
{
    NSString *path;
    void *(*loadVM)(NSString *);
    if ([allVirtualMachines count])
        [NSException raise:NSInternalInconsistencyException
            format:@"NSJavaVirtualMachine: multiple virtual machines are not supported in a single process"];
    path = bridgePath();
    if (!NSAddLibrary([path cString]))
        [NSException raise:NSInternalInconsistencyException
            format:@"NSJavaVirtualMachine: can't load ObjCJava library %@", path];
    [allVirtualMachines addObject:self];
    loadVM = (void *(*)(NSString *))bridgeFunction("_NSLoadVirtualMachine");
    if (!loadVM)
        [NSException raise:NSInternalInconsistencyException
            format:@"NSJavaVirtualMachine: can't find NSLoadVirtualMachine() in ObjCJava library"];
    _vmdata = loadVM(classPath);
    return self;
}
- (Class)findClass:(NSString *)javaClassName
{
    return NSClassFromString(javaClassName);
}
- (BOOL)defineClass:(NSData *)data withName:(NSString *)name
{
    BOOL (*addClass)(void *, NSString *, NSData *);
    addClass = (BOOL (*)(void *, NSString *, NSData *))bridgeFunction("_NSAddClassFile");
    if (!addClass)
        [NSException raise:NSInternalInconsistencyException
            format:@"NSJavaVirtualMachine: can't find NSAddClassFile() in ObjCJava library"];
    return addClass(_vmdata, name, data);
}
@end

@implementation NSObject (InstantiatingJavaObjects)
+ (id)newWithSignature:(NSString *)signature, ...
{
    [NSException raise:NSInvalidJavaClassException
        format:@"+newWithSignature: sent to non-java class %@", [self class]];
    return nil;
}
+ (id)newWithSignature:(NSString *)signature arguments:(va_list)args
{
    [NSException raise:NSInvalidJavaClassException
        format:@"+newWithSignature:arguments: sent to non-java class %@", [self class]];
    return nil;
}
@end
