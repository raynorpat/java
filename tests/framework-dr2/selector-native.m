#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
typedef struct { long a,b,c; } SelectorTriple;
@interface SelectorNativeTarget : NSObject { int calls; }
@end
@implementation SelectorNativeTarget
- (long)longValue { return 115; }
- (char)charValue { return -7; }
- (short)shortValue { return 123; }
- (long long)wideValue { return 0x123456789LL; }
- (float)floatValue { return 1.5f; }
- (double)doubleValue { return 1.25; }
- (id)objectValue { return @"native"; }
- (void)nothing { ++calls; }
- (int)unsupported { ++calls; return 17; }
- (SelectorTriple)structure { SelectorTriple value={1,2,3}; ++calls; return value; }
- (long)acceptLong:(long)value { return value; }
- (long long)acceptWide:(long long)value { return value; }
- (float)acceptFloat:(float)value { return value; }
- (double)acceptDouble:(double)value { return value; }
- (id)acceptObject:(id)value { return value; }
- (id)acceptString:(char *)value { return (id)value; }
- (id)acceptSelector:(SEL)value { return NSStringFromSelector(value); }
- (void)acceptStruct:(SelectorTriple)value { ++calls; }
- (void)acceptInt:(int)value { ++calls; }
- (long)calls { return calls; }
- (id)raises { [NSException raise:NSInvalidArgumentException format:@"selector failure"]; return nil; }
@end
@interface NSObject (SelectorNativeCalls)
+ (id)probe:(id)target :(id)name :(int)mode;
+ (BOOL)implemented;
@end
#ifdef DONOR
static void *(*loadVM)(NSString *);
static id (*create)(const char *,const char *,...);
#else
extern void *NSLoadVirtualMachine(NSString *);
extern id NSCreateJavaObject(const char *,const char *,...);
#define loadVM NSLoadVirtualMachine
#define create NSCreateJavaObject
#endif
int main(int argc,char **argv)
{
    NSAutoreleasePool *pool=[NSAutoreleasePool new];
    id fixture,target=[SelectorNativeTarget new]; Class clazz; unsigned i;
    const char *names[]={"longValue","charValue","shortValue","wideValue","floatValue","doubleValue","objectValue","nothing","unsupported","structure","acceptLong:","acceptWide:","acceptFloat:","acceptDouble:","acceptObject:","acceptString:","acceptSelector:","acceptStruct:","acceptInt:","absent","acceptLong:","longValue","calls","acceptLong:","acceptWide:","acceptFloat:","acceptDouble:","raises"};
    int modes[]={0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,5,6,1,1,0,0,1,0,5,5,5,5,0};
    if(argc!=2) return 2; setbuf(stdout,NULL);
#ifdef DONOR
    if(!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSLoadVirtualMachine"));
    create=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSCreateJavaObject"));
#endif
    if(!loadVM([NSString stringWithCString:argv[1]])) return 1;
    fixture=create("SelectorNativeFixture","()"); clazz=[fixture class];
    for(i=0;i<sizeof(modes)/sizeof(modes[0]);++i)
        printf("%s=%s\n",names[i],[[clazz probe:target :[NSString stringWithCString:names[i]] :modes[i]] cString]);
    printf("implemented=%d\n",[clazz implemented]);
    printf("nil-target=%s\n",[[clazz probe:nil :@"description" :0] cString]);
#ifndef DONOR
    for(i=0;i<_dyld_image_count();++i) {
        const char *name=_dyld_get_image_name(i);
        if(strstr(name,"/usr/lib/java/libObjCJava") || strstr(name,"/System/Library/Frameworks/JavaVM.framework")) return 9;
    }
#endif
    [target release]; [pool release]; return 0;
}
