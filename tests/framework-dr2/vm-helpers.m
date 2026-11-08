#import <Foundation/Foundation.h>
#import "vm.h"
#include <mach-o/dyld.h>
#include <mach/cthreads.h>
#include <stdio.h>
#include <string.h>
struct HandleView { void *body; unsigned typeAndLength; };
struct FieldView { void *clazz; const char *signature; const char *name; };
static void *bind(const char *name)
{
    if (!NSIsSymbolNameDefined(name)) { printf("missing=%s\n", name); return NULL; }
    return NSAddressOfSymbol(NSLookupAndBindSymbol(name));
}
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    JavaBridgeVM *(*loadVM)(NSString *);
    void *(*lookupClass)(const char *), *(*currentEnv)(void);
    struct FieldView *(*field)(void *, void *, const char *, const char *);
    struct HandleView *(*scalar)(void *, unsigned, const char *), *(*objects)(void *, unsigned, void *);
    void *(*string)(void *, const unsigned short *, unsigned);
    int (*subclass)(void *, void *, void *);
    void *(*address)(void *, const char *), *(*toNumber)(id);
    void (*initialize)(void *);
    id (*fromNumber[4])(void *), (*fromString)(void *);
    JavaBridgeVM *vm;
    JNIEnv *jni;
    void *env, *integer, *number, *object;
    struct FieldView *value;
    struct HandleView *array;
    const char *numberNames[] = {"__JavaIntegerToNSNumber", "__JavaFloatToNSNumber", "__JavaDoubleToNSNumber", "__JavaLongToNSNumber"};
    const char *types = "ZBCSIJFD";
    unsigned short characters[] = {65, 0, 0x03a9};
    char unknown[301];
    unsigned i;
    const char *version, *versionNumber;
    void **classCache;
    Class *proxyClass;
    struct mutex *mapLock;
    char *(*legacySignature)(const char *, char *, int);
    if (argc != 3 || !NSAddLibrary(argv[1])) return 1;
    loadVM = bind("_NSLoadVirtualMachine");
    vm = loadVM([NSString stringWithCString:argv[2]]);
    if (!vm) return 2;
    jni = vm->env;
    lookupClass = bind("_NSJavaLookupClassName"); currentEnv = bind("_EE");
    field = bind("_JAVAFieldLookup"); scalar = bind("_JAVANewScalarArray");
    objects = bind("_JAVANewObjectArray"); string = bind("_JAVANewString");
    subclass = bind("_JAVAIsSubclassOf"); address = bind("__NSAddressOfFunctionInLibrary");
    initialize = bind("__NSInitializeObjcRuntimeInLibrary");
    fromString = bind("__JavaStringToNSString"); toNumber = bind("__NSNumberToJavaNumber");
    for (i = 0; i < 4; ++i) fromNumber[i] = bind(numberNames[i]);
    if (!field || !scalar || !objects || !string || !subclass || !address || !initialize
        || !fromNumber[0] || !fromNumber[1] || !fromNumber[2] || !fromNumber[3]) return 3;
    env = currentEnv(); integer = lookupClass("java/lang/Integer");
    number = lookupClass("java/lang/Number"); object = lookupClass("java/lang/Object");
    version = bind("_ObjCJava_VERS_STRING"); versionNumber = bind("_ObjCJava_VERS_NUM");
    classCache = bind("_JavaLangClassClass"); proxyClass = bind("_NSJavaObjectIsa");
    mapLock = bind("_MapLock");
    legacySignature = bind("__NSJavaSignatureToObjc");
    if (!legacySignature) return 6;
    if (!version || !versionNumber || !classCache || !proxyClass || !mapLock) return 5;
    mutex_lock(mapLock); mutex_unlock(mapLock);
    printf("data=%d,%d,%d,%d\n", strstr(version, "Project:ObjCJava Version:73.dev") != NULL,
        !strcmp(versionNumber, "73.dev"), *classCache == lookupClass("java/lang/Class"),
        *proxyClass == NSClassFromString(@"NSJavaObject"));
    {
        char buffer[64], tiny[4] = "abc";
        char *result = legacySignature("(IF)D", buffer, sizeof(buffer));
        printf("signature-buffer=%d,%s\n", result == buffer, result);
        result = legacySignature("(IF)D", tiny, sizeof(tiny));
        printf("signature-small=%d,%s,%s\n", result != tiny, result, tiny);
        free(result);
        printf("signature-invalid=%d\n", legacySignature("I", buffer, sizeof(buffer)) == NULL);
    }
    value = field(env, integer, "value", "I");
    printf("field=%d,%d,%d\n", value && !strcmp(value->name, "value"), field(env, integer, "value", NULL) == value, field(env, integer, "value", "D") == NULL);
    printf("null-field=%d\n", !field(env, NULL, "value", NULL) && (*jni)->ExceptionOccurred(jni) != NULL);
    (*jni)->ExceptionClear(jni);
    printf("subclass=%d,%d,%d,%d\n", subclass(env, integer, number), subclass(env, integer, integer), subclass(env, number, integer), subclass(env, NULL, object));
    for (i = 0; types[i]; ++i) {
        array = scalar(env, 3, types + i);
        printf("array-%c=%u\n", types[i], array ? array->typeAndLength >> 5 : 99);
    }
    printf("bad-array=%d\n", !scalar(env, 3, "L") && (*jni)->ExceptionOccurred(jni) != NULL);
    (*jni)->ExceptionClear(jni);
    array = objects(env, 2, integer);
    printf("objects=%u,%d,%d\n", array->typeAndLength >> 5, ((void **)array->body)[0] == NULL, ((void **)array->body)[2] == integer);
    {
        NSString *text = fromString(string(env, characters, 3));
        printf("unicode=%u,%04x,%04x,%04x\n", [text length], [text characterAtIndex:0], [text characterAtIndex:1], [text characterAtIndex:2]);
    }
    printf("numbers=%d,%.2f,%.2f,%lld\n", [fromNumber[0](toNumber([NSNumber numberWithInt:-17])) intValue],
        [fromNumber[1](toNumber([NSNumber numberWithFloat:1.25f])) floatValue],
        [fromNumber[2](toNumber([NSNumber numberWithDouble:-2.5])) doubleValue],
        [fromNumber[3](toNumber([NSNumber numberWithLongLong:0x100000002LL])) longLongValue]);
    {
        long long bits = 0x100000002LL;
        struct HandleView boxed;
        union { long long value; unsigned long words[2]; } result;
        boxed.body = &bits;
        result.value = [fromNumber[3](&boxed) longLongValue];
        printf("long-callback=%08lx%08lx\n", result.words[1], result.words[0]);
    }
    memset(unknown, 'q', 300); unknown[300] = 0;
    initialize(NULL);
    printf("symbol=%d,%d,%d\n", address(NULL, "NSJavaLookupClassName") == (void *)lookupClass,
        address((void *)1, "FrameworkMissingSymbol") == NULL, address(NULL, unknown) == NULL);
    if (!strstr(argv[1], "/usr/lib/java/"))
        for (i = 0; i < _dyld_image_count(); ++i) {
            const char *path = _dyld_get_image_name(i);
            if (strstr(path, "/usr/lib/java/") || strstr(path, "/System/Library/Frameworks/JavaVM.framework")) return 4;
        }
    [pool release];
    return 0;
}
