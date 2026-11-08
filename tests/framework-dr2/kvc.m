#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef DONOR
static void *(*loadVM)(NSString *, id), *(*findClass)(void *, char *, int), *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initMaps)(void);
static void *(*newObject)(void *, void *, const char *, int, va_list);
static void (*setMapping)(const char *, const char *, void *, void *);
static void *stringToJava, *javaToString;
static int (*get)(void *, NSString *, id *), (*put)(void *, id, NSString *);
#else
extern void *JAVALoadVirtualMachine(NSString *, id), *FindClass(void *, char *, int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), bridgeInitializeObjectMap(void);
extern void *JAVANewObject(void *, void *, const char *, int, va_list);
extern int NSJavaValueForKey(void *, NSString *, id *), NSJavaTakeValueForKey(void *, id, NSString *);
extern id _JavaStringToNSString(void *);
extern void *_NSStringToJavaString(id);
/* All reference values used by this component fixture are Java strings. */
id NSJavaHandleToId(void *h) { return h ? _JavaStringToNSString(h) : nil; }
void *NSIdToJavaHandle(id v) { return v ? _NSStringToJavaString(v) : NULL; }
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initMaps bridgeInitializeObjectMap
#define newObject JAVANewObject
#define get NSJavaValueForKey
#define put NSJavaTakeValueForKey
#endif
static void show(void *object, NSString *key) {
 id value = @"sentinel";
 int found = get(object, key, &value);
 printf("%s=%d:%s\n", [key cString], found, value ? [[value description] cString] : "nil");
}
int main(int argc, char **argv) {
 NSAutoreleasePool *pool = [NSAutoreleasePool new];
 void *object, *env;
 unsigned i;
 NSString *keys[] = {@"i",@"b",@"s",@"c",@"z",@"f",@"d",@"j",@"text",@"object",@"inherited",@"hidden",@"precedence",@"method",@"byteMethod",@"shortMethod",@"charMethod",@"boolMethod",@"floatMethod",@"doubleMethod",@"longMethod",@"stringMethod",@"array",@"missing",@"arg"};
 setbuf(stdout,NULL); if (argc != 2) return 2;
#ifdef DONOR
 if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v = NSAddressOfSymbol(NSLookupAndBindSymbol(s))
 BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
 BIND(initGC,"_JavaGCInitialization"); BIND(initExceptions,"_ExceptionInitialization"); BIND(initMaps,"_MappingInitialization");
 BIND(setMapping,"_NSSetJavaClassMappings"); BIND(stringToJava,"__NSStringToJavaString"); BIND(javaToString,"__JavaStringToNSString");
 BIND(newObject,"_JAVANewObject"); BIND(get,"_NSJavaValueForKey"); BIND(put,"_NSJavaTakeValueForKey");
#endif
 if (!loadVM([NSString stringWithCString:argv[1]],nil)) return 1;
 env=currentEnv(); initGC(); initExceptions(); initMaps();
 #ifdef DONOR
 setMapping("java/lang/String","NSString",javaToString,stringToJava);
#endif
 object=newObject(env,findClass(env,"KVCFixture",1),"()",0,NULL);
 if (!object) return 1;
 for(i=0;i<sizeof(keys)/sizeof(keys[0]);++i) show(object,keys[i]);
 for(i=0;i<8;++i) { put(object,[NSNumber numberWithInt:34],keys[i]); show(object,keys[i]); }
 put(object,[NSNumber numberWithInt:90],@"precedence"); show(object,@"rawPrecedence");
 put(object,nil,@"i"); show(object,@"i");
 put(object,nil,@"object"); show(object,@"object");
 put(object,@"serial",@"serial"); show(object,@"serial");
 put(object,@"changed",@"stringMethod"); show(object,@"text");
 put(object,[NSNumber numberWithInt:44],@"method"); show(object,@"i");
 put(object,[NSNumber numberWithDouble:8.25],@"floatMethod"); show(object,@"f");
 put(object,[NSNumber numberWithDouble:9.5],@"doubleMethod"); show(object,@"d");
 put(object,[NSNumber numberWithLongLong:8589934595LL],@"longMethod"); show(object,@"j"); show(object,@"longMethod");
 put(object,@"32",@"number"); put(object,@"42",@"integer"); put(object,@"3.5",@"decimal"); show(object,@"numbers");
 put(object,@"32.5",@"number"); show(object,@"numbers");
 printf("missing-put=%d array-put=%d\n",put(object,nil,@"missing"),put(object,nil,@"array"));
 for(i=0;i<6;++i) {
  NS_DURING
   id value;
   if(i==0) get(NULL,@"x",&value);
   if(i==1) put(NULL,nil,@"x");
   if(i==2) get(object,@"fail",&value);
   if(i==3) put(object,[NSNumber numberWithInt:1],@"fail");
   if(i==4) put(object,@"bad",@"integer");
   if(i==5) put(object,@"wrong",@"peer");
   printf("error%u=not-raised\n",i);
  NS_HANDLER
   printf("error%u=%s|%s\n",i,[[localException name] cString],[[localException reason] cString]);
  NS_ENDHANDLER
 }
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) { const char *p=_dyld_get_image_name(i); if(strstr(p,"JavaVM.framework")||strstr(p,"libObjCJava")) return 1; }
#endif
 [pool release]; return 0;
}

