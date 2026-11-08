#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#ifdef DONOR
#import <JavaVM/JavaVM.h>
static SEL (*toSelector)(const char *, const char *, Class);
static void (*setMethod)(const char *, SEL);
static void (*setOverload)(const char *, const char *, SEL);
static BOOL (*getMethod)(SEL, const char **, const char **);
static void (*resetComplaints)(NSString *);
static void (*complain)(int, NSString *, ...);
#else
#import "selectors.h"
#import "diagnostics.h"
#define toSelector NSJavaMethodNameToSelector
#define setMethod NSSetJavaMethodForObjcSelector
#define setOverload NSSetOverloadedJavaMethodForObjcSelector
#define getMethod NSGetJavaMethodForSelector
#define resetComplaints _BRIDGEResetComplaints
#define complain _BRIDGEComplain
#endif

@interface SelectorFixture : NSObject
- (id)choose:(id)a with:(id)b;
@end
@implementation SelectorFixture
- (id)choose:(id)a with:(id)b { return a; }
@end
@interface SelectorChild : SelectorFixture
@end
@implementation SelectorChild
@end

static void show(const char *label, SEL selector)
{
    printf("%s=%s\n", label, selector ? sel_getName(selector) : "<nil>");
}

int main(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    const char *name = NULL, *signature = NULL;
#ifdef DONOR
    [NSJavaVirtualMachine defaultVirtualMachine];
    toSelector = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaMethodNameToSelector"));
    setMethod = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetJavaMethodForObjcSelector"));
    setOverload = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetOverloadedJavaMethodForObjcSelector"));
    getMethod = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSGetJavaMethodForSelector"));
    resetComplaints = NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEResetComplaints"));
    complain = NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEComplain"));
#else
    bridgeInitializeSelectorMaps();
#endif
    show("no-args", toSelector("fixtureNoArgs", "()I", Nil));
    show("two-args", toSelector("fixtureArgs", "(IJ)D", Nil));
    show("arrays", toSelector("fixtureArrays", "([[Ljava/lang/String;[I)V", Nil));
    show("inherited", toSelector("choose", "(I)I", [SelectorChild class]));
    show("nil-name", toSelector(NULL, "()V", Nil));
    show("bad-signature", toSelector("fixtureBad", "bad", Nil));
    setMethod("fixtureMapped", @selector(fixtureDefault:));
    setOverload("fixtureMapped", "(I)V", @selector(fixtureInteger:));
    show("specific", toSelector("fixtureMapped", "(I)V", Nil));
    show("fallback", toSelector("fixtureMapped", "(D)V", Nil));
    if (!getMethod(@selector(fixtureInteger:), &name, &signature)) return 1;
    printf("reverse-specific=%s,%s\n", name, signature ? signature : "<nil>");
    if (!getMethod(@selector(fixtureUnknown:with:), &name, &signature)) return 1;
    printf("reverse-derived=%s,%s\n", name, signature ? signature : "<nil>");
    show("reverse-cached", toSelector("fixtureUnknown", "()V", Nil));
    printf("nil-selector=%d\n", getMethod(NULL, &name, &signature));
    setMethod("fixtureReplace", @selector(fixtureBefore:));
    setMethod("fixtureReplace", @selector(fixtureAfter:));
    show("replacement", toSelector("fixtureReplace", "(I)V", Nil));
    if (!getMethod(@selector(fixtureBefore:), &name, &signature)) return 1;
    printf("old-reverse=%s\n", name);
    setMethod("fixtureOther", @selector(fixtureAfter:));
    if (!getMethod(@selector(fixtureAfter:), &name, &signature)) return 1;
    printf("replacement-reverse=%s\n", name);
    show("old-forward", toSelector("fixtureReplace", "(I)V", Nil));
    setMethod("fixtureOther", @selector(fixtureAfter:));
    setOverload("fixtureMapped", "(I)V", @selector(fixtureNewInteger:));
    resetComplaints(@"fixture banner");
    complain(1, @"fixture once %d", 1);
    complain(1, @"fixture hidden");
    complain(0, @"fixture always %d", 2);
    complain(0, @"fixture always %d", 3);
    resetComplaints(nil);
    complain(1, @"fixture after reset");
    [pool release];
    return 0;
}
