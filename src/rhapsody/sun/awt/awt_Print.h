#ifndef RHAPSODY_AWT_PRINT_H
#define RHAPSODY_AWT_PRINT_H
#import "awt_Graphics.h"
@interface AwtPrintGraphics : AwtGraphics
- (id)initWithPrintSurface:(id)surface;
- (void)printPage;
@end
@interface AwtPrintJob : NSObject {
    NSPrintOperation *_printOp;
}
- (id)initWithTitle:(NSString *)title disposition:(int)disposition destination:(NSString *)destination copies:(int)copies orientation:(int)orientation paperName:(NSString *)paper;
- (int)showPrintDialog;
- (void)cleanUp;
- (NSPrintOperation *)printOperation;
+ (void)discardPrintJob:(int)address;
@end
@interface AwtPrintSurface : NSObject
- (NSRect)awtBounds;
- (NSRect)awtVisibleBounds;
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (NSColor *)awtColor;
@end
#endif
