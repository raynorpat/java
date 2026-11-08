#ifndef RHAPSODY_AWT_CLIPBOARD_H
#define RHAPSODY_AWT_CLIPBOARD_H
#import <AppKit/AppKit.h>
#include "jni.h"

/* DR2 fragile ABI: NSObject is 4 bytes. */
@interface AwtClipboard : NSObject {
    jobject _peer;
    int _changeCount;
}
- (id)initWithPeer:(int)peer;
- (int)claimClipboardOwnership;
- (NSString *)loadExternalContents;
- (BOOL)javaReadableContentsAvailable;
- (int)generalPasteboardCount;
+ (void)discardClipboard:(int)address;
@end
#endif
