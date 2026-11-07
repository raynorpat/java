/* Input construction/replay: winawt_g 10007441..1000806E,10008D49,
 * 10008ED2,1000920A,10009352,10009956. Pending events use opaque tokens
 * rather than Sun's raw MSG pointers so peer retirement can reclaim them. */
#include "awt_Component.h"
#include <stdlib.h>
extern "C" {
#include "java_awt_event_KeyEvent.h"
#include "java_awt_event_MouseEvent.h"
}

struct AwtInput {
    AwtInput *next;
    AwtComponent *component;
    long token;
    UINT message;
    WPARAM w;
    LPARAM l;
    int id, modifiers, key, character, x, y, clicks;
    BOOL popup;
    __int64 when;
};
static AwtInput *pending;
static long nextToken;
static __int64 inputTime()
{
    FILETIME time; GetSystemTimeAsFileTime(&time);
    ULARGE_INTEGER value; value.LowPart=time.dwLowDateTime; value.HighPart=time.dwHighDateTime;
    return (__int64)(value.QuadPart-116444736000000000ui64)/10000;
}
static int modifiers(int button)
{
    int value=0;
    if (GetAsyncKeyState(VK_SHIFT)&0x8000) value|=1;
    if ((GetAsyncKeyState(VK_CONTROL)&0x8000) || button==2) value|=2;
    if ((GetAsyncKeyState(VK_MBUTTON)&0x8000) || (GetAsyncKeyState(VK_MENU)&0x8000)) value|=8;
    if (GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON:VK_RBUTTON)&0x8000) value|=4;
    if (button==1) value|=16;
    if (button==2) value|=8;
    if (button==3) value|=4;
    return value;
}
static int javaKey(int key)
{
    switch (key) {
    case VK_RETURN: return 10;
    case VK_SNAPSHOT: return 154;
    case VK_INSERT: return 155;
    case VK_DELETE: return 127;
    case VK_HELP: return 156;
    default: return key;
    }
}
static int nativeKey(int key)
{
    switch (key) {
    case 10: return VK_RETURN;
    case 154: return VK_SNAPSHOT;
    case 155: return VK_INSERT;
    case 127: return VK_DELETE;
    case 156: return VK_HELP;
    default: return key;
    }
}
static int keyCharacter(int key,int mods)
{
    BYTE state[256]; WORD bytes=0; WCHAR character=0;
    if (!GetKeyboardState(state)) return 0;
    if (mods&1) state[VK_SHIFT]|=128;
    if (mods&2) state[VK_CONTROL]|=128;
    if (ToAscii(key,MapVirtualKeyA(key,0),state,&bytes,0)) {
        MultiByteToWideChar(CP_ACP,0,(char *)&bytes,1,&character,1); return character;
    }
    if (key==127 || key==10) return key;
    return key>=96 && key<=105 ? key-48:0;
}
static void insertInput(AwtInput *input)
{
    AwtComponentLock lock;
    input->next=pending; pending=input;
}
static AwtInput *takeInput(AwtComponent *component,long token)
{
    AwtComponentLock lock;
    AwtInput **link=&pending;
    while (*link && ((*link)->component!=component || (*link)->token!=token)) link=&(*link)->next;
    AwtInput *input=*link;
    if (input) *link=input->next;
    return input;
}
static void queueInput(AwtComponent *component,const AwtInput &value)
{
    AwtInput *input=new AwtInput(value);
    input->component=component; input->token=InterlockedIncrement(&nextToken);
    insertInput(input);
    if (!PostMessage(component->GetHWnd(),WM_APP+9,input->token,0)) delete takeInput(component,input->token);
}
void AwtComponent::ClearInput()
{
    AwtComponentLock lock;
    AwtInput **link=&pending;
    while (*link) {
        AwtInput *input=*link;
        if (input->component==this) { *link=input->next; delete input; }
        else link=&input->next;
    }
}
BOOL AwtComponent::InputMessage(UINT message,WPARAM w,LPARAM l)
{
    if (message==WM_APP+9 || message==WM_APP+10) {
        AwtInput *input=takeInput(this,(long)w);
        if (!input) return TRUE;
        if (!started || IsDisposed()) { delete input; return TRUE; }
        if (message==WM_APP+10) {
            UINT nativeMessage=input->message; WPARAM nativeW=input->w; LPARAM nativeL=input->l;
            delete input;
            WindowProc(nativeMessage,nativeW,nativeL); return TRUE;
        }
        Hjava_awt_AWTEvent *event;
        if (input->id>=500) {
            int x=input->x,y=input->y;
            if (!(GetWindowLong(hwnd,GWL_STYLE)&WS_CHILD)) {
                RECT outer; POINT origin={0,0}; GetWindowRect(hwnd,&outer); ClientToScreen(hwnd,&origin);
                x+=origin.x-outer.left; y+=origin.y-outer.top;
            }
            event=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/MouseEvent",NULL,
                "(Ljava/awt/Component;IJIIIIZ)",GetTarget(),input->id,input->when,input->modifiers,x,y,input->clicks,input->popup);
        } else event=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/KeyEvent",NULL,
            "(Ljava/awt/Component;IJIIC)",GetTarget(),input->id,input->when,input->modifiers,input->key,input->character);
        if (event && !exceptionOccurred(EE()) && !IsDisposed()) {
            long eventToken=0;
            if (input->id==503 || input->id==506) {
                /* EventQueue replaces queued motion events without returning
                 * them to the peer. Store button flags inline, not an owned MSG.
                 * Replay uses the final Java event's client position. */
                unhand(event)->data=(long)(0x80000000u|(input->w&0x7fffffffu));
                delete input;
            } else if (input->message) {
                eventToken=input->token; unhand(event)->data=eventToken; insertInput(input);
            } else delete input;
            execute_java_dynamic_method(EE(),(HObject *)peer,"postEvent","(Ljava/awt/AWTEvent;)V",event);
            if (exceptionOccurred(EE())) delete takeInput(this,eventToken);
        } else delete input;
        if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
        return TRUE;
    }
    if (!started || IsDisposed()) return FALSE;
    AwtInput input={0}; input.message=message; input.w=w; input.l=l; input.when=inputTime();
    input.x=(short)LOWORD(l); input.y=(short)HIWORD(l);
    int button=0;
    switch (message) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: button=1; input.id=501; break;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: button=2; input.id=501; break;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: button=3; input.id=501; break;
    case WM_LBUTTONUP: button=1; input.id=502; break;
    case WM_MBUTTONUP: button=2; input.id=502; break;
    case WM_RBUTTONUP: button=3; input.id=502; break;
    case WM_MOUSEMOVE: input.id=(w&(MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) ? 506:503; break;
    case WM_MOUSELEAVE: {
        DWORD position=GetMessagePos(); POINT point={(short)LOWORD(position),(short)HIWORD(position)};
        ScreenToClient(hwnd,&point); input.x=point.x; input.y=point.y;
        mouseInside=FALSE; input.id=505; input.message=0; break;
    }
    case WM_KEYDOWN: case WM_SYSKEYDOWN: if (w==229) return FALSE; input.id=401; break;
    case WM_KEYUP: case WM_SYSKEYUP: if (w==229) return FALSE; input.id=402; break;
    case WM_CHAR: case WM_SYSCHAR: input.id=400; break;
    default: return FALSE;
    }
    input.modifiers=modifiers(button);
    if (input.id>=500) {
        if (input.id==501) {
            DWORD now=GetMessageTime();
            if (lastClick && now-lastClick<=GetDoubleClickTime() &&
                abs(input.x-clickPoint.x)<=GetSystemMetrics(SM_CXDOUBLECLK) &&
                abs(input.y-clickPoint.y)<=GetSystemMetrics(SM_CYDOUBLECLK)) ++clickCount;
            else clickCount=1;
            lastClick=now; clickPoint.x=input.x; clickPoint.y=input.y; mouseDragged=FALSE;
        }
        if (input.id==501 || input.id==502) input.clicks=clickCount;
        if (input.id==502) input.popup=button==3;
        if (input.id==503 || input.id==506) {
            if (!mouseInside) {
                mouseInside=TRUE;
                TRACKMOUSEEVENT tracking={sizeof(tracking),TME_LEAVE,hwnd,0}; TrackMouseEvent(&tracking);
                AwtInput enter=input; enter.id=504; enter.message=0; enter.modifiers=0; queueInput(this,enter);
            }
            if (input.id==506) mouseDragged=TRUE;
        }
        queueInput(this,input);
        if (input.id==502 && !mouseDragged) { input.id=500; input.message=0; input.popup=FALSE; queueInput(this,input); }
        if (input.id==502) mouseDragged=FALSE;
    } else {
        if (input.id==400) {
            input.character=(int)w;
            if (!IsWindowUnicode(hwnd)) {
                char bytes[2]={(char)(w>>8),(char)w}; WCHAR character=0;
                MultiByteToWideChar(CP_ACP,0,bytes+(w>255 ? 0:1),w>255 ? 2:1,&character,1);
                input.character=character;
            }
        } else { input.key=javaKey((int)w); input.character=keyCharacter(input.key,input.modifiers); }
        queueInput(this,input);
    }
    return TRUE;
}
void AwtComponent::ReplayInput(Hjava_awt_AWTEvent *event)
{
    Classjava_awt_AWTEvent *base=unhand(event);
    long token=base->data; base->data=0;
    int id=base->id;
    BOOL inlineMotion=(id==503 || id==506) && ((unsigned long)token&0x80000000u);
    AwtInput *input=token && !inlineMotion ? takeInput(this,token):NULL;
    if (base->consumed) { suppressTyped=id==401; delete input; return; }
    if (id==400 && suppressTyped) { suppressTyped=FALSE; delete input; return; }
    if (id==402) suppressTyped=FALSE;
    if (token && !input && !inlineMotion) return;
    if (!input) {
        if (id!=400 && id!=401 && id!=402 && id!=501 && id!=502 && id!=503 && id!=506) return;
        input=new AwtInput(); input->component=this; input->token=InterlockedIncrement(&nextToken);
        if (id>=500) {
            Classjava_awt_event_MouseEvent *mouse=unhand((Hjava_awt_event_MouseEvent *)event);
            int button=(mouse->modifiers&4) ? 1:(mouse->modifiers&8) ? 2:0;
            input->message=id==501 ? WM_LBUTTONDOWN+button*3:id==502 ? WM_LBUTTONUP+button*3:WM_MOUSEMOVE;
            int x=mouse->x,y=mouse->y;
            if (inlineMotion) {
                input->w=(unsigned long)token&0x7fffffffu;
                if (!(GetWindowLong(hwnd,GWL_STYLE)&WS_CHILD)) {
                    RECT outer; POINT origin={0,0}; GetWindowRect(hwnd,&outer); ClientToScreen(hwnd,&origin);
                    x-=origin.x-outer.left; y-=origin.y-outer.top;
                }
            }
            input->l=MAKELPARAM(x,y);
        } else {
            Classjava_awt_event_KeyEvent *key=unhand((Hjava_awt_event_KeyEvent *)event);
            input->message=id==400 ? WM_CHAR:id==401 ? WM_KEYDOWN:WM_KEYUP;
            input->w=nativeKey(key->keyCode);
        }
    }
    if (id==400) {
        Classjava_awt_event_KeyEvent *key=unhand((Hjava_awt_event_KeyEvent *)event);
        input->w=key->keyChar;
        if (!IsWindowUnicode(hwnd)) {
            WCHAR character=(WCHAR)key->keyChar; char bytes[2]={0};
            int length=WideCharToMultiByte(CP_ACP,0,&character,1,bytes,2,NULL,NULL);
            input->w=length==2 ? ((unsigned char)bytes[0]<<8)|(unsigned char)bytes[1]:(unsigned char)bytes[0];
        }
    }
    insertInput(input);
    if (!PostMessage(hwnd,WM_APP+10,input->token,0)) delete takeInput(this,input->token);
}
