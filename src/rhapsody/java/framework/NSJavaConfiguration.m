/* Reconstructed from DR2 JavaVM, configuration methods at 0x5cd01388–0x5cd01a60. */
#import "NSJavaConfiguration.h"
#include <ctype.h>

NSString *NSDefaultJavaVendorKey = @"Vendor";
NSString *NSDefaultJavaVMKey = @"VM";
NSString *NSDefaultJavaCompilerKey = @"Compiler";
NSString *NSDefaultJavaClassPathKey = @"DefaultClasspath";

@interface NSString (EnvironmentVariableExpansion)
- (NSString *)stringByExpandingEnvironmentVariables;
@end

@implementation NSString (EnvironmentVariableExpansion)
- (NSString *)stringByExpandingEnvironmentVariables
{
    NSDictionary *environment = [[NSProcessInfo processInfo] environment];
    const char *cursor = [self cString], *start;
    NSMutableString *result = [[NSMutableString alloc] init];
    NSString *value;
    while (isspace((unsigned char)*cursor)) ++cursor;
    while (*cursor) {
        start = cursor;
        while (*cursor && *cursor != '$') ++cursor;
        [result appendString:[NSString stringWithCString:start length:cursor-start]];
        if (!*cursor) break;
        ++cursor;
        if (*cursor == '{') {
            start = ++cursor;
            while (*cursor) {
                if (*cursor == ':' || *cursor == '}') {
                    while (*cursor && *cursor++ != '}') {}
                    break;
                }
                ++cursor;
            }
        } else {
            start = cursor;
            while (isalnum((unsigned char)*cursor) || *cursor == '_') ++cursor;
        }
        /* DR2 includes the closing brace in the key for ${...}. */
        value = [environment objectForKey:
            [NSString stringWithCString:start length:cursor-start]];
        if (value) [result appendString:value];
    }
    return [result autorelease];
}
@end

static NSString *configurationPath(void)
{
    static NSString *path;
    if (!path) {
        NSArray *libraries = NSSearchPathForDirectoriesInDomains(
            NSLibraryDirectory, NSSystemDomainMask, YES);
        if (![libraries count])
            [NSException raise:NSInternalInconsistencyException
                format:@"can't find /System/Library directory - NSSearchPathForDirectoriesInDomains() return nil."];
        path = [[[[libraries objectAtIndex:0] stringByAppendingPathComponent:@"Java"]
            stringByAppendingPathComponent:@"JavaConfig.plist"] retain];
    }
    return path;
}

static NSString *searchDictionary(NSDictionary *dictionary, NSString *key,
    NSString *vendor)
{
    NSString *value = nil;
    if (vendor) value = [[dictionary objectForKey:vendor] objectForKey:key];
    if (!value) value = [dictionary objectForKey:key];
    return [value stringByExpandingEnvironmentVariables];
}

static NSString *configurationValue(NSString *key, NSString *vendor)
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *value = searchDictionary([defaults objectForKey:@"JavaConfiguration"], key, vendor);
    if (!value) value = searchDictionary(
        [defaults persistentDomainForName:@"JavaConfiguration"], key, vendor);
    if (!value) value = searchDictionary(
        [NSDictionary dictionaryWithContentsOfFile:configurationPath()], key, vendor);
    return value;
}

@implementation NSJavaConfiguration
+ (NSJavaConfiguration *)defaultConfiguration
{
    return [[self class] configurationForVendor:configurationValue(NSDefaultJavaVendorKey, nil)];
}
+ (NSJavaConfiguration *)configurationForVendor:(NSString *)vendorName
{
    return [[[NSJavaConfiguration allocWithZone:NULL] initWithVendor:vendorName] autorelease];
}
+ (NSArray *)vendorNames
{
    NSMutableArray *names = [[NSMutableArray alloc] init];
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary *sources[3];
    unsigned i;
    sources[0] = [defaults objectForKey:@"JavaConfiguration"];
    sources[1] = [defaults persistentDomainForName:@"JavaConfiguration"];
    sources[2] = [NSDictionary dictionaryWithContentsOfFile:configurationPath()];
    for (i = 0; i < 3; ++i) {
        if (sources[i]) {
            [names addObjectsFromArray:[sources[i] allKeys]];
            [names removeObject:NSDefaultJavaVendorKey];
        }
    }
    return [names autorelease];
}
- init
{
    return [self initWithVendor:configurationValue(NSDefaultJavaVendorKey, nil)];
}
- initWithVendor:(NSString *)vendorName
{
    self = [super init];
    _vendorName = [vendorName retain];
    return self;
}
- (void)dealloc
{
    [_vendorName release];
    [super dealloc];
}
- (NSString *)vendorName
{
    return _vendorName;
}
- (NSString *)valueForKey:(NSString *)keyName
{
    return configurationValue(keyName, _vendorName);
}
@end
