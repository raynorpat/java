/* Run unchanged against the donor and reconstructed configuration class. */
#import <Foundation/Foundation.h>
#ifdef RECONSTRUCTED
#import "NSJavaConfiguration.h"
#else
#import <JavaVM/NSJavaConfiguration.h>
#endif
#include <stdio.h>
#include <stdlib.h>

/* Keep defaults entirely in memory, including the persistent-domain lookup. */
static NSDictionary *application, *domain;
@interface FrameworkFixtureDefaults : NSUserDefaults
@end
@implementation FrameworkFixtureDefaults
+ (id)standardUserDefaults
{
    static id defaults;
    if (!defaults) defaults = [[self alloc] init];
    return defaults;
}
- (id)objectForKey:(NSString *)key
{
    return [key isEqual:@"JavaConfiguration"] ? application : nil;
}
- (NSDictionary *)persistentDomainForName:(NSString *)name
{
    return [name isEqual:@"JavaConfiguration"] ? domain : nil;
}
@end

static void show(const char *key, NSString *value)
{
    printf("%s=%s\n", key, value ? [value cString] : "<nil>");
}

int main(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSJavaConfiguration *config;
    NSArray *names;
    unsigned i, duplicates = 0;
    [FrameworkFixtureDefaults poseAsClass:[NSUserDefaults class]];
    application = [@"{Vendor=fixture; fixture={VM=vendor_vm; Empty=\"\";}; VM=top_vm; Compiler=app_compiler; Expand=\"  $FRAMEWORK_TEST_ROOT/lib:$FRAMEWORK_UNDEFINED:x\"; Braces=\"${FRAMEWORK_TEST_ROOT}/lib\"; Adjacent=\"$FRAMEWORK_TEST_ROOT$FRAMEWORK_TEST_ROOT\"; Dollar=\"end$\"; DefaultClasspath=fixture_cp; default={Missing=not_a_fallback;};}" propertyList];
    domain = [@"{Vendor=domain_vendor; fixture={Compiler=domain_compiler; DomainOnly=domain_value;}; VM=domain_vm;}" propertyList];
    config = [NSJavaConfiguration defaultConfiguration];
    show("vendor", [config vendorName]);
    show("VM", [config valueForKey:NSDefaultJavaVMKey]);
    show("Compiler", [config valueForKey:NSDefaultJavaCompilerKey]);
    show("classpath", [config valueForKey:NSDefaultJavaClassPathKey]);
    show("domain", [config valueForKey:@"DomainOnly"]);
    show("missing", [config valueForKey:@"Missing"]);
    show("empty", [config valueForKey:@"Empty"]);
    show("expand", [config valueForKey:@"Expand"]);
    show("braces", [config valueForKey:@"Braces"]);
    show("adjacent", [config valueForKey:@"Adjacent"]);
    show("dollar", [config valueForKey:@"Dollar"]);
    show("unknown-vendor", [[NSJavaConfiguration configurationForVendor:@"unknown"] valueForKey:@"VM"]);
    show("nil-vendor", [[NSJavaConfiguration configurationForVendor:nil] vendorName]);
    show("init-vendor", [[[[NSJavaConfiguration alloc] init] autorelease] vendorName]);
    names = [NSJavaConfiguration vendorNames];
    for (i = 0; i < [names count]; ++i)
        if ([[names objectAtIndex:i] isEqual:@"fixture"]) ++duplicates;
    printf("vendor-duplicates=%u\n", duplicates);
    printf("vendor-key-removed=%d\n", ![names containsObject:@"Vendor"]);
    printf("top-key-present=%d\n", [names containsObject:@"Compiler"]);
    [pool release];
    return 0;
}
