#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
static int nativeInits,nativeCopies,nativeDeallocs;
@interface HybridFullNative : NSObject { int nativeTag; }
- (int)nativeTag;
@end
@implementation HybridFullNative
- init { self=[super init]; ++nativeInits; nativeTag=115; return self; }
- copyWithZone:(NSZone *)zone { ++nativeCopies; return NSCopyObject(self,0,zone); }
- (int)nativeTag { return nativeTag; }
- (void)dealloc { ++nativeDeallocs; [super dealloc]; }
@end
@interface NSObject (FullHybridCalls)
- (int)value;
- identity;
+ make;
- (int)nativeHash;
- (BOOL)nativeSame:(id)object;
- (NSString *)nativeText;
- (int)nativeKVC;
- (void)change:(int)value;
- (void)disposeNative;
+ nativeRoundTrip:(id)value;
+ (BOOL)isNativeBase:(id)value;
+ (int)objectHash:(id)value;
+ (void)disposeBase:(id)value;
@end
#ifdef DONOR
static void *(*loadVM)(NSString *);
static id (*create)(const char *,const char *,...);
static void *(*setClass)(const char *,Class), *(*toJava)(id);
static id (*fromJava)(void *);
static void *(*lookupId)(id);
static void (*stubExit)(int);
#else
extern void *NSLoadVirtualMachine(NSString *);
extern id NSCreateJavaObject(const char *,const char *,...);
extern void *NSSetJavaClassForObjcClass(const char *,Class), *NSIdToJavaHandle(id);
extern id NSJavaHandleToId(void *);
extern void *NSJavaLookupId(id);
extern void _NSJavaStubExit(int);
#define stubExit _NSJavaStubExit
#define lookupId NSJavaLookupId
#define loadVM NSLoadVirtualMachine
#define create NSCreateJavaObject
#define setClass NSSetJavaClassForObjcClass
#define toJava NSIdToJavaHandle
#define fromJava NSJavaHandleToId
#endif
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new]; Class clazz,fixtureClass; id object,constructed,javaMade,mapped,copy,native;
 unsigned i; void *handle;
 setbuf(stdout,NULL); if(argc!=2) return 2;
#ifdef DONOR
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(loadVM,"_NSLoadVirtualMachine"); BIND(create,"_NSCreateJavaObject"); BIND(setClass,"_NSSetJavaClassForObjcClass");
 BIND(toJava,"_NSIdToJavaHandle"); BIND(fromJava,"_NSJavaHandleToId"); BIND(lookupId,"_NSJavaLookupId"); BIND(stubExit,"__NSJavaStubExit");
#endif
 if(!loadVM([NSString stringWithCString:argv[1]])) return 1;
 clazz=NSClassFromString(@"HybridFullFixture"); fixtureClass=clazz; if(!clazz) return 1;
 object=[[clazz alloc] init];
 printf("objc-constructor=%d,%d\n",[object value],!!(((Class)[object class])->info&0x200));
 handle=toJava(object); printf("identity=%d,%d,%d\n",handle!=NULL,fromJava(handle)==object,[object identity]==object);
 printf("native=%d,%d,%d\n",[object nativeHash]==[object hash],[object nativeSame:object],[[object nativeText] rangeOfString:@"HybridFullFixture"].length!=0);
 i=[object nativeKVC]; printf("native-kvc=%d,%d\n",i,[object value]);
 constructed=[create("HybridFullFixture","(I)",91) retain]; printf("c-constructor=%d\n",[constructed value]);
 javaMade=[[clazz make] retain]; printf("java-constructor=%d,%d\n",[javaMade value],fromJava(toJava(javaMade))==javaMade);
 setClass("HybridFullBase",[HybridFullNative class]);
 clazz=NSClassFromString(@"HybridFullMapped"); mapped=[[clazz alloc] init];
 printf("mapped-constructor=%d,%d,%d\n",[mapped value],[(HybridFullNative *)mapped nativeTag],nativeInits);
 copy=[mapped copy];
 printf("copy=%d,%d,%d,%d\n",copy!=mapped,[copy value],[(HybridFullNative *)copy nativeTag],nativeCopies);
 [copy change:81]; printf("independent=%d,%d,%d\n",[mapped value],[copy value],fromJava(toJava(copy))==copy);
 native=[[HybridFullNative alloc] init];
 printf("native-wrapper=%d,%d,%d\n",[fixtureClass nativeRoundTrip:native]==native,[fixtureClass isNativeBase:native],[fixtureClass objectHash:native]==[native hash]);
 printf("native-owned=%d,%d\n",lookupId(native)!=NULL,[native retainCount]==2);
 [fixtureClass disposeBase:native]; stubExit(0); printf("native-finalize=%d,%d\n",lookupId(native)==NULL,[native retainCount]==1);
 [native release];
 [fixtureClass disposeBase:copy]; stubExit(0); [copy release];
 [fixtureClass disposeBase:mapped]; stubExit(0); [mapped release];
 [javaMade disposeNative]; stubExit(0); [javaMade release];
 [constructed disposeNative]; stubExit(0); [constructed release];
 [object disposeNative]; stubExit(0); printf("hybrid-finalize=%d,%d\n",lookupId(object)==NULL,[object retainCount]==1); [object release];
 printf("native-deallocs=%d\n",nativeDeallocs);
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) {const char *p=_dyld_get_image_name(i);if(strstr(p,"/System/Library/Frameworks/JavaVM.framework")||strstr(p,"/usr/lib/java/")) {fprintf(stderr,"Unexpected donor image: %s\n",p);return 1;}}
#endif
 [pool release];return 0;
}
