#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
static int nativeCalls, pureCalls;
@interface NSNibOutletConnector : NSObject { @public id source, destination; NSString *label; }
- (void)establishConnection;
@end
@implementation NSNibOutletConnector
- (void)establishConnection { ++nativeCalls; }
@end
@interface NibPure : NSObject
@end
@implementation NibPure
- (void)takeValue:(id)value forKey:(NSString *)key { if ([value intValue] == 115 && [key isEqual:@"outlet"]) ++pureCalls; }
@end
@interface NibHybrid : NSObject
@end
@implementation NibHybrid
- (NSString *)description { return @"nib hybrid"; }
@end
#ifdef DONOR
static void *(*loadVM)(NSString *, id), *(*findClass)(void *, char *, int), *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initMaps)(void), (*initNib)(void);
static void *(*newObject)(void *, void *, const char *, int, va_list);
static void (*map)(void *, id, BOOL);
static int (*get)(void *, NSString *, id *);
#else
extern void *JAVALoadVirtualMachine(NSString *, id), *FindClass(void *, char *, int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), bridgeInitializeObjectMap(void), NSJavaNibInitialization(void);
extern void *JAVANewObject(void *, void *, const char *, int, va_list);
extern void NSJavaMapObjects(void *, id, BOOL);
extern int NSJavaValueForKey(void *, NSString *, id *);
/* No object-valued Java fields or exception userInfo in this component fixture. */
id NSJavaHandleToId(void *h) { if(h) abort(); return nil; }
void *NSIdToJavaHandle(id o) { if(o) abort(); return NULL; }
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initMaps bridgeInitializeObjectMap
#define initNib NSJavaNibInitialization
#define newObject JAVANewObject
#define map NSJavaMapObjects
#define get NSJavaValueForKey
#endif
int main(int argc, char **argv) {
    NSAutoreleasePool *pool=[NSAutoreleasePool new];
    NSNibOutletConnector *connector=[NSNibOutletConnector new];
    id native=[NSObject new], pure=[NibPure new], hybrid=[NibHybrid new], value;
    void *env,*handle;
    unsigned i;
    if(argc!=2) return 2;
#ifdef DONOR
    if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
    BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
    BIND(initGC,"_JavaGCInitialization"); BIND(initExceptions,"_ExceptionInitialization"); BIND(initMaps,"_MappingInitialization");
    BIND(initNib,"_NSJavaNibInitialization"); BIND(newObject,"_JAVANewObject"); BIND(map,"_NSJavaMapObjects"); BIND(get,"_NSJavaValueForKey");
#endif
    if(!loadVM([NSString stringWithCString:argv[1]],nil)) return 1;
    env=currentEnv(); initGC(); initExceptions(); initMaps(); initNib(); initNib();
    [NibPure class]->info |= 0x400; [NibHybrid class]->info |= 0x200;
    connector->destination=[NSNumber numberWithInt:115]; connector->label=@"outlet";
    connector->source=native; [connector establishConnection];
    connector->source=pure; [connector establishConnection];
    printf("ordinary=%d pure=%d\n",nativeCalls,pureCalls);
    connector->source=hybrid; [connector establishConnection];
    printf("unmapped=%d\n",nativeCalls);
    handle=newObject(env,findClass(env,"NibFixture",1),"()",0,NULL); if(!handle) return 1;
    map(handle,hybrid,NO); [connector establishConnection];
    get(handle,@"outlet",&value); printf("hybrid=%d,%d\n",[value intValue],nativeCalls);
    connector->label=@"missing"; [connector establishConnection];
    printf("fallback=%d\n",nativeCalls);
#ifndef DONOR
    for(i=0;i<_dyld_image_count();++i) { const char *p=_dyld_get_image_name(i); if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1; }
#endif
    [pool release]; return 0;
}
