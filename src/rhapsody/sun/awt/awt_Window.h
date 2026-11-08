#ifndef RHAPSODY_AWT_WINDOW_H
#define RHAPSODY_AWT_WINDOW_H

#import <AppKit/AppKit.h>
#include "jni.h"

@interface AwtWindow : NSWindow {
    jobject _peer;
    id _menuBar;
    int _nativeWidgetCount;
    BOOL _isRealWindow;
}
- (id)commonInitWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled styleMask:(unsigned int)style contentView:(NSView *)view;
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view;
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view resizable:(BOOL)resizable title:(NSString *)title;
- (jobject)peer;
- (void)awtSetFrame:(NSRect)frame display:(BOOL)display;
- (void)notifyJavaFrameChanged;
- (void)postWindowEvent:(int)type;
@end

#endif
