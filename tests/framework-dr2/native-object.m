#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static int labelSets;
@interface NativeProbe : NSObject
@end
@implementation NativeProbe
- (NSString *)description { return @"native probe"; }
- (unsigned)hash { return 115; }
- copyWithZone:(NSZone *)zone { return [[NativeProbe allocWithZone:zone] init]; }
- label { return nil; }
- (void)setLabel:(id)value { ++labelSets; }
- valueForKey:(NSString *)key { return [self label]; }
- (void)takeValue:(id)value forKey:(NSString *)key { [self setLabel:value]; }
@end
@interface NativeOverride : NativeProbe
@end
@interface NativeOwn : NSObject
@end
@implementation NativeOwn
- retain { return [super retain]; }
- (void)release { [super release]; }
- (unsigned)hash { return 115; }
@end
@implementation NativeOverride
- retain { return [super retain]; }
- (void)release { [super release]; }
@end
static void *env, *nativeClass;
#ifdef DONOR
static void *(*loadVM)(NSString *, id), *(*findClass)(void *, char *, int), *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initCreate)(void), (*initMaps)(void);
static void *(*wrap)(id, void *), *(*lookupId)(id);
static long *(*hashStub)(long *, void *), *(*equalStub)(long *, void *), *(*nukeStub)(long *, void *);
static long *(*allocateStub)(long *, void *), *(*copyStub)(long *, void *), *(*stringStub)(long *, void *);
static id (*javaString)(void *), (*lookupHandle)(void *);
static void *(*objcString)(id);
static long *(*getStub)(long *, void *), *(*setStub)(long *, void *);
static void *(*newObject)(void *, void *, const char *, int, va_list);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invoke)(void *, void *, void *, int, int, va_list, long *, void **);
#else
extern void *JAVALoadVirtualMachine(NSString *, id), *FindClass(void *, char *, int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), CreateInitialization(void), bridgeInitializeObjectMap(void);
extern void *NSCreateExplicitJavaWrapper(id, void *), *NSJavaLookupId(id);
extern id NSJavaLookupHandle(void *);
extern long *Java_com_apple_alpha_core_NativeObject_hashCode_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_equals_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_nukeObjcObject_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_allocateObjcObject_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_copyObjcObject_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_toString_stub(long *, void *);
extern id _JavaStringToNSString(void *);
extern void *_NSStringToJavaString(id);
extern long *Java_com_apple_alpha_core_NativeObject_valueForKey_stub(long *, void *);
extern long *Java_com_apple_alpha_core_NativeObject_takeValueForKey_stub(long *, void *);
extern void *JAVANewObject(void *, void *, const char *, int, va_list);
extern void *_JAVAMethodLookup(void *, void *, const char *, const char *, int, int);
extern long __JAVAMethodInvokeNoExceptionConversion(void *, void *, void *, int, int, va_list, long *, void **);
/* Isolated dependencies: map-only object conversion and native class identity. */
id NSJavaHandleToId(void *h) { id o = NSJavaLookupHandle(h); if (h && !o) abort(); return o; }
void *NSIdToJavaHandle(id o) { void *h = NSJavaLookupId(o); if (o && !h) abort(); return h; }
void *NSObjcClassToJavaClass(Class c) { return nativeClass; }
Class NSJavaClassToObjcClass(void *c, BOOL create) { return [NativeProbe class]; }
BOOL _NSJavaObjectRespondsToSelector(void *h, id o, SEL s) { return [o respondsToSelector:s]; }
SEL _NSConvertSelFromJava(void *h) { abort(); return 0; }
int _NSJavaStubEnter(void) { return 0; }
void _NSJavaStubExit(int state) {}
void _NSJavaSafeRelease(id o) { [o release]; }
BOOL NSJavaValueForKey(void *h, NSString *key, id *value) { return NO; }
BOOL NSJavaTakeValueForKey(void *h, id value, NSString *key) { return NO; }
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initCreate CreateInitialization
#define initMaps bridgeInitializeObjectMap
#define wrap NSCreateExplicitJavaWrapper
#define lookupId NSJavaLookupId
#define lookup _JAVAMethodLookup
#define invoke __JAVAMethodInvokeNoExceptionConversion
#define hashStub Java_com_apple_alpha_core_NativeObject_hashCode_stub
#define equalStub Java_com_apple_alpha_core_NativeObject_equals_stub
#define nukeStub Java_com_apple_alpha_core_NativeObject_nukeObjcObject_stub
#define allocateStub Java_com_apple_alpha_core_NativeObject_allocateObjcObject_stub
#define copyStub Java_com_apple_alpha_core_NativeObject_copyObjcObject_stub
#define stringStub Java_com_apple_alpha_core_NativeObject_toString_stub
#define javaString _JavaStringToNSString
#define newObject JAVANewObject
#define lookupHandle NSJavaLookupHandle
#define objcString _NSStringToJavaString
#define getStub Java_com_apple_alpha_core_NativeObject_valueForKey_stub
#define setStub Java_com_apple_alpha_core_NativeObject_takeValueForKey_stub
#endif
static long roots(void) {
    void *c = findClass(env,"com/apple/yellow/foundation/NSRuntime",1), *e;
    return invoke(env,c,lookup(env,c,"count","()I",0,1),1,0,NULL,NULL,&e);
}
int main(int argc, char **argv) {
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    id object;
    void *handle, *copyHandle;
    long args[3], constructor[2] = {0,0}, baseline;
    unsigned i;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v = NSAddressOfSymbol(NSLookupAndBindSymbol(s))
    BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
    BIND(initGC,"_JavaGCInitialization"); BIND(initExceptions,"_ExceptionInitialization");
    BIND(initCreate,"_CreateInitialization"); BIND(initMaps,"_MappingInitialization");
    BIND(wrap,"_NSCreateExplicitJavaWrapper"); BIND(lookupId,"_NSJavaLookupId");
    BIND(lookup,"__JAVAMethodLookup"); BIND(invoke,"___JAVAMethodInvokeNoExceptionConversion");
    BIND(hashStub,"_Java_com_apple_alpha_core_NativeObject_hashCode_stub");
    BIND(equalStub,"_Java_com_apple_alpha_core_NativeObject_equals_stub");
    BIND(nukeStub,"_Java_com_apple_alpha_core_NativeObject_nukeObjcObject_stub");
    BIND(allocateStub,"_Java_com_apple_alpha_core_NativeObject_allocateObjcObject_stub");
    BIND(copyStub,"_Java_com_apple_alpha_core_NativeObject_copyObjcObject_stub");
    BIND(stringStub,"_Java_com_apple_alpha_core_NativeObject_toString_stub");
    BIND(javaString,"__JavaStringToNSString"); BIND(newObject,"_JAVANewObject");
    BIND(lookupHandle,"_NSJavaLookupHandle");
    BIND(objcString,"__NSStringToJavaString");
    BIND(getStub,"_Java_com_apple_alpha_core_NativeObject_valueForKey_stub");
    BIND(setStub,"_Java_com_apple_alpha_core_NativeObject_takeValueForKey_stub");
#endif
    if (!loadVM([NSString stringWithCString:argv[1]],nil)) return 1;
    env=currentEnv(); initGC(); initExceptions(); initMaps(); initCreate();
    nativeClass=findClass(env,"com/apple/alpha/core/NativeObject",1);
    if (!nativeClass) return 1;
    for (i=0;i<3;++i) {
        object = i == 2 ? [NativeOwn new] : i ? [NativeOverride new] : [NativeProbe new];
        baseline=roots(); handle=wrap(object,nativeClass);
        printf("mapped%u=%d,%u,%ld\n",i,lookupId(object)==handle,[object retainCount],roots()-baseline);
        args[0]=(long)handle; hashStub(args,env); printf("hash%u=%ld\n",i,args[0]);
        args[0]=(long)handle; args[1]=(long)handle; equalStub(args,env); printf("equal%u=%ld\n",i,args[0]);
        [object release];
        printf("release%u=%u,%ld\n",i,[object retainCount],roots()-baseline);
        [object retain];
        printf("retain%u=%u,%ld\n",i,[object retainCount],roots()-baseline);
        [object release];
        args[0]=(long)handle; nukeStub(args,env);
        printf("nuke%u=%ld,%d,%d\n",i,roots()-baseline, lookupHandle(handle) == nil, **(long **)handle == 0);
    }
    object = [NativeProbe new]; handle = wrap(object,nativeClass);
    args[0]=(long)handle; stringStub(args,env);
    printf("description=%s\n", [javaString((void *)args[0]) cString]);
    args[0]=(long)handle; args[1]=(long)objcString(@"label"); getStub(args,env);
    printf("get=%d\n", args[0] == 0);
    args[0]=(long)handle; args[1]=0; args[2]=(long)objcString(@"label"); setStub(args,env);
    printf("set=%d\n", labelSets == 1);
    copyHandle = newObject((void *)-1,nativeClass,"(ZI)",0,(va_list)constructor);
    args[0]=(long)handle; args[1]=(long)copyHandle; args[2]=0; copyStub(args,env);
    printf("copy=%d\n", lookupHandle(copyHandle) != object && [lookupHandle(copyHandle) hash] == 115);
    [lookupHandle(copyHandle) release]; args[0]=(long)copyHandle; nukeStub(args,env);
    [object release]; args[0]=(long)handle; nukeStub(args,env);
#ifndef DONOR
    /* Allocation uses the explicit class-conversion seam in this component test. */
    handle = newObject((void *)-1,nativeClass,"(ZI)",0,(va_list)constructor);
    args[0]=(long)handle; allocateStub(args,env);
    if (![lookupHandle(handle) isKindOfClass:[NativeProbe class]]) return 1;
    args[0]=(long)handle; nukeStub(args,env);
#endif
#ifndef DONOR
    for(i=0;i<_dyld_image_count();++i) {
        const char *p=_dyld_get_image_name(i);
        if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1;
    }
#endif
    [pool release]; return 0;
}
