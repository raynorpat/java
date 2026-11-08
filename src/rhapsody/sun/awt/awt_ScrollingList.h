#ifndef RHAPSODY_AWT_SCROLLINGLIST_H
#define RHAPSODY_AWT_SCROLLINGLIST_H
#import "awt_View.h"

/* DR2 fragile ABI: NSTableView is 228 bytes. */
@interface AwtTableView : NSTableView {
    jobject _peer;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame;
- (jobject)peer;
- (void)setPeer:(jobject)peer;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
@end

/* DR2 fragile ABI: NSScrollView is 120 bytes. */
@interface AwtScrollingList : NSScrollView {
    jobject _peer;
    NSMutableArray *_items;
    AwtTableView *_tableView;
    NSTextFieldCell *_tableDataCell;
    BOOL _awtEnabled;
    NSCursor *_cursor;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor allowMultipleSelection:(BOOL)multiple;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetFont:(NSFont *)font;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)awtSetAllowsMultipleSelection:(BOOL)multiple;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (NSSize)preferredSizeForNumRows:(int)rows;
- (NSArray *)selectedIndexes;
- (void)updateForNewSelection;
- (void)remarkSelection;
- (void)insertItemWithTitle:(NSString *)title atIndex:(int)index;
- (void)removeItemsInRange:(NSRange)range;
- (void)removeAllItems;
- (void)selectItemAtIndex:(int)index;
- (void)deselectItemAtIndex:(int)index;
- (void)scrollRowToVisible:(int)index;
@end
#endif
