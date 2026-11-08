/* Reconstructed Rhapsody DR2 Java configuration interface. */
#import <Foundation/Foundation.h>

extern NSString *NSDefaultJavaVendorKey;
extern NSString *NSDefaultJavaVMKey;
extern NSString *NSDefaultJavaCompilerKey;
extern NSString *NSDefaultJavaClassPathKey;

@interface NSJavaConfiguration : NSObject
{
    NSString *_vendorName;
}
+ (NSJavaConfiguration *)defaultConfiguration;
+ (NSJavaConfiguration *)configurationForVendor:(NSString *)vendorName;
+ (NSArray *)vendorNames;
- init;
- initWithVendor:(NSString *)vendorName;
- (NSString *)vendorName;
- (NSString *)valueForKey:(NSString *)keyName;
@end
