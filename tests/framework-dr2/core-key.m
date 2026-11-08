#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
@interface NSObject (WrapperDependencyProbe)
+ (NSString *)probe;
@end
#ifndef DONOR
extern void *NSLoadVirtualMachine(NSString *);
#endif
int main(int argc,char **argv) {
 NSAutoreleasePool *pool=[NSAutoreleasePool new]; Class fixture; unsigned i;
#ifdef DONOR
 void *(*loadVM)(NSString *);
 if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
 loadVM=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSLoadVirtualMachine"));
#else
#define loadVM NSLoadVirtualMachine
#endif
 if(argc!=2 || !loadVM([NSString stringWithCString:argv[1]])) return 1;
 fixture=NSClassFromString(@"CoreKeyFixture");if(!fixture)return 1;
 printf("result=%s\n",[[fixture probe] cString]);
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) { const char *p=_dyld_get_image_name(i); if(strstr(p,"/usr/lib/java/") || strstr(p,"/System/Library/Frameworks/JavaVM.framework")) {fprintf(stderr,"Unexpected donor image: %s\n",p);return 1;} }
#endif
 [pool release];return 0;
}


/* Bind the donor's actual exports without the retained keyValue class's broken
 * loadLibrary("ObjCJava") initializer. The reconstructed side uses the same ABI. */
#define KEY_ALIAS(name) void *Java_CoreKeyApi_##name##_stub(void *args,void *env) { void *(*f)(void *,void *)=NSAddressOfSymbol(NSLookupAndBindSymbol("_Java_com_apple_alpha_core_keyValue_" #name "_stub")); return f(args,env); }
KEY_ALIAS(hasKey)
KEY_ALIAS(getValue)
KEY_ALIAS(getIntValue)
KEY_ALIAS(getLongValue)
KEY_ALIAS(getFloatValue)
KEY_ALIAS(setValue)
KEY_ALIAS(setIntValue)
KEY_ALIAS(setLongValue)
KEY_ALIAS(setFloatValue)
