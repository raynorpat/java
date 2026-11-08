#import <AppKit/AppKit.h>
#include <stdio.h>
#include <stdlib.h>
@interface NSView (ViewTest)
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(id)font backgroundColor:(id)background foregroundColor:(id)foreground cursor:(id)cursor;
- (id)awtBackgroundColor;
- (id)awtForegroundColor;
- (id)awtFont;
- (void)awtSetBackgroundColor:(id)value;
- (void)awtSetForegroundColor:(id)value;
- (void)awtSetFont:(id)value;
- (void)awtSetEnabled:(BOOL)value;
- (BOOL)isAwtEnabled;
- (void)setWantsMouseMotionEvents:(BOOL)value;
- (BOOL)dispatchesAwtEventsOfType:(int)type;
- (void *)peer;
- (void)awtSetCursor:(id)value;
- (id)cursor;
@end
@interface NSWindow (ViewTest)
- (id)deepestViewUnderLocation:(NSPoint)location dispatchingEventsOfType:(int)type;
@end
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static int released;
@interface ViewValueProbe : NSObject
@end
@implementation ViewValueProbe
- (void)dealloc { released++; [super dealloc]; }
@end
int main(void)
{
    id pool = [[NSAutoreleasePool alloc] init];
    Class viewClass = NSClassFromString(@"AwtView");
    NSWindow *window;
    id parent, child, probe, intermediate;
    NSRect rect = NSMakeRect(0, 0, 100, 80);
    [NSApplication sharedApplication];
    CHECK(viewClass != Nil);
    window = [[NSWindow alloc] initWithContentRect:rect styleMask:0 backing:NSBackingStoreBuffered defer:YES];
    parent = [[viewClass alloc] initWithPeer:123 frame:rect enabled:YES font:[NSFont systemFontOfSize:12] backgroundColor:[NSColor redColor] foregroundColor:[NSColor blueColor] cursor:nil];
    child = [[viewClass alloc] initWithPeer:456 frame:rect enabled:YES font:nil backgroundColor:nil foregroundColor:nil cursor:nil];
    CHECK(![child isAwtEnabled]);
    [window setContentView:parent];
    [parent addSubview:child];
    CHECK([parent peer] == (void *)123 && [child peer] == (void *)456);
    CHECK([parent isFlipped] && [child isOpaque]);
    CHECK([child awtBackgroundColor] == [parent awtBackgroundColor]);
    CHECK([child awtForegroundColor] == [parent awtForegroundColor]);
    CHECK([child awtFont] == [parent awtFont]);
    [parent awtSetBackgroundColor:[NSColor greenColor]];
    CHECK([child awtBackgroundColor] == [parent awtBackgroundColor]);
    CHECK([child isAwtEnabled] && [child acceptsFirstResponder]);
    CHECK([child dispatchesAwtEventsOfType:1]);
    CHECK(![child dispatchesAwtEventsOfType:5]);
    [child setWantsMouseMotionEvents:YES];
    CHECK([child dispatchesAwtEventsOfType:5]);
    CHECK([window deepestViewUnderLocation:NSMakePoint(20, 20) dispatchingEventsOfType:1] == child);
    [child awtSetCursor:[NSCursor IBeamCursor]];
    CHECK([child cursor] == [NSCursor IBeamCursor]);
    [parent awtSetEnabled:NO];
    CHECK(![child isAwtEnabled] && ![child acceptsFirstResponder]);
    CHECK(![child dispatchesAwtEventsOfType:1]);
    CHECK([window deepestViewUnderLocation:NSMakePoint(20, 20) dispatchingEventsOfType:1] == nil);
    CHECK([child dispatchesAwtEventsOfType:100] && [child dispatchesAwtEventsOfType:101]);
    CHECK(![child dispatchesAwtEventsOfType:0] && ![child dispatchesAwtEventsOfType:102]);
    [parent awtSetEnabled:YES];
    [child awtSetEnabled:NO];
    CHECK([parent isAwtEnabled] && ![child isAwtEnabled]);
    [child removeFromSuperview];
    intermediate = [[NSView alloc] initWithFrame:rect];
    [parent addSubview:intermediate];
    [intermediate addSubview:child];
    [child awtSetEnabled:YES];
    CHECK([child isAwtEnabled]);
    CHECK([child awtBackgroundColor] == nil && [child awtForegroundColor] == nil && [child awtFont] == nil);
    [intermediate release];
    probe = [ViewValueProbe new];
    [child awtSetFont:probe];
    [child awtSetForegroundColor:probe];
    [probe release];
    CHECK(released == 0);
    [child removeFromSuperview];
    [child release];
    [window release];
    [parent release];
    [pool release];
    CHECK(released == 1);
    puts("DR2 native views preserve inherited state and release owned values");
    return 0;
}
