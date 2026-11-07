/* Text peers recovered from winawt_g1002098A..100221F7 (NT Unicode path). */
#include "awt_Component.h"
extern "C" {
#include "java_awt_TextArea.h"
}
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

class AwtTextComponent : public AwtComponent {
public:
    BOOL multiline;
    AwtTextComponent(Hsun_awt_windows_WComponentPeer *p,BOOL multi) : AwtComponent(p), multiline(multi) {}
    virtual void Command(WORD code) {
        if (started && code==EN_CHANGE) PostMessage(hwnd,WM_APP+4,0,0);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (!multiline && message==WM_CHAR && (w==13 || w==9)) {
            if (w==13 && started) PostMessage(hwnd,WM_APP+5,0,0);
            return 0;
        }
        if (message==WM_APP+4 || message==WM_APP+5) {
            if (started) {
                execute_java_dynamic_method(EE(),(HObject *)peer,message==WM_APP+4 ? "valueChanged" : "handleAction","()V");
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
    WCHAR *Text() {
        int length=GetWindowTextLengthW(hwnd);
        WCHAR *text=(WCHAR *)malloc((length+1)*sizeof(WCHAR));
        if (!text) return NULL;
        GetWindowTextW(hwnd,text,length+1);
        int write=0;
        for (int i=0;text[i];i++) {
            if (text[i]==13 && text[i+1]==10) i++;
            text[write++]=text[i];
        }
        text[write]=0; return text;
    }
};
/* 1002134F inserts a CR before every LF, including one preceded by CR. */
static WCHAR *nativeText(Hjava_lang_String *value)
{
    WCHAR *text=awtUnicode(value);
    if (!text) return NULL;
    int length=(int)wcslen(text), lines=0;
    for (int i=0;i<length;i++) if (text[i]==10) lines++;
    WCHAR *native=(WCHAR *)malloc((length+lines+1)*sizeof(WCHAR));
    if (native) {
        int write=0;
        for (int i=0;i<length;i++) { if (text[i]==10) native[write++]=13; native[write++]=text[i]; }
        native[write]=0;
    }
    free(text); return native;
}
enum TextOperation { TEXT_FIELD, TEXT_AREA, TEXT_GET, TEXT_SET, TEXT_START, TEXT_END, TEXT_SELECT, TEXT_EDITABLE, TEXT_ECHO, TEXT_REPLACE };
struct TextRequest {
    Hsun_awt_windows_WComponentPeer *peer,*parent;
    TextOperation operation;
    WCHAR *text;
    long first,last,result;
    BOOL failed;
};
static int newlinesBefore(const WCHAR *text,int position)
{
    int count=0;
    for (int i=0;text[i] && i<position;i++) if (text[i]==10) count++;
    return count;
}
static void textRequest(void *argument)
{
    TextRequest *r=(TextRequest *)argument;
    if (r->operation==TEXT_FIELD || r->operation==TEXT_AREA) {
        AwtComponent *parent=AwtComponent::Get(r->parent);
        if (!parent) { r->failed=TRUE; return; }
        BOOL multi=r->operation==TEXT_AREA;
        DWORD style=0x44000180;
        if (multi) {
            static const DWORD scrollStyles[]={0x3000C0,0x200040,0x1000C0,0x40};
            int policy=unhand((Hjava_awt_TextArea *)unhand(r->peer)->target)->scrollbarVisibility;
            style=0x44001104|scrollStyles[policy>=0 && policy<4 ? policy : 0];
        }
        AwtTextComponent *text=new AwtTextComponent(r->peer,multi);
        if (!text->CreateControl(L"EDIT",L"",style,parent->GetHWnd(),WS_EX_CLIENTEDGE)) { delete text; r->failed=TRUE; }
        return;
    }
    AwtTextComponent *text=(AwtTextComponent *)AwtComponent::Get(r->peer);
    if (!text) { r->failed=TRUE; return; }
    AwtComponentLease lease(text);
    HWND window=text->GetHWnd();
    switch (r->operation) {
    case TEXT_GET: r->text=text->Text(); break;
    case TEXT_SET: SetWindowTextW(window,r->text ? r->text : L""); break;
    case TEXT_START: { DWORD start; SendMessageW(window,EM_GETSEL,(WPARAM)&start,0); r->result=start; break; }
    case TEXT_END: { DWORD end; SendMessageW(window,EM_GETSEL,0,(LPARAM)&end); r->result=end; break; }
    case TEXT_EDITABLE: SendMessageW(window,EM_SETREADONLY,!r->first,0); break;
    case TEXT_ECHO: SendMessageW(window,EM_SETPASSWORDCHAR,r->first,0); break;
    case TEXT_SELECT:
    case TEXT_REPLACE: {
        long first=r->first,last=r->last;
        WCHAR *current=text->Text();
        if (current) {
            if (r->operation==TEXT_REPLACE) {
                int length=(int)wcslen(current);
                if (first>length) first=length;
                if (last>length) last=length;
                first+=newlinesBefore(current,first); last+=newlinesBefore(current,last);
            } else if (last>first) {
                first+=newlinesBefore(current,r->first); last+=newlinesBefore(current,r->last);
            }
            free(current);
        }
        SendMessageW(window,EM_SETSEL,first,last);
        if (r->operation==TEXT_REPLACE) SendMessageW(window,EM_REPLACESEL,FALSE,(LPARAM)r->text);
        else SendMessageW(window,EM_SCROLLCARET,0,0);
        break;
    }
    }
}
static void invokeText(TextRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(textRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid text peer");
}
#define STUB(peer,name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_##peer##_##name##_stub(stack_item *sp,struct execenv *ee)
#define CREATE(peer,operation) STUB(peer,create) { \
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,operation}; \
    invokeText(&r,ee); return sp; }
CREATE(WTextFieldPeer,TEXT_FIELD)
CREATE(WTextAreaPeer,TEXT_AREA)
STUB(WTextComponentPeer,getText) {
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_GET};
    invokeText(&r,ee); sp[0].p=NULL;
    if (r.text) {
        int length=(int)wcslen(r.text);
        HArrayOfChar *chars=(HArrayOfChar *)ArrayAlloc(T_CHAR,length);
        if (chars) {
            memcpy(unhand(chars)->body,r.text,length*sizeof(WCHAR));
            sp[0].p=execute_java_constructor(ee,"java/lang/String",NULL,"([CII)",chars,0,length);
        }
        free(r.text);
    }
    return sp+1;
}
STUB(WTextComponentPeer,setText) {
    WCHAR *value=nativeText((Hjava_lang_String *)sp[1].p);
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_SET,value};
    invokeText(&r,ee); free(value); return sp;
}
#define GET_SELECTION(name,operation) STUB(WTextComponentPeer,name) { \
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,operation}; \
    invokeText(&r,ee); sp[0].i=r.result; return sp+1; }
GET_SELECTION(getSelectionStart,TEXT_START)
GET_SELECTION(getSelectionEnd,TEXT_END)
STUB(WTextComponentPeer,select) {
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_SELECT,NULL,sp[1].i,sp[2].i};
    invokeText(&r,ee); return sp;
}
STUB(WTextComponentPeer,enableEditing) {
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_EDITABLE,NULL,sp[1].i};
    invokeText(&r,ee); return sp;
}
STUB(WTextFieldPeer,setEchoCharacter) {
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_ECHO,NULL,sp[1].i};
    invokeText(&r,ee); return sp;
}
static stack_item *replaceText(stack_item *sp,struct execenv *ee,BOOL insert)
{
    if (!sp[1].p) { SignalError(ee,"java/lang/NullPointerException","null string"); return sp; }
    WCHAR *value=nativeText((Hjava_lang_String *)sp[1].p);
    TextRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,TEXT_REPLACE,value,sp[2].i,insert ? sp[2].i : sp[3].i};
    invokeText(&r,ee); free(value); return sp;
}
STUB(WTextAreaPeer,insertText) { return replaceText(sp,ee,TRUE); }
STUB(WTextAreaPeer,replaceText) { return replaceText(sp,ee,FALSE); }
