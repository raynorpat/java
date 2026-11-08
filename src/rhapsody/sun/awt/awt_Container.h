#ifndef RHAPSODY_AWT_CONTAINER_H
#define RHAPSODY_AWT_CONTAINER_H

#import "awt_Canvas.h"

@interface AwtContainer : AwtCanvas {
    BOOL _clipSubviews;
    NSView *_lastViewEntered;
    int _nativeSubviewCount;
}
- (void)_clipAroundSubviews;
- (NSView *)lastJavaViewEntered;
- (void)setLastJavaViewEntered:(NSView *)view;
@end

#endif
