#ifndef RHAPSODY_AWT_MENU_H
#define RHAPSODY_AWT_MENU_H
#import "awt_Window.h"

/* DR2 fragile ABI: NSMenu is 28 bytes; NSMenuItem is 64 bytes. */
@interface AwtMenu : NSMenu {
    jobject _peer;
    BOOL _isHelpMenu;
    NSView *_currentlyPoppedUpInView;
}
- (id)initWithPeer:(int)peer withLabel:(NSString *)label tearOff:(BOOL)tearOff isEnabled:(BOOL)enabled isHelpMenu:(BOOL)help withFont:(NSFont *)font inMenuComponent:(id)parent;
- (jobject)peer;
- (void)clearPeer;
- (void)awtDispose;
- (void)awtAddItem:(NSMenuItem *)item;
- (void)awtAddMenu:(AwtMenu *)menu isEnabled:(BOOL)enabled;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)awtSetLabel:(NSString *)label;
- (BOOL)isHelpMenu;
- (void)updateFont;
- (int)_newItemInsertionIndex;
- (void)awtShowPopupForView:(NSView *)view atLocation:(NSPoint)point;
+ (void)updateFontAtNextUpdate;
+ (void)discardComponent:(int)address;
@end

@interface AwtMenuBar : NSMenu {
    jobject _peer;
    AwtWindow *_window;
}
- (id)initWithPeer:(int)peer withMenuSet:(int)set withFont:(NSFont *)font inWindow:(AwtWindow *)window;
- (jobject)peer;
- (void)clearPeer;
- (void)awtDispose;
- (void)awtAddMenu:(AwtMenu *)menu isEnabled:(BOOL)enabled;
- (void)addMenuBarToWindow:(AwtWindow *)window;
- (void)removeMenuBarFromWindow:(AwtWindow *)window;
- (void)hideMenuBarForWindow:(AwtWindow *)window;
+ (void)showMenuBarForWindow:(AwtWindow *)window menubar:(AwtMenuBar *)bar;
@end

@interface AwtMenuItem : NSMenuItem {
    jobject _peer;
    BOOL _isCheckBoxType;
}
- (id)commonInitWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift action:(SEL)action withFont:(NSFont *)font inMenu:(AwtMenu *)menu;
- (id)initWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift withFont:(NSFont *)font inMenu:(AwtMenu *)menu;
- (id)initWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift withState:(BOOL)state withFont:(NSFont *)font inMenu:(AwtMenu *)menu;
- (void)addSeparatorWithPeer:(int)peer inMenu:(AwtMenu *)menu;
- (jobject)peer;
- (BOOL)isCheckBoxMenuItem;
- (void)clearPeer;
- (void)awtDispose;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)awtSetLabel:(NSString *)label withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift;
- (void)awtSetState:(BOOL)state;
- (void)awtToggleState:(BOOL)notify;
- (void)updateFontWithIndex:(int)index;
- (void)_setUsesShiftModifier:(BOOL)shift;
@end
#endif
