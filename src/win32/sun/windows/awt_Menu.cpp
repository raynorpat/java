/* Menus recovered from winawt_g 1001BF30..1001E9BB. Java layouts are
 * generated from the unchanged 1.1.5 classes, not the older classsrc peers. */
#include "awt_Menu.h"
#include "awt_Font.h"
extern "C" {
#include "sun_awt_windows_WMenuItemPeer.h"
#include "java_awt_MenuComponent.h"
#include "java_awt_MenuItem.h"
#include "java_awt_Event.h"
}

enum MenuKind { MENU_ITEM, MENU, MENU_BAR, POPUP };
static BOOL attachingMenuBar=FALSE;
/* CHECK_BITMAP's 26x26 monochrome raster, recovered from the Sun 1.1.5
 * resource. Row padding and the vertical StretchBlt placement match1001DA3C. */
static void drawMenuCheck(HDC dc,RECT rect,int width)
{
    static const DWORD bits[26]={
        0xc0ffffff,0xc0ffffff,0xc0ffffff,0xc0ffffff,0xc0ffffff,0xc0ffffff,
        0xc0ff3fff,0xc0ff3fff,0xc0ff0ffc,0xc0ff0ffc,0xc0ff03f0,0xc0ff03f0,
        0xc0ffc0f0,0xc0ffc0f0,0xc03ff0f3,0xc03ff0f3,0xc03ffcff,0xc03ffcff,
        0xc03fffff,0xc03fffff,0xc0ffffff,0xc0ffffff,0xc0ffffff,0xc0ffffff,
        0xc0ffffff,0xc0ffffff
    };
    struct { BITMAPINFOHEADER header; RGBQUAD colors[2]; } info={
        {sizeof(BITMAPINFOHEADER),26,26,1,1,BI_RGB,sizeof(bits)},{{0,0,0,0},{255,255,255,0}}};
    HBITMAP bitmap=CreateBitmap(26,26,1,1,NULL);
    if (!bitmap) return;
    SetDIBits(dc,bitmap,0,26,bits,(BITMAPINFO *)&info,DIB_RGB_COLORS);
    HDC source=CreateCompatibleDC(dc); HGDIOBJ old=SelectObject(source,bitmap);
    int extra=max(0,rect.bottom-rect.top-width);
    StretchBlt(dc,rect.left,rect.top+extra/2,width,rect.bottom-rect.top-extra,source,0,0,26,26,SRCCOPY);
    SelectObject(source,old); DeleteDC(source); DeleteObject(bitmap);
}
class AwtMenu {
public:
    Hsun_awt_windows_WMenuItemPeer *peer;
    AwtMenu *parent,*next;
    HMENU handle;
    HWND owner;
    UINT id,token;
    MenuKind kind;
    BOOL disposed;
    LONG references;
    UINT stagedWidth,stagedHeight;
    static AwtMenu *menus;
    static UINT nextID,nextToken;
    AwtMenu(Hsun_awt_windows_WMenuItemPeer *p,MenuKind k) : peer(p),parent(NULL),next(NULL),
        handle(NULL),owner(NULL),id(0),token(++nextToken),kind(k),disposed(FALSE),references(1),stagedWidth(0),stagedHeight(0) {}
    void Retain() { ++references; }
    void Release() { if (!--references) delete this; }
    Hjava_awt_MenuItem *Target() { return (Hjava_awt_MenuItem *)unhand(peer)->target; }
    BOOL TopLevel() { return parent && parent->kind==MENU_BAR; }
    static AwtMenu *Get(HObject *p) {
        for (AwtMenu *m=menus;m;m=m->next) if ((HObject *)m->peer==p) return m;
        return NULL;
    }
    static AwtMenu *ByID(UINT id,HWND owner) {
        for (AwtMenu *m=menus;m;m=m->next) if (m->id==id && m->owner==owner) return m;
        return NULL;
    }
    static AwtMenu *ByToken(UINT token,HWND owner) {
        for (AwtMenu *m=menus;m;m=m->next) if (m->token==token && m->owner==owner) return m;
        return NULL;
    }
    /* Repainting can call Java font code. The Java caller of a mutation may
     * still own its target's monitor while waiting for Invoke to return. */
    void Redraw() { if (IsWindow(owner)) PostMessage(owner,WM_APP+31,0,0); }
    void Dispose() {
        if (disposed) return;
        disposed=TRUE; unhand(peer)->pData=0;
        AwtMenu **link=&menus; while (*link && *link!=this) link=&(*link)->next;
        if (*link) *link=next;
        /* Java normally retires children first. Keep finalization and native
         * window destruction safe when it does not. */
        for (;;) {
            AwtMenu *child=menus; while (child && child->parent!=this) child=child->next;
            if (!child) break;
            child->Dispose();
        }
        if (handle) {
            if (kind==MENU_BAR && GetMenu(owner)==handle) SetMenu(owner,NULL);
            DestroyMenu(handle); handle=NULL;
        }
        Release();
    }
    Hjava_awt_Font *Font() {
        Hjava_awt_Font *font=unhand((Hjava_awt_MenuComponent *)Target())->font;
        if (!font) font=(Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)peer,
            "getDefaultFont","()Ljava/awt/Font;");
        return font;
    }
    SIZE Text(HDC dc,Hjava_awt_Font *font,Hjava_lang_String *text,int x,int y,BOOL draw) {
        SIZE empty={0,0};
        return text ? awtFontText(dc,font,text,x,y,draw):empty;
    }
    void Measure(HDC dc,MEASUREITEMSTRUCT *item) {
        if (attachingMenuBar) { item->itemWidth=stagedWidth; item->itemHeight=stagedHeight; return; }
        Hjava_awt_Font *volatile font=Font(); if (disposed || exceptionOccurred(EE())) return;
        SIZE size=Text(dc,font,unhand(Target())->label,0,0,FALSE); if (disposed) return;
        HObject *toolkit=(HObject *)execute_java_static_method(EE(),
            FindClass(EE(),"java/awt/Toolkit",TRUE),"getDefaultToolkit","()Ljava/awt/Toolkit;");
        if (disposed || !toolkit || exceptionOccurred(EE())) return;
        HObject *metrics=(HObject *)execute_java_dynamic_method(EE(),toolkit,"getFontMetrics",
            "(Ljava/awt/Font;)Ljava/awt/FontMetrics;",font);
        if (disposed || !metrics || exceptionOccurred(EE())) return;
        int height=(int)execute_java_dynamic_method(EE(),metrics,"getHeight","()I");
        if (disposed || exceptionOccurred(EE())) return;
        item->itemHeight=height+height/3; item->itemWidth=size.cx;
        if (!TopLevel()) {
            int check=GetSystemMetrics(SM_CXMENUCHECK); item->itemWidth+=check;
            Hjava_lang_String *shortcut=unhand(peer)->shortcutLabel;
            if (shortcut) {
                SIZE extra=Text(dc,font,shortcut,0,0,FALSE); if (disposed) return;
                item->itemWidth+=extra.cx+check;
            }
        }
    }
    void Draw(DRAWITEMSTRUCT *item) {
        if (attachingMenuBar) return; /* The posted redraw renders after Invoke returns. */
        HDC dc=item->hDC; int saved=SaveDC(dc); RECT rect=item->rcItem;
        Hjava_awt_Font *volatile font=Font();
        if (!disposed && !exceptionOccurred(EE())) {
            Hjava_lang_String *volatile label=unhand(Target())->label;
            SIZE size=Text(dc,font,label,0,0,FALSE);
            if (!disposed) {
                BOOL selected=(item->itemState&ODS_SELECTED)!=0,enabled=unhand(Target())->enabled;
                COLORREF background=GetSysColor(selected ? COLOR_HIGHLIGHT:COLOR_MENU);
                HBRUSH brush=CreateSolidBrush(background); FillRect(dc,&rect,brush); DeleteObject(brush);
                SetBkColor(dc,background); SetBkMode(dc,TRANSPARENT);
                SetTextColor(dc,GetSysColor(!enabled ? COLOR_GRAYTEXT:selected ? COLOR_HIGHLIGHTTEXT:COLOR_MENUTEXT));
                int check=GetSystemMetrics(SM_CXMENUCHECK);
                if (unhand(peer)->isCheckbox && (item->itemState&ODS_CHECKED)) {
                    drawMenuCheck(dc,rect,check);
                }
                int x=TopLevel() ? (rect.left+rect.right-size.cx)/2:rect.left+check;
                int y=(rect.top+rect.bottom-size.cy)/2;
                if (!enabled && !selected) {
                    SetTextColor(dc,GetSysColor(COLOR_3DHILIGHT)); Text(dc,font,label,x+1,y+1,TRUE);
                    SetTextColor(dc,GetSysColor(COLOR_3DSHADOW));
                }
                if (!disposed) Text(dc,font,label,x,y,TRUE);
                if (!disposed && !TopLevel()) {
                    Hjava_lang_String *shortcut=unhand(peer)->shortcutLabel;
                    if (shortcut) {
                        SIZE extra=Text(dc,font,shortcut,0,0,FALSE);
                        if (!disposed) Text(dc,font,shortcut,rect.right-check-extra.cx,y,TRUE);
                    }
                }
            }
        }
        RestoreDC(dc,saved);
    }
};
AwtMenu *AwtMenu::menus=NULL;
UINT AwtMenu::nextID=0,AwtMenu::nextToken=0;
class MenuLease {
    AwtMenu *menu;
    HObject *volatile peer;
public:
    MenuLease(AwtMenu *m) : menu(m),peer((HObject *)m->peer) { m->Retain(); }
    ~MenuLease() { if (peer) menu->Release(); }
};
static void clearCallbackError()
{ if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); } }

BOOL awtMenuMessage(AwtComponent *component,UINT message,WPARAM w,LPARAM l,LRESULT *result)
{
    HWND window=component->GetHWnd(); AwtMenu *menu=NULL;
    if (message==WM_DESTROY) {
        for (;;) {
            menu=AwtMenu::menus; while (menu && menu->owner!=window) menu=menu->next;
            if (!menu) break; menu->Dispose();
        }
        return FALSE;
    }
    if (message==WM_COMMAND && !l && !HIWORD(w)) {
        menu=AwtMenu::ByID(LOWORD(w),window);
        if (!menu || menu->kind!=MENU_ITEM || !menu->parent) return FALSE;
        UINT state=GetMenuState(menu->parent->handle,menu->id,MF_BYCOMMAND);
        if (state==(UINT)-1 || (state&(MF_DISABLED|MF_GRAYED))) return FALSE;
        int modifiers=(GetKeyState(VK_SHIFT)<0 ? 1:0)|(GetKeyState(VK_CONTROL)<0 ? 2:0)|
            (GetKeyState(VK_RBUTTON)<0 ? 4:0)|(GetKeyState(VK_MBUTTON)<0 ? 8:0);
        PostMessage(window,WM_APP+30,menu->token,modifiers|((state&MF_CHECKED) ? 0:0x100));
        *result=0; return TRUE;
    }
    if (message==WM_APP+30) {
        menu=AwtMenu::ByToken((UINT)w,window);
        if (menu) {
            MenuLease lease(menu);
            if (unhand(menu->peer)->isCheckbox)
                execute_java_dynamic_method(EE(),(HObject *)menu->peer,"handleAction","(ZI)V",(l&0x100)!=0,(int)(l&0xff));
            else execute_java_dynamic_method(EE(),(HObject *)menu->peer,"handleAction","(I)V",(int)(l&0xff));
            clearCallbackError();
        }
        *result=0; return TRUE;
    }
    if (message==WM_APP+31) {
        DrawMenuBar(window);
        if (!component->IsDisposed()) component->UpdateInsets(NULL);
        *result=0; return TRUE;
    }
    if (message==WM_MEASUREITEM && l && ((MEASUREITEMSTRUCT *)l)->CtlType==ODT_MENU) {
        menu=AwtMenu::ByID(((MEASUREITEMSTRUCT *)l)->itemID,window);
        if (!menu) return FALSE;
        MenuLease lease(menu); HDC dc=GetDC(window); int saved=SaveDC(dc);
        menu->Measure(dc,(MEASUREITEMSTRUCT *)l); RestoreDC(dc,saved); ReleaseDC(window,dc);
        clearCallbackError(); *result=TRUE; return TRUE;
    }
    if (message==WM_DRAWITEM && l && ((DRAWITEMSTRUCT *)l)->CtlType==ODT_MENU) {
        menu=AwtMenu::ByID(((DRAWITEMSTRUCT *)l)->itemID,window);
        if (!menu) return FALSE;
        MenuLease lease(menu); menu->Draw((DRAWITEMSTRUCT *)l);
        clearCallbackError(); *result=TRUE; return TRUE;
    }
    return FALSE;
}
void awtMenuCleanup() { while (AwtMenu::menus) AwtMenu::menus->Dispose(); }

enum MenuOperation { CREATE_ITEM,CREATE_MENU,CREATE_BAR,CREATE_POPUP,LABEL,ENABLE,CHECK,DISPOSE,SEPARATOR,REMOVE,REDRAW,SHOW };
struct MenuRequest {
    HObject *peer,*parent;
    MenuOperation operation;
    long value;
    Hjava_awt_Event *event;
    BOOL failed;
};
static void menuRequest(void *argument)
{
    MenuRequest *r=(MenuRequest *)argument;
    AwtMenu *menu=AwtMenu::Get(r->peer);
    if (r->operation<=CREATE_POPUP) {
        MenuKind kind=r->operation==CREATE_ITEM ? MENU_ITEM:r->operation==CREATE_BAR ? MENU_BAR:r->operation==CREATE_POPUP ? POPUP:MENU;
        AwtMenu *parent=NULL; AwtComponent *owner=NULL;
        if (kind==MENU_ITEM || kind==MENU) parent=AwtMenu::Get(r->parent);
        else owner=AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->parent);
        if ((kind==MENU_ITEM || kind==MENU) ? !parent:(kind==POPUP && !owner)) { r->failed=TRUE; return; }
        menu=new AwtMenu((Hsun_awt_windows_WMenuItemPeer *)r->peer,kind);
        menu->parent=parent; menu->owner=parent ? parent->owner:owner ? owner->GetHWnd():NULL;
        if (kind!=MENU_ITEM) menu->handle=kind==POPUP ? CreatePopupMenu():CreateMenu();
        if (kind==MENU_ITEM) {
            if (AwtMenu::nextID>=0xffff) { menu->Release(); r->failed=TRUE; return; }
            menu->id=++AwtMenu::nextID;
        } else menu->id=(UINT)menu->handle;
        if (kind!=MENU_ITEM && !menu->handle) { menu->Release(); r->failed=TRUE; return; }
        menu->next=AwtMenu::menus; AwtMenu::menus=menu;
        unhand(menu->peer)->pData=(long)menu;
        if (parent) {
            Classjava_awt_MenuItem *target=unhand(menu->Target());
            Hjava_lang_String *label=target->label;
            unicode first=0; if (label && javaStringLength(label)==1) javaString2unicode(label,&first,1);
            BOOL separator=kind==MENU_ITEM && label && javaStringLength(label)==1 && first=='-';
            UINT flags=separator ? MF_SEPARATOR:MF_OWNERDRAW|(target->enabled ? 0:MF_GRAYED)|(kind==MENU ? MF_POPUP:0);
            if (!AppendMenuW(parent->handle,flags,separator ? 0:menu->id,(LPCWSTR)menu)) {
                menu->Dispose(); r->failed=TRUE; return;
            }
            menu->Redraw();
        }
        return;
    }
    if (!menu) { if (r->operation!=DISPOSE) r->failed=TRUE; return; }
    MenuLease lease(menu);
    switch (r->operation) {
    case DISPOSE: menu->Dispose(); break;
    case LABEL: menu->Redraw(); break; /* Owner drawing reads the Java label. */
    case ENABLE:
        if (menu->parent) EnableMenuItem(menu->parent->handle,menu->id,MF_BYCOMMAND|(r->value ? MF_ENABLED:MF_GRAYED));
        menu->Redraw(); break;
    case CHECK:
        if (menu->parent) CheckMenuItem(menu->parent->handle,menu->id,MF_BYCOMMAND|(r->value ? MF_CHECKED:MF_UNCHECKED));
        menu->Redraw(); break;
    case SEPARATOR: AppendMenuW(menu->handle,MF_SEPARATOR,0,NULL); break;
    case REMOVE: RemoveMenu(menu->handle,r->value,MF_BYPOSITION); menu->Redraw(); break;
    case REDRAW: menu->Redraw(); break;
    case SHOW: {
        HObject *target=(HObject *)unhand(r->event)->target;
        Hsun_awt_windows_WComponentPeer *peer=target ? (Hsun_awt_windows_WComponentPeer *)unhand((Hjava_awt_Component *)target)->peer:NULL;
        AwtComponent *component=AwtComponent::Get(peer);
        if (!component) { r->failed=TRUE; break; }
        POINT point={unhand(r->event)->x,unhand(r->event)->y};
        MapWindowPoints(component->GetHWnd(),NULL,&point,1);
        TrackPopupMenu(menu->handle,TPM_RIGHTBUTTON,point.x,point.y,0,menu->owner,NULL);
        break;
    }
    }
}
static stack_item *request(stack_item *sp,struct execenv *ee,MenuOperation operation)
{
    MenuRequest r={(HObject *)sp[0].p,NULL,operation,0,NULL,FALSE};
    if (operation<=CREATE_POPUP) r.parent=(HObject *)sp[1].p;
    if (operation==ENABLE || operation==CHECK || operation==REMOVE) r.value=sp[1].i;
    if (operation==SHOW) r.event=(Hjava_awt_Event *)sp[1].p;
    if (!r.peer || (operation==SHOW && !r.event) || (operation==LABEL && !sp[1].p))
        SignalError(ee,"java/lang/NullPointerException","invalid menu peer, label or event");
    else {
        AwtToolkit::GetInstance().Invoke(menuRequest,&r);
        if (r.failed) SignalError(ee,"java/lang/NullPointerException","invalid menu peer or parent");
    }
    return sp;
}
#define MENU_STUB(name,operation) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_##name##_stub(stack_item *sp,struct execenv *ee) { return request(sp,ee,operation); }
MENU_STUB(WMenuItemPeer_create,CREATE_ITEM)
MENU_STUB(WMenuPeer_createMenu,CREATE_MENU)
MENU_STUB(WMenuPeer_createSubMenu,CREATE_MENU)
MENU_STUB(WMenuBarPeer_create,CREATE_BAR)
MENU_STUB(WPopupMenuPeer_createMenu,CREATE_POPUP)
MENU_STUB(WMenuItemPeer__0005fsetLabel,LABEL)
MENU_STUB(WMenuItemPeer_enable,ENABLE)
MENU_STUB(WCheckboxMenuItemPeer_setState,CHECK)
MENU_STUB(WMenuItemPeer__0005fdispose,DISPOSE)
MENU_STUB(WMenuPeer_addSeparator,SEPARATOR)
MENU_STUB(WMenuPeer_delItem,REMOVE)
MENU_STUB(WMenuBarPeer_addMenu,REDRAW)
MENU_STUB(WMenuBarPeer_delMenu,REMOVE)
MENU_STUB(WPopupMenuPeer__0005fshow,SHOW)

struct MenuLayout { HObject *peer; UINT width,height; };
static void stageMenuLayout(void *argument)
{
    MenuLayout *layout=(MenuLayout *)argument; AwtMenu *menu=AwtMenu::Get(layout->peer);
    if (menu) { menu->stagedWidth=layout->width; menu->stagedHeight=layout->height; }
}
/* SetMenu itself may synchronously measure/draw on the native owner. Resolve
 * Java metrics on the initiating Java thread, where its monitors are reentrant.
 * Only scalar layout data crosses Invoke; no native pointer survives Java. */
static void prepareMenuLayout(HObject *target,BOOL bar,BOOL topLevel)
{
    if (!target || exceptionOccurred(EE())) return;
    Hsun_awt_windows_WMenuItemPeer *volatile peer=(Hsun_awt_windows_WMenuItemPeer *)
        unhand((Hjava_awt_MenuComponent *)target)->peer;
    if (!peer) return;
    if (!bar) {
        Hjava_awt_Font *volatile font=unhand((Hjava_awt_MenuComponent *)target)->font;
        if (!font) font=(Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)peer,
            "getDefaultFont","()Ljava/awt/Font;");
        HObject *toolkit=(HObject *)execute_java_static_method(EE(),
            FindClass(EE(),"java/awt/Toolkit",TRUE),"getDefaultToolkit","()Ljava/awt/Toolkit;");
        if (!toolkit || exceptionOccurred(EE())) return;
        HObject *volatile metrics=(HObject *)execute_java_dynamic_method(EE(),toolkit,"getFontMetrics",
            "(Ljava/awt/Font;)Ljava/awt/FontMetrics;",font);
        if (!metrics || exceptionOccurred(EE())) return;
        Hjava_lang_String *label=unhand((Hjava_awt_MenuItem *)target)->label;
        int width=label ? (int)execute_java_dynamic_method(EE(),metrics,"stringWidth","(Ljava/lang/String;)I",label):0;
        int height=(int)execute_java_dynamic_method(EE(),metrics,"getHeight","()I");
        if (!topLevel) {
            int check=GetSystemMetrics(SM_CXMENUCHECK); width+=check;
            Hjava_lang_String *shortcut=unhand(peer)->shortcutLabel;
            if (shortcut) width+=check+(int)execute_java_dynamic_method(EE(),metrics,"stringWidth","(Ljava/lang/String;)I",shortcut);
        }
        if (exceptionOccurred(EE())) return;
        MenuLayout layout={(HObject *)peer,(UINT)width,(UINT)(height+height/3)};
        AwtToolkit::GetInstance().Invoke(stageMenuLayout,&layout);
    }
    if (!bar && !is_instance_of(target,FindClass(EE(),"java/awt/Menu",TRUE),EE())) return;
    int count=(int)execute_java_dynamic_method(EE(),target,bar ? "countMenus":"countItems","()I");
    for (int i=0;i<count && !exceptionOccurred(EE());i++) {
        HObject *child=(HObject *)execute_java_dynamic_method(EE(),target,bar ? "getMenu":"getItem",
            bar ? "(I)Ljava/awt/Menu;":"(I)Ljava/awt/MenuItem;",i);
        prepareMenuLayout(child,FALSE,bar);
    }
}
struct MenuBarRequest { Hsun_awt_windows_WComponentPeer *frame; HObject *menu; };
static void setMenuBar(void *argument)
{
    MenuBarRequest *r=(MenuBarRequest *)argument;
    AwtComponent *frame=AwtComponent::Get(r->frame); AwtMenu *menu=AwtMenu::Get(r->menu);
    if (!frame || (r->menu && !menu)) return;
    AwtComponentLease lease(frame);
    if (menu) for (AwtMenu *item=AwtMenu::menus;item;item=item->next) {
        AwtMenu *root=item; while (root->parent) root=root->parent;
        if (root==menu) item->owner=frame->GetHWnd();
    }
    attachingMenuBar=TRUE;
    SetMenu(frame->GetHWnd(),menu ? menu->handle:NULL);
    attachingMenuBar=FALSE;
    if (!frame->IsDisposed()) {
        frame->UpdateInsets(NULL); PostMessage(frame->GetHWnd(),WM_APP+31,0,0);
    }
}
void awtSetMenuBar(Hsun_awt_windows_WComponentPeer *frame,HObject *menu)
{
    if (menu) prepareMenuLayout((HObject *)unhand((Hsun_awt_windows_WMenuItemPeer *)menu)->target,TRUE,FALSE);
    if (!exceptionOccurred(EE())) { MenuBarRequest r={frame,menu}; AwtToolkit::GetInstance().Invoke(setMenuBar,&r); }
}
