#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
@interface NSObject (WrapperDependencyProbe)
+ (NSString *)probe:(int)which;
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
 if(argc!=3 || !loadVM([NSString stringWithCString:argv[1]])) return 1;
 fixture=NSClassFromString(@"WrapperDependencyFixture");if(!fixture)return 1;
 printf("result=%s\n",[[fixture probe:atoi(argv[2])] cString]);
#ifndef DONOR
 for(i=0;i<_dyld_image_count();++i) { const char *p=_dyld_get_image_name(i); if(strstr(p,"/usr/lib/java/") || strstr(p,"/System/Library/Frameworks/JavaVM.framework")) {fprintf(stderr,"Unexpected donor image: %s\n",p);return 1;} }
#endif
 [pool release];return 0;
}
