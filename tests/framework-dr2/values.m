#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef struct { void *toJava, *toObjc; const char *name; void *javaClass, *objcClass; int count; void *constructors; unsigned size; } ValueInfo;
#ifdef DONOR
static ValueInfo *(*getInfo)(const char *);
static void *(*loadVM)(NSString *, id), *(*findClass)(void *, char *, int), *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initMaps)(void), (*initSelectors)(void);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invoke)(void *, void *, void *, int, int, va_list, long *);
static void *(*toNumber)(id), *(*selToJava)(SEL), *(*selFromStack)(SEL *, unsigned *);
static id (*fromNumber)(void *), (*stringFromJava)(void *);
static SEL (*selFromJava)(void *);
static void (*mapSelector)(const char *, SEL);
#else
extern void *JAVALoadVirtualMachine(NSString *,id), *FindClass(void *,char *,int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), bridgeInitializeObjectMap(void), SelectorInitialization(void);
extern void *_JAVAMethodLookup(void *,void *,const char *,const char *,int,int);
extern long __JAVAMethodInvoke(void *,void *,void *,int,int,va_list,long *);
extern void *_NSNumberToJavaNumber(id), *_NSConvertSelToJava(SEL), *_NSConvertSelToJavaFromStack(SEL *,unsigned *);
extern id _JavaNumberToNSNumber(void *), _JavaStringToNSString(void *);
extern SEL _NSConvertSelFromJava(void *);
extern void NSSetJavaMethodForObjcSelector(const char *,SEL);
extern ValueInfo *_BRIDGEJavaInfoForJavaClassName(const char *);
#define getInfo _BRIDGEJavaInfoForJavaClassName
/* No general object conversion is used by this component. */
id NSJavaHandleToId(void *h) { if(h) abort(); return nil; }
void *NSIdToJavaHandle(id v) { if(v) abort(); return NULL; }
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initMaps bridgeInitializeObjectMap
#define initSelectors SelectorInitialization
#define lookup _JAVAMethodLookup
#define invoke __JAVAMethodInvoke
#define toNumber _NSNumberToJavaNumber
#define fromNumber _JavaNumberToNSNumber
#define stringFromJava _JavaStringToNSString
#define selToJava _NSConvertSelToJava
#define selFromJava _NSConvertSelFromJava
#define selFromStack _NSConvertSelToJavaFromStack
#define mapSelector NSSetJavaMethodForObjcSelector
#endif
static void *env,*fixture;
static long call(const char *name,const char *signature,...) {
 void *method=lookup(env,fixture,name,signature,0,1);
 va_list args; long result;
 if(!method) abort();
 va_start(args,signature); result=invoke(env,fixture,method,1,0,args,NULL); va_end(args);
 return result;
}
static const char *describe(void *h,const char *method) {
 return [stringFromJava((void *)call(method,"(Ljava/lang/Object;)Ljava/lang/String;",h)) cString];
}
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new];
 id numbers[9],value;
 void *h; SEL selector; unsigned size,i; ValueInfo *info;
 setbuf(stdout,NULL); if(argc!=2) return 2;
#ifdef DONOR
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
 BIND(initGC,"_JavaGCInitialization"); BIND(initExceptions,"_ExceptionInitialization"); BIND(initMaps,"_MappingInitialization");
 BIND(initSelectors,"_SelectorInitialization"); BIND(lookup,"__JAVAMethodLookup"); BIND(invoke,"___JAVAMethodInvoke");
 BIND(toNumber,"__NSNumberToJavaNumber"); BIND(fromNumber,"__JavaNumberToNSNumber"); BIND(stringFromJava,"__JavaStringToNSString");
 BIND(selToJava,"__NSConvertSelToJava"); BIND(selFromJava,"__NSConvertSelFromJava"); BIND(selFromStack,"__NSConvertSelToJavaFromStack");
 BIND(mapSelector,"_NSSetJavaMethodForObjcSelector");
 BIND(getInfo,"__BRIDGEJavaInfoForJavaClassName");
#endif
 if(!loadVM([NSString stringWithCString:argv[1]],nil)) return 1;
 env=currentEnv(); initGC(); initExceptions(); initMaps(); initSelectors();
 fixture=findClass(env,"ValuesFixture",1); if(!fixture) return 1;
 info=getInfo("com/apple/alpha/core/Selector");
 printf("mapping=%d,%d,%u\n",info && info->toObjc==(void *)selFromJava,info && info->toJava==(void *)selFromStack,info ? info->size : 999);
 info=getInfo("com/apple/alpha/core/ObjectiveCSelector");
 printf("objc-mapping=%d,%d,%u\n",info && info->toObjc==(void *)selFromJava,info && info->toJava==(void *)selFromStack,info ? info->size : 999);
 numbers[0]=[NSNumber numberWithInt:-42]; numbers[1]=[NSNumber numberWithUnsignedInt:4000000000U];
 numbers[2]=[NSNumber numberWithFloat:1.25]; numbers[3]=[NSNumber numberWithDouble:-3.5];
 numbers[4]=[NSNumber numberWithLongLong:8589934595LL]; numbers[5]=[NSNumber numberWithBool:YES];
 numbers[6]=[NSNumber numberWithChar:-7]; numbers[7]=[NSNumber numberWithShort:-300]; numbers[8]=[NSNumber numberWithLong:65536];
 printf("nil=%d,%d,%d,%d\n",toNumber(nil)==NULL,fromNumber(NULL)==nil,selToJava(NULL)==NULL,selFromJava(NULL)==NULL);
 for(i=0;i<9;++i) {
  h=toNumber(numbers[i]); value=fromNumber(h);
  printf("objc%u=%s|%s:%s\n",i,describe(h,"describe"),[value objCType],[[value description] cString]);
 }
 for(i=0;i<6;++i) {
  h=(void *)call("number","(I)Ljava/lang/Number;",i); value=fromNumber(h);
  printf("java%u=%s\n",i,value ? [[value description] cString] : "nil");
 }
 mapSelector("mapped",sel_registerName("customName:"));
 for(i=0;i<6;++i) {
  h=(void *)call("selector","(I)Lcom/apple/alpha/core/Selector;",i); selector=selFromJava(h);
  printf("selector%u=%s\n",i,selector ? sel_getName(selector) : "nil");
 }
 selector=sel_registerName("round:trip:"); h=selToJava(selector);
 printf("wrapper=%s:%s\n",describe(h,"className"),sel_getName(selFromJava(h)));
 h=selFromStack(&selector,&size);
 printf("stack=%u:%s\n",size,sel_getName(selFromJava(h)));
 selector=NULL; printf("nil-stack=%d:",selFromStack(&selector,&size)==NULL); printf("%u\n",size);
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) { const char *p=_dyld_get_image_name(i); if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1; }
#endif
 [pool release]; return 0;
}
