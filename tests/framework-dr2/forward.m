#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
typedef struct { long x; } ForwardOne;
typedef struct { long x,y; } ForwardPair;
typedef struct { long x,y,z; } ForwardTriple;
static long returnWord(void *handle) { return **(long **)handle; }
static ForwardOne returnOne(void *handle) { ForwardOne v; v.x=returnWord(handle); return v; }
static ForwardPair returnPair(void *handle) { ForwardPair v; v.x=returnWord(handle); v.y=v.x+1; return v; }
static ForwardTriple returnTriple(void *handle) { ForwardTriple v; v.x=returnWord(handle); v.y=v.x+1; v.z=v.x+2; return v; }
static void *argumentHandle(void *argument,int *size) { *size=12; return *(void **)argument; }
#ifdef DONOR
#include <mach-o/dyld.h>
#import <JavaVM/JavaVM.h>
static void (*mapSelector)(const char *,const char *,SEL);
static void (*mapType)(const char *,void *,void *,unsigned);
static void (*convertReturn)(void *,const char *,NSObjCValue *);
static BOOL (*objectResponds)(void *,id,SEL);
static BOOL (*classResponds)(void *,Class,SEL);
static void *invokeJava, *invokeStatic;
static void *(*javaClass)(Class);
static void *(*convertArgument)(const char *,void *,int *);
static SEL (*selectorName)(const char *,int,char *,int);
static char *(*methodName)(SEL,char *,int);
static const char *(*oldMethodName)(SEL);
static void *structAddress;
#else
#import "objects.h"
#import "vm.h"
#import "bridge-exceptions.h"
#import "retention.h"
#import "selectors.h"
#import "coding.h"
#import "forwarding.h"
#import "dispatch.h"
#define objectResponds _NSJavaObjectRespondsToSelector
#define classResponds _NSJavaClassRespondsToSelector
#define invokeJava _NSInvokeJavaMethod
#define invokeStatic _NSInvokeStaticJavaMethod
#define javaClass NSObjcClassToJavaClass
#define convertArgument _BRIDGEConvertObjcArgToJava
#define selectorName JavaMethodNameToObjectiveCSelector
#define methodName ObjectiveCSelectorToJavaMethodName
#define oldMethodName NSSelectorToJavaMethodName
#define structAddress _BRIDGEStructReturnAddress
extern void *_NSStringToJavaString(id);
extern id _JavaStringToNSString(void *);
#define mapSelector NSSetOverloadedJavaMethodForObjcSelector
#define mapType BRIDGESetJavaTypeMapping
#define convertReturn _BRIDGEConvertJavaRetvalToObjc
extern unsigned long _dyld_image_count(void);
extern const char *_dyld_get_image_name(unsigned long);
#endif
@interface NSObject (ForwardProbe)
- (BOOL)respondsTo:(SEL)selector;
- (id)validateValue:(id *)value forKey:(NSString *)key;
- initWithKeyValueUnarchiver:(id)coder;
- awakeAfterUsingArchiver:(id)coder;
- replacementObjectForArchiver:(id)coder;
- (int)tally;
- (long)r0;
- (ForwardOne)r4;
- (ForwardPair)r8;
- (ForwardTriple)r12;
- (float)aliasSingle:(float)value;
- (double)aliasFraction:(double)value;
#ifdef DONOR
- (void *)javaHandle;
#endif
@end
@interface RespondProbe : NSObject @end
@implementation RespondProbe
- (BOOL)respondsToSelector:(SEL)selector { return selector == @selector(nativeOnly); }
- (BOOL)respondsTo:(SEL)selector { return NO; }
@end
@interface RespondHybridProbe : RespondProbe @end
@implementation RespondHybridProbe
- (BOOL)respondsToSelector:(SEL)selector { return NO; }
@end
@interface NSMethodSignature (ForwardProbe)
+ (id)signatureWithObjCTypes:(const char *)types;
@end
static void invokeStruct(id object,SEL selector,const char *types,void *result) {
    NSInvocation *call=[NSInvocation invocationWithMethodSignature:[NSMethodSignature signatureWithObjCTypes:types]];
    [call setSelector:selector]; [call setTarget:object]; [object forwardInvocation:call]; [call getReturnValue:result];
}
static void invoke(id object, SEL selector, void *a, void *b, void *result)
{
    NSMethodSignature *signature=[object methodSignatureForSelector:selector];
    NSInvocation *call=[NSInvocation invocationWithMethodSignature:signature];
    [call setSelector:selector]; [call setTarget:object];
    if(a) [call setArgument:a atIndex:2]; if(b) [call setArgument:b atIndex:3];
    [object forwardInvocation:call];
    if(result) [call getReturnValue:result];
}
int main(int argc,char **argv)
{
    NSAutoreleasePool *pool=[NSAutoreleasePool new];
    Class clazz;
    id object, value, decoded, kv;
    long frame[4];
    int a=11,b=9,sum=0;
    float f=1.25f,fr=0;
    double d=1.25,dr=0;
    long long wide=0;
    NSMutableData *archive;
    NSArchiver *writer; NSUnarchiver *reader;
    ForwardOne one; ForwardPair pair; ForwardTriple triple;
    NSObjCValue converted;
    unsigned i;
#ifdef DONOR
    NSJavaVirtualMachine *vm;
#endif
    if(argc!=2) return 2; setbuf(stdout,NULL);
#ifdef DONOR
    vm=[[[NSJavaVirtualMachine alloc] initWithClassPath:[NSString stringWithCString:argv[1]]] autorelease];
    objectResponds=NSAddressOfSymbol(NSLookupAndBindSymbol("__NSJavaObjectRespondsToSelector"));
    invokeJava=NSAddressOfSymbol(NSLookupAndBindSymbol("__NSInvokeJavaMethod"));
    invokeStatic=NSAddressOfSymbol(NSLookupAndBindSymbol("__NSInvokeStaticJavaMethod"));
    javaClass=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSObjcClassToJavaClass"));
    convertArgument=NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEConvertObjcArgToJava"));
    selectorName=NSAddressOfSymbol(NSLookupAndBindSymbol("_JavaMethodNameToObjectiveCSelector"));
    methodName=NSAddressOfSymbol(NSLookupAndBindSymbol("_ObjectiveCSelectorToJavaMethodName"));
    oldMethodName=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSelectorToJavaMethodName"));
    structAddress=NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEStructReturnAddress"));
    classResponds=NSAddressOfSymbol(NSLookupAndBindSymbol("__NSJavaClassRespondsToSelector"));
    mapType=NSAddressOfSymbol(NSLookupAndBindSymbol("_BRIDGESetJavaTypeMapping"));
    convertReturn=NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEConvertJavaRetvalToObjc"));
    mapType("ForwardR0",returnWord,NULL,0); mapType("ForwardR4",returnOne,NULL,4);
    mapType("ForwardR8",returnPair,NULL,8); mapType("ForwardR12",returnTriple,NULL,12);
    clazz=[vm findClass:@"ForwardFixture"];
    {
        /* The shipped bootstrap leaves these NSCoder natives unresolved.
         * Bind the donor's own implementations for the archive integration probe. */
        extern void codingBind(void *, void *, void *);
        void *(*currentEnv)(void)=NSAddressOfSymbol(NSLookupAndBindSymbol("_EE"));
        void *(*findClass)(void *,char *,int)=NSAddressOfSymbol(NSLookupAndBindSymbol("_FindClass"));
        void *(*lookup)(void *,void *,char *,char *,int,int)=NSAddressOfSymbol(NSLookupAndBindSymbol("__JAVAMethodLookup"));
        char *classes[4]={"NSCoderOutputStream","NSCoderOutputStream","NSCoderInputStream","NSCoderInputStream"};
        char *names[4]={"writeByte","write","readByte","read"};
        char *signatures[4]={"(I)V","([BII)V","()I","([BII)I"};
        char className[256],symbol[256]; unsigned i;
        for(i=0;i<4;++i) {
            sprintf(className,"com/apple/yellow/foundation/_private/%s",classes[i]);
            sprintf(symbol,"_Java_com_apple_yellow_foundation_0005fprivate_%s_%s_stub",classes[i],names[i]);
            codingBind(lookup(currentEnv(),findClass(currentEnv(),className,1),names[i],signatures[i],0,0),
                NSAddressOfSymbol(NSLookupAndBindSymbol(symbol)),NSAddressOfSymbol(NSLookupAndBindSymbol("_invokeNativeMethod")));
        }
    }
    mapSelector=NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetOverloadedJavaMethodForObjcSelector"));
#else
    if(!JAVALoadVirtualMachine([NSString stringWithCString:argv[1]],nil)) return 1;
    JavaGCInitialization(); ExceptionInitialization(); MappingInitialization(); bridgeInitializeSelectorMaps(); SerializeInitialization();
    NSSetJavaClassMappings("java/lang/String","NSString",_JavaStringToNSString,_NSStringToJavaString);
    NSSetJavaClassMappings("java/lang/Throwable","NSException",_JavaExceptionToNSException,_NSExceptionToJavaException);
    mapType("ForwardR0",returnWord,NULL,0); mapType("ForwardR4",returnOne,NULL,4);
    mapType("ForwardR8",returnPair,NULL,8); mapType("ForwardR12",returnTriple,NULL,12);
    clazz=NSJavaClassToObjcClass(NSJavaLookupClassName("ForwardFixture"),YES);
#endif
    object=[[clazz alloc] init];
    mapSelector("validateValueForKey","(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/Object;",@selector(validateValue:forKey:));
    mapSelector("sum","(II)I",@selector(aliasSum::));
    mapSelector("word","()Ljava/lang/String;",@selector(aliasWord));
    mapSelector("one","(Ljava/lang/String;)Ljava/lang/String;",@selector(aliasOne:));
    mapSelector("two","(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",@selector(aliasTwo::));
    mapSelector("single","(F)F",@selector(aliasSingle:));
    mapSelector("fraction","(D)D",@selector(aliasFraction:));
    mapSelector("wide","()J",@selector(aliasWide));
    mapSelector("mark","(I)V",@selector(aliasMark:));
    mapSelector("twice","(I)I",@selector(aliasTwice:));
    {
        id native=[RespondProbe new], hybrid=[RespondHybridProbe new];
        ((Class)[RespondHybridProbe class])->info |= 0x200;
        printf("respond-fallback=%d,%d,%d,%d,%d\n",objectResponds(NULL,native,@selector(nativeOnly)),objectResponds(NULL,hybrid,@selector(nativeOnly)),objectResponds(NULL,native,@selector(description)),objectResponds(NULL,native,(SEL)0),classResponds(NULL,[RespondProbe class],@selector(nativeOnly)));
        ((Class)[RespondHybridProbe class])->info &= ~0x200;
        [native release]; [hybrid release];
    }
    printf("perform=%s|%s|%s\n",[[object performSelector:@selector(aliasWord)] cString],
        [[object performSelector:@selector(aliasOne:) withObject:@"a"] cString],
        [[object performSelector:@selector(aliasTwo::) withObject:@"a" withObject:@"b"] cString]);
    printf("native-perform=%d\n",[object performSelector:@selector(class)]==clazz);
    invoke(object,@selector(aliasSum::),&a,&b,&sum); printf("invoke-int=%d\n",sum);
    invoke(object,@selector(aliasSingle:),&f,NULL,&fr); printf("invoke-float=%.2f\n",fr);
    invoke(object,@selector(aliasFraction:),&d,NULL,&dr); printf("invoke-double=%.2f\n",dr);
    printf("forward-floats=%.2f,%.2f\n",[object aliasSingle:f],[object aliasFraction:d]);
    invoke(object,@selector(aliasWide),NULL,NULL,&wide); printf("invoke-long=%016llx\n",wide);
    value=@"x"; invoke(object,@selector(aliasOne:),&value,NULL,&decoded); printf("invoke-object=%s\n",[decoded cString]);
    a=37; invoke(object,@selector(aliasMark:),&a,NULL,NULL); printf("invoke-void=%d\n",[object tally]);
    frame[0]=(long)object; frame[1]=(long)@selector(aliasSum::); frame[2]=3; frame[3]=4;
    printf("forward=%lld\n",((long long (*)(id,SEL,SEL,void *))objc_msgSend)(object,@selector(forward::),@selector(aliasSum::),frame));
    frame[0]=(long)clazz; frame[1]=(long)@selector(aliasTwice:); frame[2]=6;
    printf("forward-static=%lld\n",((long long (*)(id,SEL,SEL,void *))objc_msgSend)(clazz,@selector(forward::),@selector(aliasTwice:),frame));
    {
        char buffer[64], tiny[1], *allocated;
        SEL name=selectorName("named",2,buffer,sizeof(buffer));
        int consumed=0;
        id argument=object;
        printf("exports-invoke=%lld,",((long long (*)(void *,SEL,void *,void *))invokeJava)([object javaHandle],@selector(aliasSum::),frame+2,NULL));
        printf("%lld,",((long long (*)(void *,SEL,void *,void *))invokeStatic)(javaClass(clazz),@selector(aliasTwice:),frame+2,NULL));
        printf("%.2f,",((float (*)(void *,SEL,void *,void *))invokeJava)([object javaHandle],@selector(aliasSingle:),&f,NULL));
        printf("%.2f\n",((double (*)(void *,SEL,void *,void *))invokeJava)([object javaHandle],@selector(aliasFraction:),&d,NULL));
        printf("exports-argument=%d,",convertArgument("ForwardFixture",&argument,&consumed)==[object javaHandle]); printf("%d\n",consumed);
        allocated=methodName(name,tiny,sizeof(tiny));
        printf("exports-names=%s,%s,%s,%d,%d\n",sel_getName(name),methodName(name,buffer,sizeof(buffer)),allocated,selectorName(NULL,0,buffer,sizeof(buffer))==NULL,methodName(NULL,buffer,sizeof(buffer))==NULL);
        free(allocated);
        printf("exports-oldname=%s\n",oldMethodName(@selector(aliasTwice:)));
        {
            void *observed,*handle=[object javaHandle];
            mapType("ForwardArg",returnWord,argumentHandle,12);
            printf("exports-customarg=%d,",convertArgument("ForwardArg",&handle,&consumed)==handle); printf("%d\n",consumed);
            __asm__ volatile ("call *%1" : "=a" (observed) : "S" (structAddress), "b" (&triple) : "ecx", "edx", "memory");
            ((long long (*)(void *,SEL,void *,void *))invokeJava)(handle,@selector(r12),NULL,&triple);
            printf("exports-struct=%d,%ld,%ld,%ld\n",observed==&triple,triple.x,triple.y,triple.z);
        }
    }
    printf("protocol=%d responds=%d\n",[object conformsToProtocol:@protocol(NSCoding)],[object respondsTo:@selector(aliasWord)]);
    value=@"old"; kv=[[objc_getClass("NSJavaObject") alloc] init];
    printf("validation-absent=%d\n",[kv validateValue:&value forKey:@"x"]==nil && [value isEqual:@"old"]);
    decoded=[object validateValue:&value forKey:@"x"]; printf("validation=%d|%s\n",decoded==nil,[value cString]);
    printf("coder-class=%d,%d\n",[object classForCoder]==clazz,[object classForArchiver]==clazz);
    printf("coding-hooks=%d,%d,%d,%d\n",[object replacementObjectForCoder:nil]==object,[object replacementObjectForArchiver:nil]==object,[object awakeAfterUsingCoder:nil]==object,[object awakeAfterUsingArchiver:nil]==object);
    archive=[NSMutableData data]; writer=[[NSArchiver alloc] initForWritingWithMutableData:archive];
    NS_DURING
        [writer encodeRootObject:object]; reader=[[NSUnarchiver alloc] initForReadingWithData:archive]; decoded=[reader decodeObject];
        printf("coding=%d,%d\n",[decoded class]==clazz,[decoded tally]);
    NS_HANDLER
        printf("coding=%s\n",[[localException name] cString]);
    NS_ENDHANDLER
    kv=[[clazz alloc] initWithKeyValueUnarchiver:nil]; printf("keyvalue-init=%d\n",[kv tally]);
    printf("mapped-word=%ld\n",[object r0]);
    one=[object r4]; pair=[object r8]; triple=[object r12];
    printf("mapped-struct=%ld|%ld,%ld|%ld,%ld,%ld\n",one.x,pair.x,pair.y,triple.x,triple.y,triple.z);
    memset(&one,0,sizeof(one)); memset(&pair,0,sizeof(pair)); memset(&triple,0,sizeof(triple));
    invokeStruct(object,@selector(r4),"{One=i}0@0:0",&one);
    invokeStruct(object,@selector(r8),"{Pair=ii}0@0:0",&pair);
    invokeStruct(object,@selector(r12),"{Triple=iii}0@0:0",&triple);
    printf("invoke-struct=%ld|%ld,%ld|%ld,%ld,%ld\n",one.x,pair.x,pair.y,triple.x,triple.y,triple.z);
    mapType("ForwardMissing",NULL,NULL,0);
    for(i=0;i<2;++i) {
        memset(&converted,0,sizeof(converted));
        NS_DURING
            convertReturn([object javaHandle],i ? "ForwardR12" : "ForwardMissing",&converted);
            printf("mapping-error%u=not-raised\n",i);
        NS_HANDLER
            printf("mapping-error%u=%s|%s\n",i,[[localException name] cString],[[localException reason] cString]);
        NS_ENDHANDLER
    }
    kv=[objc_getClass("NSJavaObject") alloc];
    NS_DURING
        ((long long (*)(id,SEL,SEL,void *))objc_msgSend)(kv,@selector(forward::),@selector(aliasWord),frame);
        puts("uninitialized=not-raised");
    NS_HANDLER
        printf("uninitialized=%s|%s\n",[[localException name] cString],[[localException reason] cString]);
    NS_ENDHANDLER
    NSDeallocateObject(kv);
#ifndef DONOR
    for(i=0;i<_dyld_image_count();++i) {
        const char *path=_dyld_get_image_name(i);
        if(strstr(path,"JavaVM.framework")||strstr(path,"libObjCJava")) return 1;
    }
#endif
    [pool release]; return 0;
}
