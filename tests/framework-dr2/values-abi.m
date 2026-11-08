#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#ifndef DONOR
extern void *NSLoadVirtualMachine(NSString *);
#endif
struct Info { void *toJava,*toObjc; const char *name; void *javaClass; Class objcClass; int count; void *constructors; unsigned size; };
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new];
 void *(*lookupClass)(const char *); Class (*fromJava)(void *); void *(*toJava)(Class);
 void (*setType)(const char *,void *,void *); struct Info *(*infoForName)(const char *); struct Info *info;
  void *javaClass; unsigned i; void **classGlobal;Class *isaGlobal;
 void *(*currentEnv)(void); void (*classInit)(void); Class (*publicFrom)(void *),(*createClass)(void *,const char *);void *(*publicTo)(Class);
 struct Info *(*setConstructors)(const char *,const char *,int,void *);
 int (*goop)(void *,void *,unsigned *,unsigned **,char ***,char ***,unsigned *,unsigned **,char ***,char ***,unsigned *,unsigned **,char ***);
 unsigned fc,mc,cc,*ff,*mf,*cf;char **fn,**ft,**mn,**mt,**ct;
 void *(*rootPath)(void *,void *),*rootArgs[2]; id (*toId)(void *);
 struct {SEL selector;const char *signature;} constructors[1];
#ifdef DONOR
 void *(*loadVM)(NSString *);
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
 loadVM=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSLoadVirtualMachine"));
#else
#define loadVM NSLoadVirtualMachine
#endif
 if(argc!=2 || !loadVM([NSString stringWithCString:argv[1]])) return 1;
 if(!NSIsSymbolNameDefined("_NSSetJavaTypeMappings") || !NSIsSymbolNameDefined("__NSConvertClassFromJava") || !NSIsSymbolNameDefined("__NSConvertClassToJava")) { puts("abi-symbols=0");return 1; }
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(lookupClass,"_NSJavaLookupClassName"); BIND(fromJava,"__NSConvertClassFromJava"); BIND(toJava,"__NSConvertClassToJava");
 BIND(setType,"_NSSetJavaTypeMappings"); BIND(infoForName,"__BRIDGEJavaInfoForJavaClassName");
 BIND(classGlobal,"_JavaLangClassClass");BIND(isaGlobal,"_NSJavaObjectIsa");
 printf("globals=%d,%d\n",*classGlobal==lookupClass("java/lang/Class"),*isaGlobal==NSClassFromString(@"NSJavaObject"));
 javaClass=lookupClass("java/lang/String");
 printf("class=%d,%d\n",fromJava(javaClass)==[NSString class],toJava([NSString class])==javaClass);
 setType("audit/LegacyClass",fromJava,toJava); info=infoForName("audit/LegacyClass");
 printf("legacy=%d,%d,%d,%u\n",info->toObjc==(void *)fromJava,info->toJava==(void *)toJava,info->objcClass==Nil,info->size);
 setType("audit/LegacyClass",toJava,fromJava); setType("java/lang/String",fromJava,toJava);
 printf("preserved=%d,%d\n",info->toObjc==(void *)fromJava,info->toJava==(void *)toJava);
 BIND(publicFrom,"_BRIDGEConvertClassFromJava");BIND(publicTo,"_BRIDGEConvertClassToJava");BIND(classInit,"_ClassInitialization");classInit();
 printf("public-class=%d,%d\n",publicFrom(javaClass)==[NSString class],publicTo([NSString class])==javaClass);
 BIND(setConstructors,"_NSSetJavaClassAndConstructorsForObjcClass");constructors[0].selector=@selector(init);constructors[0].signature="()";
 info=setConstructors("audit/Constructed","NSConditionLock",1,constructors);
 printf("constructors=%d,%d,%d\n",info!=NULL,info->count==1,info->constructors==constructors);
 BIND(goop,"__BRIDGEInterestingGoopForJavaClass");BIND(currentEnv,"_EE");javaClass=lookupClass("ClassAbiFixture");
 if(!goop(currentEnv(),javaClass,&fc,&ff,&fn,&ft,&mc,&mf,&mn,&mt,&cc,&cf,&ct))return 1;
 printf("goop=%u,%u,%u\n",fc,mc,cc);
 for(i=0;i<fc;++i)printf("field=%u:%s:%.*s\n",ff[i],fn[i],ft[i][0]=='L' ? (int)(strchr(ft[i],';')-ft[i]+1) : 1,ft[i]);
 for(i=0;i<mc;++i)printf("method=%u:%s:%s\n",mf[i],mn[i],mt[i]);
 for(i=0;i<cc;++i)printf("constructor=%u:%s\n",cf[i],ct[i]);
 BIND(createClass,"__BRIDGECreateObjectiveCClassForJavaClass");
 printf("named-class=%s\n",createClass(javaClass,"AuditNamedClass")->name);
 BIND(rootPath,"_Java_com_apple_yellow_foundation_NSRuntime_nextRootPath_stub");BIND(toId,"_NSJavaHandleToId");
 rootPath(rootArgs,currentEnv());printf("next-root=%s\n",[toId(rootArgs[0]) cString]);
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) {const char *p=_dyld_get_image_name(i);if(strstr(p,"/usr/lib/java/")||strstr(p,"/System/Library/Frameworks/JavaVM.framework"))return 1;}
#endif
 [pool release];return 0;
}
