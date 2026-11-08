#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef DONOR
static void *(*loadVM)(NSString *, id), *(*findClass)(void *, char *, int), *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initCoding)(void);
static void (*archiveJava)(void *, id);
static void *(*unarchiveJava)(id);
static void (*serializeObjc)(id, void *);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invoke)(void *, void *, void *, int, int, va_list, long *, void **);
#else
extern void *JAVALoadVirtualMachine(NSString *, id), *FindClass(void *, char *, int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), SerializeInitialization(void);
extern void NSArchiveJavaObject(void *, id);
extern void *NSUnarchiveJavaObject(id), *ObjectOutputStreamForCoder(id), *ObjectInputStreamForCoder(id);
extern id CoderForObjectOutputStream(void *), CoderForObjectInputStream(void *);
extern void NSSerializeObjCObject(id, void *);
extern void *_JAVAMethodLookup(void *, void *, const char *, const char *, int, int);
extern long __JAVAMethodInvokeNoExceptionConversion(void *, void *, void *, int, int, va_list, long *, void **);
/* Exception conversion component boundary: these cases have no userInfo. */
id NSJavaHandleToId(void *object) { if (object) abort(); return nil; }
void *NSIdToJavaHandle(id object) { if (object) abort(); return NULL; }
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initCoding SerializeInitialization
#define archiveJava NSArchiveJavaObject
#define unarchiveJava NSUnarchiveJavaObject
#define outputStream ObjectOutputStreamForCoder
#define inputStream ObjectInputStreamForCoder
#define outputCoder CoderForObjectOutputStream
#define inputCoder CoderForObjectInputStream
#define serializeObjc NSSerializeObjCObject
#define lookup _JAVAMethodLookup
#define invoke __JAVAMethodInvokeNoExceptionConversion
#endif
static void *env, *fixture;
static long call(char *name, char *signature, ...) {
    void *exception;
    long result;
    va_list args;
    va_start(args, signature);
    result = invoke(env, fixture, lookup(env, fixture, name, signature, 0, 1), 1, 0, args, NULL, &exception);
    va_end(args);
    if (exception) exit(1);
    return result;
}
int main(int argc, char **argv) {
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSMutableData *data = [NSMutableData data];
    NSArchiver *writer = [[NSArchiver alloc] initForWritingWithMutableData:data];
    NSUnarchiver *reader;
    void *object, *decoded;
    unsigned i;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(v,s) v = NSAddressOfSymbol(NSLookupAndBindSymbol(s))
    BIND(loadVM,"_JAVALoadVirtualMachine"); BIND(findClass,"_FindClass"); BIND(currentEnv,"_EE");
    BIND(initGC,"_JavaGCInitialization"); BIND(initExceptions,"_ExceptionInitialization"); BIND(initCoding,"_SerializeInitialization");
    BIND(archiveJava,"_NSArchiveJavaObject"); BIND(unarchiveJava,"_NSUnarchiveJavaObject");
    BIND(serializeObjc,"_NSSerializeObjCObject");
    BIND(lookup,"__JAVAMethodLookup"); BIND(invoke,"___JAVAMethodInvokeNoExceptionConversion");
#endif
    if (!loadVM([NSString stringWithCString:argv[1]], nil)) return 1;
    env = currentEnv(); initGC(); initExceptions(); initCoding();
#ifdef DONOR
    {
        char *classes[4] = {"NSCoderOutputStream", "NSCoderOutputStream", "NSCoderInputStream", "NSCoderInputStream"};
        char *names[4] = {"writeByte", "write", "readByte", "read"};
        char *signatures[4] = {"(I)V", "([BII)V", "()I", "([BII)I"};
        extern void codingBind(void *, void *, void *);
        char className[256], symbol[256];
        for (i = 0; i < 4; ++i) {
            sprintf(className,"com/apple/yellow/foundation/_private/%s", classes[i]);
            sprintf(symbol,"_Java_com_apple_yellow_foundation_0005fprivate_%s_%s_stub", classes[i], names[i]);
            codingBind(lookup(env,findClass(env,className,1),names[i],signatures[i],0,0),
                NSAddressOfSymbol(NSLookupAndBindSymbol(symbol)), NSAddressOfSymbol(NSLookupAndBindSymbol("_invokeNativeMethod")));
        }
    }
#endif
    fixture = findClass(env,"CodingFixture",1);
    object = (void *)call("make","()Ljava/lang/Object;");
    archiveJava(object, writer); archiveJava(object, writer); archiveJava(NULL, writer);
#ifdef DONOR
    if (![data writeToFile:@"out/dr2-framework-tests/coding-donor.archive" atomically:NO]) return 1;
#else
    if (![data writeToFile:@"out/dr2-framework-tests/coding.archive" atomically:NO]) return 1;
    if (outputCoder(outputStream(writer)) != writer) return 1;
#endif
    reader = [[NSUnarchiver alloc] initForReadingWithData:data];
    decoded = unarchiveJava(reader);
    printf("roundtrip=%ld\n", call("check","(Ljava/lang/Object;)Z",decoded));
    printf("identity=%d\n", unarchiveJava(reader) == decoded);
    printf("nil=%d\n", unarchiveJava(reader) == NULL);
#ifndef DONOR
    if (inputCoder(inputStream(reader)) != reader) return 1;
#endif
    NS_DURING
        archiveJava((void *)call("invalid","()Ljava/lang/Object;"), writer);
        puts("invalid=not-raised");
    NS_HANDLER
        printf("invalid=%s\n", [[localException name] cString]);
    NS_ENDHANDLER
    NS_DURING
        serializeObjc(@"unsupported", NULL);
        puts("unknown=not-raised");
    NS_HANDLER
        printf("unknown=%s|%s\n", [[localException name] cString], [[localException reason] cString]);
    NS_ENDHANDLER
#ifndef DONOR
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path,"JavaVM.framework") || strstr(path,"libObjCJava")) return 1;
    }
#endif
    [pool release];
    return 0;
}



