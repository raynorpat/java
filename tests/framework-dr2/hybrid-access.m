#import <Foundation/Foundation.h>
#import "vm.h"
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef DONOR
static void *(*loadVM)(NSString *,id), *(*findClass)(void *,char *,int), *(*currentEnv)(void);
static void (*gcInit)(void), (*exceptionsInit)(void);
static void *(*newObject)(void *,void *,const char *,int,va_list);
#else
extern void *JAVALoadVirtualMachine(NSString *,id), *FindClass(void *,char *,int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void);
extern void *JAVANewObject(void *,void *,const char *,int,va_list);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define gcInit JavaGCInitialization
#define exceptionsInit ExceptionInitialization
#define newObject JAVANewObject
#endif
static jint JNICALL constructorAccess(JNIEnv *jni, jclass clazz) {
 int result=0;
 NS_DURING
  result=newObject(currentEnv(),findClass(currentEnv(),"AccessFixture",1),"()",0,NULL) ? 1 : 2;
 NS_HANDLER
  result=[[localException name] isEqual:@"java/lang/IllegalAccessException"] ? 3 : 4;
 NS_ENDHANDLER
 return result;
}
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new];
 const char *names[]={"AccessFixture","AccessPackage","AccessProtected","AccessPublic","AccessThrow","AccessAbstract","AccessMissing"};
 JavaBridgeVM *data; JNIEnv *jni; jclass caller; jmethodID method;
 JNINativeMethod nativeMethod={"constructorAccess","()I",constructorAccess};
 int i; void *clazz; float shortFloat=1.25f; double wideFloat=1.25;
 setbuf(stdout,NULL); if(argc!=2) return 2;
#ifdef DONOR
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v=NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
 BIND(gcInit,"_JavaGCInitialization"); BIND(exceptionsInit,"_ExceptionInitialization"); BIND(newObject,"_JAVANewObject");
#endif
 data=loadVM([NSString stringWithCString:argv[1]],nil); if(!data) return 1;
 jni=data->env;
 gcInit(); exceptionsInit();
 for(i=0;i<7;++i) {
  NS_DURING
   clazz=findClass(currentEnv(),(char *)names[i],1);
   printf("%s=%d\n",names[i],newObject(currentEnv(),clazz,"()",0,NULL)!=NULL);
  NS_HANDLER
   printf("%s=%s|%s\n",names[i],[[localException name] cString],[localException reason] ? [[localException reason] cString] : "nil");
  NS_ENDHANDLER
 }
 NS_DURING
  clazz=findClass(currentEnv(),"AccessFixture",1);
  printf("float=%d,%d\n",newObject(currentEnv(),clazz,"(F)",1,(va_list)&shortFloat)!=NULL,newObject(currentEnv(),clazz,"(F)",0,(va_list)&wideFloat)!=NULL);
 NS_HANDLER
  printf("float=%s\n",[[localException name] cString]);
 NS_ENDHANDLER
 for(i=0;i<2;++i) {
  caller=(*jni)->FindClass(jni,i ? "AccessFixture" : "AccessCaller");
  if(!caller || (*jni)->RegisterNatives(jni,caller,&nativeMethod,1)) return 1;
  method=(*jni)->GetStaticMethodID(jni,caller,"call","()I");
  printf("java-caller%d=%d\n",i,(*jni)->CallStaticIntMethod(jni,caller,method));
  if((*jni)->ExceptionOccurred(jni)) return 1;
 }
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) {const char *p=_dyld_get_image_name(i);if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1;}
#endif
 [pool release];return 0;
}
