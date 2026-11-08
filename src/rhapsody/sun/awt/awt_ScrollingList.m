/* DR2 libawt.A.dylib: AwtScrollingList 5400ac8c--5400bde8. */
#import "awt_ScrollingList.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#include <math.h>

@implementation AwtTableView
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame
{ self = [super initWithFrame:frame]; _peer = peer; return self; }
- (jobject)peer { return _peer; }
- (void)setPeer:(jobject)peer { _peer = peer; }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type == 100 || type == 101) return _peer != NULL;
    if (!_peer) return NO;
    if (type == 6 || type == 7) return _wantsMouseMotionEvents && [self isAwtEnabled];
    if ((type >= 1 && type <= 4) || (type >= 10 && type <= 12)) return [self isAwtEnabled];
    return NO;
}
- (BOOL)becomeFirstResponder
{ BOOL result = [super becomeFirstResponder]; if (result) [self dispatchFocusEventGained:YES temporary:NO]; return result; }
- (BOOL)resignFirstResponder
{ BOOL result = [super resignFirstResponder]; if (result) [self dispatchFocusEventGained:NO temporary:NO]; return result; }
- (BOOL)acceptsFirstResponder { return [self isEnabled]; }
- (void)mouseDown:(NSEvent *)event { if ([self isEnabled]) [super mouseDown:event]; }
@end

@interface ItemPair : NSObject {
    NSString *title;
    BOOL selected;
}
+ (id)newWithTitle:(NSString *)value selected:(BOOL)state;
- (id)initWithTitle:(NSString *)value selected:(BOOL)state;
- (BOOL)isSelected;
- (void)setSelected:(BOOL)state;
- (NSString *)title;
@end
@implementation ItemPair
+ (id)newWithTitle:(NSString *)value selected:(BOOL)state
{ return [[[self alloc] initWithTitle:value selected:state] autorelease]; }
- (id)initWithTitle:(NSString *)value selected:(BOOL)state
{ self = [super init]; title = [value retain]; selected = state; return self; }
- (void)dealloc { [title release]; [super dealloc]; }
- (BOOL)isSelected { return selected; }
- (void)setSelected:(BOOL)state { selected = state; }
- (NSString *)title { return title; }
@end

@implementation AwtScrollingList
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor allowMultipleSelection:(BOOL)multiple
{
    NSTableColumn *column;
    self = [super initWithFrame:frame]; _peer = (jobject)peer;
    _tableView = [[AwtTableView alloc] initWithPeer:_peer frame:frame];
    _tableDataCell = [[NSTextFieldCell alloc] init];
    [_tableView setHeaderView:nil]; [_tableView setAllowsColumnReordering:NO];
    [_tableView setAllowsColumnResizing:NO]; [_tableView setDrawsGrid:NO];
    [_tableView setIntercellSpacing:NSMakeSize(0, 0)];
    column = [[[NSTableColumn alloc] init] autorelease];
    [column setEditable:NO]; [column setResizable:YES]; [column setDataCell:_tableDataCell];
    [_tableView addTableColumn:column]; [_tableDataCell setEditable:NO];
    [_tableView setTarget:self]; [_tableView setDoubleAction:@selector(doubleClick:)];
    [_tableView setDataSource:self]; [_tableView setDelegate:self];
    _items = [[NSMutableArray alloc] init];
    [self setDocumentView:_tableView]; [self setHasVerticalScroller:YES];
    [self setBorderType:NSBezelBorder]; [_tableView sizeToFit];
    if (![self awtComponentGetsBackgroundFromItsContainer]) [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground]; [self awtSetFont:font];
    [self awtSetCursor:cursor]; [self awtSetEnabled:enabled];
    [self awtSetAllowsMultipleSelection:multiple];
    return self;
}
- (void)dealloc
{ [_items release]; [_tableDataCell release]; [_tableView release]; [_cursor release]; [super dealloc]; }
- (jobject)peer { return _peer; }
- (void)clearPeer
{
    AwtContainer *container = [self outermostJavaContainer];
    if ([container lastJavaViewEntered] == self || [container lastJavaViewEntered] == _tableView)
        [container setLastJavaViewEntered:nil];
    [_tableView setPeer:NULL]; [_tableView setDelegate:nil];
    [_tableView setDataSource:nil]; [_tableView setTarget:nil]; _peer = NULL;
}
- (void)awtSetBackgroundColor:(NSColor *)color
{
    if (color) { [_tableDataCell setBackgroundColor:color]; [self setBackgroundColor:color]; }
    [_tableDataCell setDrawsBackground:color != nil];
}
- (void)awtSetForegroundColor:(NSColor *)color { if (color) [_tableDataCell setTextColor:color]; }
- (void)awtSetFont:(NSFont *)font
{ if (font) [_tableDataCell setFont:font]; [_tableView setRowHeight:[_tableDataCell cellSize].height]; }
- (void)awtSetAllowsMultipleSelection:(BOOL)multiple { [_tableView setAllowsMultipleSelection:multiple]; }
- (void)awtSetEnabled:(BOOL)enabled
{
    _awtEnabled = enabled;
    [[self verticalScroller] setEnabled:enabled]; [_tableDataCell setEnabled:enabled];
    [_tableView setEnabled:enabled];
    if (_cursor) [[self window] invalidateCursorRectsForView:self];
    if (!enabled) [self resignFirstResponderIfNecessary];
}
- (void)awtSetCursor:(NSCursor *)cursor
{
    [_cursor autorelease];
    _cursor = [cursor retain];
    [_cursor setOnMouseEntered:YES];
    [[self window] invalidateCursorRectsForView:self];
}
- (void)resetCursorRects
{
    if (_cursor && [self isAwtEnabled]) [self addCursorRect:[self convertRect:[[self contentView] bounds] fromView:[self contentView]] cursor:_cursor];
    if (_trackingRectTag) [self removeTrackingRect:_trackingRectTag];
    _trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
- (BOOL)isAwtEnabled { return _awtEnabled && [[self superview] isAwtEnabled]; }
- (BOOL)isFlipped { return YES; }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type >= 100 && type <= 101) return YES;
    if (type < 1 || type > 12) return NO;
    if (type >= 5 && type <= 7 && !_wantsMouseMotionEvents) return NO;
    return [self isAwtEnabled];
}
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (NSView *)hitTest:(NSPoint)point { return [self isAwtEnabled] ? [super hitTest:point] : nil; }
- (BOOL)componentIsFocusTraversable { return [_tableView acceptsFirstResponder]; }
- (void)makeComponentTakeFocus { [[self window] makeFirstResponder:_tableView]; }
- (NSSize)preferredSizeForNumRows:(int)rows
{
    NSSize content;
    [_tableDataCell setStringValue:@"0123456789abcde"];
    content = [_tableDataCell cellSize]; content.height *= rows;
    return [[self class] frameSizeForContentSize:content hasHorizontalScroller:NO hasVerticalScroller:YES borderType:[self borderType]];
}
- (NSArray *)selectedIndexes
{
    NSMutableArray *result = [[[NSMutableArray alloc] init] autorelease];
    unsigned int index;
    for (index = 0; index < [_items count]; index++)
        if ([[_items objectAtIndex:index] isSelected]) [result addObject:[NSNumber numberWithInt:index]];
    return result;
}
- (void)doubleClick:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv]; jclass cls; jmethodID method; jobject event;
    if (!_peer) return;
    [[self retain] autorelease];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RListPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "makeActionEvent", "(I)Ljava/awt/event/ActionEvent;");
    if (!method) [AwtException raiseUsingEnv:env];
    event = (*env)->CallObjectMethod(env, _peer, method, [sender clickedRow]);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_DURING
        [NSApp dispatchJavaEvent:event isLocalRef:YES];
    NS_HANDLER
        (*env)->DeleteLocalRef(env, event);
        [localException raise];
    NS_ENDHANDLER
    (*env)->DeleteLocalRef(env, event);
}
- (void)updateForNewSelection
{
    JNIEnv *env = [NSApp appkitEnv];
    NSEnumerator *rows = [_tableView selectedRowEnumerator];
    NSNumber *next = [rows nextObject];
    unsigned int index;
    [[self retain] autorelease];
    for (index = 0; _peer && index < [_items count]; index++) {
        ItemPair *item = [_items objectAtIndex:index];
        BOOL selected = next && [next intValue] == index;
        if (selected != [item isSelected]) {
            jclass cls; jmethodID method; jobject event;
            [item setSelected:selected];
            cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RListPeer");
            if (!cls) [AwtException raiseUsingEnv:env];
            method = (*env)->GetMethodID(env, cls, "makeItemEvent", "(IZ)Ljava/awt/event/ItemEvent;");
            if (!method) [AwtException raiseUsingEnv:env];
            event = (*env)->CallObjectMethod(env, _peer, method, index, selected);
            (*env)->DeleteLocalRef(env, cls);
            if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
            NS_DURING
                [NSApp dispatchJavaEvent:event isLocalRef:YES];
            NS_HANDLER
                (*env)->DeleteLocalRef(env, event);
                [localException raise];
            NS_ENDHANDLER
            (*env)->DeleteLocalRef(env, event);
        }
        if (selected) next = [rows nextObject];
    }
}
- (void)tableViewSelectionIsChanging:(NSNotification *)notification { [self updateForNewSelection]; }
- (void)remarkSelection
{
    unsigned int index;
    [_tableView setDelegate:nil]; [_tableView deselectAll:nil];
    for (index = 0; index < [_items count]; index++)
        if ([[_items objectAtIndex:index] isSelected])
            [_tableView selectRow:index byExtendingSelection:[_tableView allowsMultipleSelection]];
    [_tableView setDelegate:self];
}
- (void)insertItemWithTitle:(NSString *)title atIndex:(int)index
{
    ItemPair *item = [ItemPair newWithTitle:title selected:NO];
    if (index < 0) [_items addObject:item];
    else { [_items insertObject:item atIndex:index]; [self remarkSelection]; }
    [_tableView reloadData];
}
- (void)removeItemsInRange:(NSRange)range
{ [_items removeObjectsInRange:range]; [self remarkSelection]; [_tableView reloadData]; }
- (void)removeAllItems { [_items removeAllObjects]; [_tableView reloadData]; }
- (void)selectItemAtIndex:(int)index
{
    [_tableView selectRow:index byExtendingSelection:[_tableView allowsMultipleSelection]];
    [[_items objectAtIndex:index] setSelected:YES]; [_tableView scrollRowToVisible:index];
}
- (void)deselectItemAtIndex:(int)index
{ [_tableView deselectRow:index]; [[_items objectAtIndex:index] setSelected:NO]; }
- (void)scrollRowToVisible:(int)index { [_tableView scrollRowToVisible:index]; }
- (int)numberOfRowsInTableView:(NSTableView *)table { return [_items count]; }
- (id)tableView:(NSTableView *)table objectValueForTableColumn:(NSTableColumn *)column row:(int)row
{ return [[_items objectAtIndex:row] title]; }
@end
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RListPeer_createNativeList(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jboolean multiple)
{
    AwtScrollingList *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *nativeFont = javaToNSFont(env, font);
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtScrollingList alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtScrollingList"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor allowMultipleSelection:multiple];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_add(JNIEnv *env, jobject peer, jstring title, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) insertItemWithTitle:javaToNSString(env, title) atIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_delItems(JNIEnv *env, jobject peer, jint first, jint last)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) removeItemsInRange:NSMakeRange(first, last - first + 1)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_removeAll(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) removeAllItems];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_nSelect(JNIEnv *env, jobject peer, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) selectItemAtIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_deselect(JNIEnv *env, jobject peer, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) deselectItemAtIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_makeVisible(JNIEnv *env, jobject peer, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) scrollRowToVisible:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RListPeer_setMultipleMode(JNIEnv *env, jobject peer, jboolean multiple)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetAllowsMultipleSelection:multiple];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RListPeer_getPreferredSize(JNIEnv *env, jobject peer, jint rows)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSSize size = [viewFromPeer(env, peer) preferredSizeForNumRows:rows];
        jclass cls = (*env)->FindClass(env, "java/awt/Dimension");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        /* 54017e8c requests upward rounding from nsSizeToJavaDimension. */
        result = (*env)->NewObject(env, cls, ctor, (int)ceil(size.width), (int)ceil(size.height));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
/* 54012d40/54012e20: focus requests honor inherited enabled state. */

JNIEXPORT jintArray JNICALL Java_com_apple_rhapsody_awt_RListPeer_getSelectedIndexes(JNIEnv *env, jobject peer)
{
    jintArray result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSArray *indexes = [viewFromPeer(env, peer) selectedIndexes];
        unsigned int index, count = [indexes count];
        result = (*env)->NewIntArray(env, count);
        if (!result) [AwtException raiseUsingEnv:env];
        for (index = 0; index < count; index++) {
            jint value = [[indexes objectAtIndex:index] intValue];
            (*env)->SetIntArrayRegion(env, result, index, 1, &value);
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
