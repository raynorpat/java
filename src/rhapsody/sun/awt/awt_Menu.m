/* DR2 libawt.A.dylib: menus 540079b8--54009350. */
#import "awt_Menu.h"
#import "awt_Application.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Dispatcher.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

extern NSString *NSAppleMenuName, *NSWindowsMenuName, *NSServicesMenuName;
extern NSString *_NSWindowsMenuName, *_NSServicesMenuName;
@interface NSMenu (AWTMenuAccess)
- (NSString *)_menuName;
- (void)_setMenuName:(NSString *)name;
- (int)_newMenuInsertionIndex;
- (void)_popUpContextMenu:(NSMenu *)menu withEvent:(NSEvent *)event forView:(NSView *)view;
@end
@interface NSObject (AWTMenuCellAccess)
- (id)menuItemCellForItemAtIndex:(int)index;
@end
@interface NSInputManager (AWTMenuAccess)
+ (NSMenu *)inputManagerMenu;
@end

static AwtMenu *lastUpdatedMenu;
static NSFont *menuFont(jobject peer)
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RMenuComponentPeer");
    jmethodID method; jobject font; NSFont *result;
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "getMenuFont", "()Ljava/awt/Font;");
    if (!method) [AwtException raiseUsingEnv:env];
    font = (*env)->CallObjectMethod(env, peer, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    result = javaToNSFont(env, font);
    if (font) (*env)->DeleteLocalRef(env, font);
    return result ? result : [NSFont menuFontOfSize:-1];
}
static BOOL isJavaItem(NSMenuItem *item)
{ return [item isKindOfClass:[AwtMenuItem class]] || [[item submenu] isKindOfClass:[AwtMenu class]]; }

@implementation NSMenu (AWTInsertion)
/* 54008c44: insert Java menus before system window/services/input menus. */
- (int)_newMenuInsertionIndex
{
    int index, count = [self numberOfItems];
    for (index = 0; index < count; index++) {
        NSMenu *menu = [[self itemAtIndex:index] submenu];
        NSString *name = [menu _menuName];
        if (menu && ([name isEqual:NSWindowsMenuName] || [name isEqual:_NSWindowsMenuName] ||
            [name isEqual:NSServicesMenuName] || [name isEqual:_NSServicesMenuName] ||
            [[menu title] isEqual:[[NSInputManager inputManagerMenu] title]])) return index;
    }
    return count;
}
@end

@implementation AwtMenu
- (id)initWithPeer:(int)peer withLabel:(NSString *)label tearOff:(BOOL)tearOff isEnabled:(BOOL)enabled isHelpMenu:(BOOL)help withFont:(NSFont *)font inMenuComponent:(id)parent
{
    self = [super initWithTitle:label]; _peer = (jobject)peer; _isHelpMenu = help;
    [self setAutoenablesItems:NO];
    if (font) [[self menuRepresentation] setFont:font];
    [parent awtAddMenu:self isEnabled:enabled];
    return self;
}
- (jobject)peer { return _peer; }
- (void)clearPeer
{
    _peer = NULL;
    if (lastUpdatedMenu == self) lastUpdatedMenu = nil;
    [_currentlyPoppedUpInView release]; _currentlyPoppedUpInView = nil;
}
- (void)awtDispose
{
    NSMenu *parent = [self supermenu];
    if (parent) {
        NSMenuItem *item = [parent itemAtIndex:[parent indexOfItemWithSubmenu:self]];
        [item setSubmenu:nil]; [parent removeItem:item];
    }
}
+ (void)discardComponent:(int)address
{ id component = (id)address; [component awtDispose]; [component clearPeer]; [component release]; }
- (void)awtAddItem:(NSMenuItem *)item { [self insertItem:item atIndex:[self _newItemInsertionIndex]]; }
- (void)awtAddMenu:(AwtMenu *)menu isEnabled:(BOOL)enabled
{
    NSMenuItem *item = [self insertItemWithTitle:[menu title] action:NULL keyEquivalent:@"" atIndex:[self _newItemInsertionIndex]];
    [item setSubmenu:menu]; [item setEnabled:enabled];
}
- (void)awtSetEnabled:(BOOL)enabled
{ NSMenu *parent = [self supermenu]; [[parent itemAtIndex:[parent indexOfItemWithSubmenu:self]] setEnabled:enabled]; }
- (void)awtSetLabel:(NSString *)label
{
    NSMenu *parent = [self supermenu];
    [[parent itemAtIndex:[parent indexOfItemWithSubmenu:self]] setTitle:label];
    if (![parent supermenu]) [[parent menuRepresentation] sizeToFit];
    [self setTitle:label];
}
- (BOOL)isHelpMenu { return _isHelpMenu; }
- (int)_newItemInsertionIndex
{
    int index;
    for (index = [self numberOfItems] - 1; index >= 0; index--)
        if (isJavaItem([self itemAtIndex:index])) return index + 1;
    return 0;
}
- (void)update
{
    if (_peer && lastUpdatedMenu != self) { [self updateFont]; lastUpdatedMenu = self; }
    [super update];
}
+ (void)updateFontAtNextUpdate { lastUpdatedMenu = nil; }
- (void)updateFont
{
    NSFont *font = menuFont(_peer);
    id representation = [self menuRepresentation];
    BOOL changed = font != [representation font];
    NSArray *items = [self itemArray]; unsigned int index;
    if (changed) [representation setFont:font];
    for (index = 0; index < [items count]; index++) {
        id item = [items objectAtIndex:index];
        if ([item respondsToSelector:@selector(updateFontWithIndex:)]) [item updateFontWithIndex:index];
        else if (changed) { [[representation menuItemCellForItemAtIndex:index] setFont:font]; [self itemChanged:item]; }
    }
}
- (void)awtShowPopupForView:(NSView *)view atLocation:(NSPoint)point
{
    if (!_currentlyPoppedUpInView) {
        NSEvent *event;
        _currentlyPoppedUpInView = [view retain];
        event = [NSEvent mouseEventWithType:NSLeftMouseDown location:[view convertPoint:point toView:nil]
            modifierFlags:0 timestamp:0 windowNumber:[[view window] windowNumber] context:nil
            eventNumber:0 clickCount:1 pressure:1];
        [self performSelector:@selector(popUpMenuWithEvent:) withObject:event afterDelay:0.1];
    }
}
- (void)popUpMenuWithEvent:(NSEvent *)event
{
    NSView *view = _currentlyPoppedUpInView;
    if (!_peer) return;
    _currentlyPoppedUpInView = nil;
    NS_DURING
        [self _popUpContextMenu:self withEvent:event forView:view];
    NS_HANDLER
        [view release]; [localException raise];
    NS_ENDHANDLER
    [view release];
}
@end

@implementation AwtMenuBar
- (id)initWithPeer:(int)peer withMenuSet:(int)set withFont:(NSFont *)font inWindow:(AwtWindow *)window
{
    int index; NSString *titles[] = {@"", @"Windows", @"Services"};
    NSString *names[] = {NSAppleMenuName, NSWindowsMenuName, NSServicesMenuName};
    self = [super initWithTitle:@""]; _peer = (jobject)peer;
    for (index = 0; index < 3; index++) if (set & (1 << index)) {
        NSMenuItem *item = [self addItemWithTitle:titles[index] action:NULL keyEquivalent:@""];
        NSMenu *menu = [[[NSMenu alloc] initWithTitle:[item title]] autorelease];
        [item setSubmenu:menu];
        if (index == 0) [item setImage:[NSImage imageNamed:@"NSAppleMenuImage"]];
        [menu _setMenuName:names[index]];
    }
    [self setAutoenablesItems:NO];
    if (font) [[self menuRepresentation] setFont:font];
    [window awtSetMenuBar:self];
    return self;
}
- (jobject)peer { return _peer; }
- (void)clearPeer { _peer = NULL; }
- (void)dealloc { [_window release]; [super dealloc]; }
- (void)awtDispose
{
    [_window awtSetMenuBar:nil];
    if ([NSApp mainMenu] == self) [NSApp setMainMenu:nil];
}
- (void)awtAddMenu:(AwtMenu *)menu isEnabled:(BOOL)enabled
{
    NSMenuItem *item = [menu isHelpMenu] ? [self addItemWithTitle:[menu title] action:NULL keyEquivalent:@""] :
        [self insertItemWithTitle:[menu title] action:NULL keyEquivalent:@"" atIndex:[self _newMenuInsertionIndex]];
    [item setEnabled:enabled]; [item setSubmenu:menu];
}
- (void)update { [AwtMenu updateFontAtNextUpdate]; [super update]; }
- (void)addMenuBarToWindow:(AwtWindow *)window
{
    [_window autorelease]; _window = [window retain];
    if ([window isKeyWindow]) [AwtMenuBar showMenuBarForWindow:window menubar:self];
}
- (void)removeMenuBarFromWindow:(AwtWindow *)window
{
    if ([window isKeyWindow]) [self hideMenuBarForWindow:window];
    [_window autorelease]; _window = nil;
}
+ (void)showMenuBarForWindow:(AwtWindow *)window menubar:(AwtMenuBar *)bar
{
    NSMenu *current = [NSApp mainMenu];
    if (current && ![current isKindOfClass:[AwtMenuBar class]]) {
        int insertion = [current _newMenuInsertionIndex], index, source = 0, count = [bar numberOfItems];
        for (index = 0; index < count; index++) {
            NSMenuItem *item = [bar itemAtIndex:source];
            if (isJavaItem(item)) {
                [item retain]; [[item menu] removeItem:item];
                if ([(AwtMenu *)[item submenu] isHelpMenu]) [current addItem:item];
                else [current insertItem:item atIndex:insertion++];
                [item release];
            } else source++;
        }
    } else if (current != bar) {
        NSMenuItem *info = [bar itemWithTitle:@"Info"];
        if (info) [info setTitle:@"Infu"];
        [NSApp setMainMenu:bar];
        if (info) [info setTitle:@"Info"];
    }
}
- (void)hideMenuBarForWindowMaybe:(AwtWindow *)window
{
    if ([NSApp mainMenu] == self && (![window isKeyWindow] || ![NSApp isActive]) && ![window counterpartIsKey])
        [NSApp setMainMenu:nil];
}
- (void)hideMenuBarForWindow:(AwtWindow *)window
{
    NSMenu *current = [NSApp mainMenu];
    if (current == self) [self performSelector:@selector(hideMenuBarForWindowMaybe:) withObject:window afterDelay:0.2];
    else {
        int index;
        for (index = [current numberOfItems] - 1; index >= 0; index--) {
            NSMenuItem *item = [current itemAtIndex:index];
            if (isJavaItem(item)) { [item retain]; [current removeItem:item]; [self insertItem:item atIndex:0]; [item release]; }
        }
    }
}
@end

@implementation AwtMenuItem
- (id)commonInitWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift action:(SEL)action withFont:(NSFont *)font inMenu:(AwtMenu *)menu
{
    self = [super initWithTitle:label action:action keyEquivalent:shortcut ? shortcut : @""];
    _peer = (jobject)peer; [self _setUsesShiftModifier:shift];
    [self setEnabled:enabled]; [self setTarget:self]; [menu awtAddItem:self];
    if (font) [[[menu menuRepresentation] menuItemCellForItemAtIndex:[menu numberOfItems] - 1] setFont:font];
    return self;
}
- (id)initWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift withFont:(NSFont *)font inMenu:(AwtMenu *)menu
{
    _isCheckBoxType = NO;
    return [self commonInitWithPeer:peer withLabel:label isEnabled:enabled withShortcut:shortcut usesShiftModifier:shift action:@selector(menuItemSelected:) withFont:font inMenu:menu];
}
- (id)initWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift withState:(BOOL)state withFont:(NSFont *)font inMenu:(AwtMenu *)menu
{
    _isCheckBoxType = YES;
    self = [self commonInitWithPeer:peer withLabel:label isEnabled:enabled withShortcut:shortcut usesShiftModifier:shift action:@selector(checkboxMenuItemSelected:) withFont:font inMenu:menu];
    [self setState:state]; return self;
}
- (jobject)peer { return _peer; }
- (BOOL)isCheckBoxMenuItem { return _isCheckBoxType; }
- (void)clearPeer { _peer = NULL; [self setTarget:nil]; }
- (void)addSeparatorWithPeer:(int)peer inMenu:(AwtMenu *)menu { _peer = (jobject)peer; [menu awtAddItem:self]; }
- (void)awtDispose { [[self menu] removeItem:self]; }
- (void)awtSetEnabled:(BOOL)enabled { [self setEnabled:enabled]; }
- (void)awtSetLabel:(NSString *)label withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift
{ [self setTitle:label]; [self setKeyEquivalent:shortcut ? shortcut : @""]; [self _setUsesShiftModifier:shift]; }
- (void)awtSetState:(BOOL)state { [self setState:state]; }
- (void)_setUsesShiftModifier:(BOOL)shift
{
    unsigned int mask = [self keyEquivalentModifierMask];
    [self setKeyEquivalentModifierMask:shift ? mask | NSShiftKeyMask : mask & ~NSShiftKeyMask];
}
- (void)awtToggleState:(BOOL)notify
{
    [self setState:[self state] != 1];
    if (notify && _peer) {
        JNIEnv *env = [NSApp appkitEnv];
        jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RCheckboxMenuItemPeer");
        jmethodID method;
        if (!cls) [AwtException raiseUsingEnv:env];
        method = (*env)->GetMethodID(env, cls, "updateCounterpartState", "(Z)V");
        if (!method) [AwtException raiseUsingEnv:env];
        (*env)->CallVoidMethod(env, _peer, method, [self state]);
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    }
}
- (void)menuItemSelected:(id)sender
{
    if (!_peer) return;
    [[self retain] autorelease];
    [[AwtDispatcher defaultDispatcher] dispatchActionEventToMenuItemPeer:_peer];
}
- (void)checkboxMenuItemSelected:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv]; jclass cls; jmethodID method; jobject event;
    if (!_peer) return;
    [[self retain] autorelease]; [self setState:[self state] != 1];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RCheckboxMenuItemPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "makeItemEvent", "(Z)Ljava/awt/event/ItemEvent;");
    if (!method) [AwtException raiseUsingEnv:env];
    event = (*env)->CallObjectMethod(env, _peer, method, [self state]);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    [NSApp dispatchJavaEvent:event isLocalRef:YES];
}
- (void)updateFontWithIndex:(int)index
{
    if (_peer) {
        NSFont *font = menuFont(_peer);
        id cell = [[[self menu] menuRepresentation] menuItemCellForItemAtIndex:index];
        if (cell && font != [cell font]) { [cell setFont:font]; [[self menu] itemChanged:self]; }
    }
}
@end

static jfieldID menuField(JNIEnv *env)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RMenuComponentPeer");
    jfieldID field;
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "nNativeMenuComponent", "I");
    (*env)->DeleteLocalRef(env, cls);
    if (!field) [AwtException raiseUsingEnv:env];
    return field;
}
static id realMenuComponent(JNIEnv *env, jobject peer)
{ return peer ? (id)(*env)->GetIntField(env, peer, menuField(env)) : nil; }
static id menuComponent(JNIEnv *env, jobject peer) { return [realMenuComponent(env, peer) appkitProxy]; }

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RMenuComponentPeer_dispose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id component = realMenuComponent(env, peer);
        jobject globalPeer = [component peer];
        (*env)->SetIntField(env, peer, menuField(env), 0);
        [[AwtMenu appkitProxy] discardComponent:(int)component];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RMenuBarPeer_createNativeMenuBar(JNIEnv *env, jobject peer, jobject parent, jobject font)
{
    AwtMenuBar *volatile component = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *nativeFont = javaToNSFont(env, font);
        id nativeParent = windowFromPeer(env, parent);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        component = [AwtMenuBar alloc];
        if (!component) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtMenuBar"];
        [[component appkitProxy] initWithPeer:(int)globalPeer withMenuSet:1 withFont:[nativeFont appkitProxy] inWindow:nativeParent];
    NS_HANDLER
        [[AwtMenu appkitProxy] discardComponent:(int)component];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        component = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)component;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RMenuPeer_createNativeMenu(JNIEnv *env, jobject peer, jobject parent, jstring label, jboolean tearOff, jboolean enabled, jboolean help, jobject font)
{
    AwtMenu *volatile component = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeLabel = label ? javaToNSString(env, label) : nil;
        NSFont *nativeFont = javaToNSFont(env, font);
        id nativeParent = menuComponent(env, parent);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        component = [AwtMenu alloc];
        if (!component) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtMenu"];
        [[component appkitProxy] initWithPeer:(int)globalPeer withLabel:nativeLabel tearOff:tearOff isEnabled:enabled isHelpMenu:help withFont:[nativeFont appkitProxy] inMenuComponent:nativeParent];
    NS_HANDLER
        [[AwtMenu appkitProxy] discardComponent:(int)component];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        component = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)component;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RMenuItemPeer_createNativeMenuItem(JNIEnv *env, jobject peer,
    jobject parent, jstring label, jboolean enabled, jstring shortcut, jboolean shift, jobject font)
{
    AwtMenuItem *volatile component = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeLabel = javaToNSString(env, label);
        id nativeParent = menuComponent(env, parent);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        if ([nativeLabel isEqual:@"-"]) {
            component = [[AwtMenuItem separatorItem] retain];
            [[component appkitProxy] addSeparatorWithPeer:(int)globalPeer inMenu:nativeParent];
        } else {
            NSString *nativeShortcut = javaToNSString(env, shortcut);
            NSFont *nativeFont = javaToNSFont(env, font);
            component = [AwtMenuItem alloc];
            if (!component) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtMenuItem"];
            [[component appkitProxy] initWithPeer:(int)globalPeer withLabel:nativeLabel isEnabled:enabled withShortcut:nativeShortcut usesShiftModifier:shift withFont:[nativeFont appkitProxy] inMenu:nativeParent];
        }
    NS_HANDLER
        [[AwtMenu appkitProxy] discardComponent:(int)component];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        component = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)component;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RCheckboxMenuItemPeer_createNativeCheckboxMenuItem(JNIEnv *env, jobject peer,
    jobject parent, jstring label, jboolean enabled, jstring shortcut, jboolean shift, jboolean state, jobject font)
{
    AwtMenuItem *volatile component = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeLabel = javaToNSString(env, label);
        NSString *nativeShortcut = javaToNSString(env, shortcut);
        NSFont *nativeFont = javaToNSFont(env, font);
        id nativeParent = menuComponent(env, parent);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        component = [AwtMenuItem alloc];
        if (!component) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtMenuItem"];
        [[component appkitProxy] initWithPeer:(int)globalPeer withLabel:nativeLabel isEnabled:enabled withShortcut:nativeShortcut usesShiftModifier:shift withState:state withFont:[nativeFont appkitProxy] inMenu:nativeParent];
    NS_HANDLER
        [[AwtMenu appkitProxy] discardComponent:(int)component];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        component = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)component;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RMenuPeer_setLabel(JNIEnv *env, jobject peer, jstring label)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtSetLabel:javaToNSString(env, label)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RMenuPeer_setEnabled(JNIEnv *env, jobject peer, jboolean enabled)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtSetEnabled:enabled];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RMenuItemPeer_setEnabled(JNIEnv *env, jobject peer, jboolean enabled)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtSetEnabled:enabled];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RMenuItemPeer_nSetLabel(JNIEnv *env, jobject peer, jstring label, jstring shortcut, jboolean shift)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtSetLabel:javaToNSString(env, label) withShortcut:javaToNSString(env, shortcut) usesShiftModifier:shift];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RCheckboxMenuItemPeer_setState(JNIEnv *env, jobject peer, jboolean state)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtSetState:state];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFramePeer_nSetMenuBar(JNIEnv *env, jobject peer, jobject bar)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) awtSetMenuBar:menuComponent(env, bar)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RPopupMenuPeer_nShow(JNIEnv *env, jobject peer, jobject component, jint x, jint y)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [menuComponent(env, peer) awtShowPopupForView:viewFromPeer(env, component) atLocation:NSMakePoint(x, y)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
