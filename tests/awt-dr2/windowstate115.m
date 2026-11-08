#import <AppKit/AppKit.h>
#include <stdio.h>
#include <stdlib.h>
@interface NSObject (WindowStateTest)
- (id)initWithPeer:(void *)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(id)font backgroundColor:(id)background foregroundColor:(id)foreground cursor:(id)cursor;
- (id)initWithPeer:(void *)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(id)view;
- (id)initWithPeer:(void *)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(id)view resizable:(BOOL)resizable title:(id)title;
- (void *)peer;
- (void)awtSetFrame:(NSRect)frame display:(BOOL)display;
@end
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* DR2 uses the fragile Objective-C ABI: a subclass needs the full superclass size. */
@interface AwtWindow : NSWindow {
    void *_peer;
    id _menuBar;
    int _nativeWidgetCount;
    BOOL _isRealWindow;
}
@end
static int moved, resized;
@interface WindowStateProbe : AwtWindow
@end
@implementation WindowStateProbe
- (void)windowDidMove:(NSNotification *)notification { moved++; }
- (void)windowDidResize:(NSNotification *)notification { resized++; }
@end
int main(void)
{
    id pool = [[NSAutoreleasePool alloc] init];
    Class windowClass = NSClassFromString(@"WindowStateProbe");
    Class viewClass = NSClassFromString(@"AwtContainer");
    int mode;
    [NSApplication sharedApplication];
    CHECK(windowClass && viewClass);
    for (mode = 0; mode < 3; mode++) {
        NSRect frame = NSMakeRect(100, 120, 300, 180);
        unsigned int style = mode == 0 ? 0 : mode == 1 ? 7 : 15;
        NSRect content = [NSWindow contentRectForFrameRect:frame styleMask:style];
        NSView *view = [[viewClass alloc] initWithPeer:(void *)123 frame:NSMakeRect(0, 0, 300, 180)
            enabled:YES font:nil backgroundColor:[NSColor redColor] foregroundColor:nil cursor:nil];
        NSWindow *window;
        if (mode == 0)
            window = [[windowClass alloc] initWithPeer:(void *)123 frame:frame enabled:YES contentView:view];
        else
            window = [[windowClass alloc] initWithPeer:(void *)123 frame:frame enabled:YES contentView:view
                resizable:mode == 2 title:@"Native Frame"];
        CHECK([window peer] == (void *)123);
        CHECK(NSEqualRects([window frame], frame));
        CHECK([window styleMask] == style);
        CHECK([window contentView] == view && [window firstResponder] == view);
        CHECK([window delegate] == window);
        /* DR2 ignores this setter until a deferred window has a server number. */
        CHECK(![window acceptsMouseMovedEvents]);
        CHECK([view bounds].origin.x == content.origin.x - frame.origin.x);
        CHECK([view bounds].origin.y == NSMaxY(frame) - NSMaxY(content));
        CHECK(NSEqualSizes([view bounds].size, content.size));
        if (mode) CHECK([[window title] isEqualToString:@"Native Frame"]);
        frame = NSMakeRect(130, 140, 320, 200);
        moved = resized = 0;
        [window awtSetFrame:frame display:NO];
        CHECK(NSEqualRects([window frame], frame) && [window delegate] == window);
        CHECK(moved == 0 && resized == 0);
        frame.size.width += 10;
        [window setFrame:frame display:NO];
        CHECK(resized > 0);
        [window release];
        [view release];
    }
    [pool release];
    puts("DR2 native windows preserve frame geometry, content insets and responder setup");
    return 0;
}
