/* DR2 libObjCJava 0x53d02358, 0x53d05d84, 0x53d06008, 0x53d06b38. */
#import "bootstrap.h"
#import "vm.h"
#import "class-map.h"
#import "selectors.h"
#import "bridge-exceptions.h"
#import "retention.h"
#include <stdlib.h>
#include <limits.h>
extern const char *bridgeLoadedLibraryPath(void);
/* DR2 0x53d09d8c is an empty initializer; reflection uses VM metadata directly. */
void ReflectionInitialization(void) {}
extern void CreateInitialization(void);
extern void ClassgenInitialization(void), MappingInitialization(void);
extern void SelectorInitialization(void), SerializeInitialization(void);
extern void NSJavaNibInitialization(void);
extern id _JavaStringToNSString(void *), _JavaNumberToNSNumber(HObject *);
extern void *_NSStringToJavaString(id);
extern HObject *_NSNumberToJavaNumber(id);
void *NSLoadVirtualMachine(NSString *classpath)
{
    void *vm = JAVALoadVirtualMachine(classpath, [NSUserDefaults standardUserDefaults]);
    if (vm) {
        JavaBridgeVM *data = vm;
        JNIEnv *env = data->env;
        const char *image = bridgeLoadedLibraryPath();
        char absolute[PATH_MAX];
        jclass system, propertiesClass;
        jmethodID getProperties, put;
        jobject properties;
        jstring key, value;
        if (!image || !realpath(image, absolute)) {
            JAVAUnloadVirtualMachine(vm);
            return NULL;
        }
        system = (*env)->FindClass(env, "java/lang/System");
        getProperties = (*env)->GetStaticMethodID(env, system, "getProperties", "()Ljava/util/Properties;");
        properties = (*env)->CallStaticObjectMethod(env, system, getProperties);
        propertiesClass = (*env)->GetObjectClass(env, properties);
        put = (*env)->GetMethodID(env, propertiesClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        key = (*env)->NewStringUTF(env, "com.apple.yellow.foundation.ObjCJava");
        value = (*env)->NewStringUTF(env, absolute);
        (*env)->CallObjectMethod(env, properties, put, key, value);
        (*env)->DeleteLocalRef(env, value);
        (*env)->DeleteLocalRef(env, key);
        (*env)->DeleteLocalRef(env, propertiesClass);
        (*env)->DeleteLocalRef(env, properties);
        (*env)->DeleteLocalRef(env, system);
    }
    if (vm && !FindClass(EE(), "com/apple/yellow/foundation/NSRuntime", TRUE)) {
        NSLog(@"Couldn't load runtime class %s", "com/apple/yellow/foundation/NSRuntime");
        NSUnloadVirtualMachine(vm);
        return NULL;
    }
    return vm;
}
void NSUnloadVirtualMachine(void *vm) { JAVAUnloadVirtualMachine(vm); }
BOOL NSAddClassFile(void *vm, NSString *name, NSData *bytes)
{
    return JAVAAddClassFile(vm, [name cString], [bytes bytes], [bytes length]) != NULL;
}
void _NextJavaTableInitialization(void)
{
    NSAutoreleasePool *pool;
    unsigned i;
    static const char *strings[] = { "NSMutableString", "NSInlineCString", "NSCString",
        "NSSimpleCString", "NSSimpleString", "NSUnicodeString", "NSConstantString", "NSString" };
    static const char *numbers[][2] = { {"java/lang/Integer", "NSintNumber"},
        {"java/lang/Float", "NSfloatNumber"}, {"java/lang/Double", "NSdoubleNumber"},
        {"java/lang/Long", "NSlongLongNumber"}, {"java/lang/Number", "NSNumber"} };
    _NSBridgeLock = [NSRecursiveLock new];
    _NSFirstThread = cthread_self();
    GoThreadSafe();
    pool = [NSAutoreleasePool new];
    ReflectionInitialization();
    ExceptionInitialization();
    CreateInitialization();
    ClassgenInitialization();
    MappingInitialization();
    JavaGCInitialization();
    SelectorInitialization();
    SerializeInitialization();
    NSJavaNibInitialization();
    NSSetJavaMethodForObjcSelector("equals", @selector(isEqual:));
    NSSetJavaMethodForObjcSelector("hashCode", @selector(hash));
    NSSetJavaMethodForObjcSelector("clone", @selector(copyWithZone:));
    NSSetJavaMethodForObjcSelector("respondsToMethod", @selector(respondsToSelector:));
    NSSetJavaMethodForObjcSelector("performMethod", @selector(performSelector:));
    NSSetJavaMethodForObjcSelector("performMethodWithObject", @selector(performSelector:withObject:));
    NSSetJavaMethodForObjcSelector("performMethodWithObjects", @selector(performSelector:withObject:withObject:));
    for (i = 0; i < sizeof(strings)/sizeof(strings[0]); ++i)
        NSSetOverloadedJavaClassMappings("java/lang/String", strings[i], _JavaStringToNSString, _NSStringToJavaString);
    for (i = 0; i < sizeof(numbers)/sizeof(numbers[0]); ++i)
        NSSetJavaClassMappings(numbers[i][0], numbers[i][1], _JavaNumberToNSNumber, _NSNumberToJavaNumber);
    [pool release];
}
void ObjCJava_AInitialization(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    _NextJavaTableInitialization();
    NSSetJavaClassMappings("java/lang/Throwable", "NSException", _JavaExceptionToNSException, _NSExceptionToJavaException);
    [pool release];
}
void ObjCJavaInitialization(void) { ObjCJava_AInitialization(); }
