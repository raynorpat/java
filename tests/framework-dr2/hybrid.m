#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
static int nativeRetains,nativeReleases,nativeSets;
@interface HybridNativeBase : NSObject
@end
@implementation HybridNativeBase
- retain { ++nativeRetains; return [super retain]; }
- (void)release { ++nativeReleases; [super release]; }
- valueForKey:(NSString *)key { return @"native fallback"; }
- (void)takeValue:(id)value forKey:(NSString *)key { ++nativeSets; }
@end
@interface NSObject (HybridCalls)
- (int)answer;
- (int)second;
@end
#ifdef DONOR
static void *(*loadVM)(NSString *,id), *(*findClass)(void *,char *,int), *(*env)(void);
static void (*gcInit)(void), (*exceptionsInit)(void), (*mapsInit)(void), (*classesInit)(void), (*selectorsInit)(void);
static void *(*setClass)(const char *,Class);
static Class (*classForJava)(void *,BOOL);
static void *(*newObject)(void *,void *,const char *,int,va_list);
static void (*map)(void *,id,BOOL), (*unmap)(void *,id,BOOL);
static void *(*lookup)(void *,void *,const char *,const char *,int,int);
static long (*invoke)(void *,void *,void *,int,int,va_list,long *);
#else
extern void *JAVALoadVirtualMachine(NSString *,id), *FindClass(void *,char *,int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), MappingInitialization(void), ClassgenInitialization(void), SelectorInitialization(void);
extern void *NSSetJavaClassForObjcClass(const char *,Class);
extern Class createHybridFixture(void *,Class);
extern void *JAVANewObject(void *,void *,const char *,int,va_list);
extern void NSJavaMapObjects(void *,id,BOOL), NSJavaRemoveMapping(void *,id,BOOL);
extern void *_JAVAMethodLookup(void *,void *,const char *,const char *,int,int);
extern long __JAVAMethodInvoke(void *,void *,void *,int,int,va_list,long *);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define env EE
#define gcInit JavaGCInitialization
#define exceptionsInit ExceptionInitialization
#define mapsInit MappingInitialization
#define classesInit ClassgenInitialization
#define selectorsInit SelectorInitialization
#define setClass NSSetJavaClassForObjcClass
#define newObject JAVANewObject
#define map NSJavaMapObjects
#define unmap NSJavaRemoveMapping
#define lookup _JAVAMethodLookup
#define invoke __JAVAMethodInvoke
#endif
static long roots(void) {
 void *clazz=findClass(env(),"com/apple/yellow/foundation/NSRuntime",1);
 return invoke(env(),clazz,lookup(env(),clazz,"count","()I",0,1),1,0,NULL,NULL);
}
static int own(Class clazz,SEL selector) {
 void *iterator=NULL; struct objc_method_list *list; int i;
 while((list=class_nextMethodList(clazz,&iterator))) for(i=0;i<list->method_count;++i) if(list->method_list[i].method_name==selector) return 1;
 return 0;
}
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new];
 Class child,descendant; void *javaChild,*javaDescendant,*handle; id object; int i; long count;
 SEL selectors[6]={@selector(copy),@selector(retain),@selector(release),@selector(copyWithZone:),@selector(valueForKey:),@selector(takeValue:forKey:)};
 setbuf(stdout,NULL); if(argc!=2) return 2;
#ifdef DONOR
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(env,"_EE");
 BIND(gcInit,"_JavaGCInitialization"); BIND(exceptionsInit,"_ExceptionInitialization"); BIND(mapsInit,"_MappingInitialization"); BIND(classesInit,"_ClassgenInitialization"); BIND(selectorsInit,"_SelectorInitialization");
 BIND(setClass,"_NSSetJavaClassForObjcClass"); BIND(classForJava,"_NSJavaClassToObjcClass"); BIND(newObject,"_JAVANewObject");
 BIND(map,"_NSJavaMapObjects"); BIND(unmap,"_NSJavaRemoveMapping"); BIND(lookup,"__JAVAMethodLookup"); BIND(invoke,"___JAVAMethodInvoke");
#endif
 if(!loadVM([NSString stringWithCString:argv[1]],nil)) return 1;
 gcInit(); exceptionsInit(); mapsInit(); selectorsInit(); classesInit();
 setClass("HybridBase",[HybridNativeBase class]);
 javaChild=findClass(env(),"HybridFixture",1); javaDescendant=findClass(env(),"HybridDescendant",1);
#ifdef DONOR
 child=classForJava(javaChild,YES); descendant=classForJava(javaDescendant,YES);
#else
 child=createHybridFixture(javaChild,[HybridNativeBase class]); descendant=createHybridFixture(javaDescendant,child);
#endif
 printf("flags=%d,%d,%d,%d\n",!!(child->info&0x200),!!(child->isa->info&0x200),!!(descendant->info&0x200),!(child->info&0x400));
 printf("layout=%d,%d\n",child->instance_size==[HybridNativeBase class]->instance_size,descendant->instance_size==child->instance_size);
 for(i=0;i<6;++i) printf("method%d=%d,%d:%s\n",i,own(child,selectors[i]),own(descendant,selectors[i]),class_getInstanceMethod(child,selectors[i])->method_types);
 handle=newObject(env(),javaDescendant,"()",0,NULL);
 object=NSAllocateObject(descendant,0,NULL); map(handle,object,NO);
 printf("dispatch=%d,%d\n",[object answer],[object second]);
 printf("get=%d\n",[[object valueForKey:@"value"] intValue]);
 [object takeValue:[NSNumber numberWithInt:57] forKey:@"value"];
 printf("set=%d\n",[object answer]);
 printf("fallback=%s\n",[[object valueForKey:@"nativeOnly"] cString]);
 [object takeValue:nil forKey:@"nativeOnly"]; printf("fallback-set=%d\n",nativeSets);
 printf("copy=%d,%d\n",[object copy]==object,[object copyWithZone:NULL]==object);
 printf("clones=%d\n",[[object valueForKey:@"clones"] intValue]);
 count=roots(); nativeRetains=nativeReleases=0;
 [object retain]; printf("retain=%d,%d,%d\n",nativeRetains,[object retainCount]==2,roots()==count+1);
 [object retain]; printf("retain-again=%d,%d\n",nativeRetains,roots()==count+1);
 [object release]; printf("release3=%d,%d\n",nativeReleases,roots()==count+1);
 [object release]; printf("release2=%d,%d\n",nativeReleases,roots()==count);
 unmap(handle,object,NO); [object release];
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) {const char *p=_dyld_get_image_name(i);if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1;}
#endif
 [pool release];return 0;
}
