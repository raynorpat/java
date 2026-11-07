/* Clipboard Unicode/newline and ownership contracts: winawt_g
 * 10004B60,10004BD4,10004C31,10004EEB,10021608. */
#include "awt_Component.h"
#include "awt_Clipboard.h"
#include <stdlib.h>
#include <limits.h>
extern "C" {
#include "java_awt_datatransfer_StringSelection.h"
}
void awtClipboardLost()
{
    /* Clear before Java so a new owner installed by a callback is not erased.
     * The volatile stack handle keeps the previous owner alive through GC. */
    HObject *volatile owner=AwtToolkit::GetInstance().TakeClipboardOwner();
    if (owner) execute_java_dynamic_method(EE(),(HObject *)owner,"lostSelectionOwnership","()V");
    if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WClipboard_init_stub(stack_item *sp,ExecEnv *ee)
{
    if (!RegisterClipboardFormatA("JavaString")) SignalError(ee,"java/awt/AWTException","cannot register clipboard format");
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WClipboard_setClipboardText_stub(stack_item *sp,ExecEnv *ee)
{
    Hjava_awt_datatransfer_StringSelection *selection=(Hjava_awt_datatransfer_StringSelection *)sp[1].p;
    if (!selection || !unhand(selection)->data) { SignalError(ee,"java/lang/NullPointerException","null StringSelection"); return sp; }
    AwtToolkit &toolkit=AwtToolkit::GetInstance();
    if (!toolkit.EnsureClipboardRoot()) { SignalError(ee,"java/lang/IllegalStateException","toolkit event loop unavailable"); return sp; }
    Hjava_lang_String *value=unhand(selection)->data;
    int length=javaStringLength(value);
    if (length>(INT_MAX/2-1)/2) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp; }
    WCHAR *source=awtUnicode(value);
    HGLOBAL memory=source ? GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,((SIZE_T)length*2+1)*sizeof(WCHAR)):NULL;
    WCHAR *destination=memory ? (WCHAR *)GlobalLock(memory):NULL;
    if (!destination) { free(source); if(memory)GlobalFree(memory); SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp; }
    int count=0;
    for (int i=0;i<length && source[i];i++) {
        if (source[i]=='\n') destination[count++]='\r';
        destination[count++]=source[i];
    }
    destination[count]=0; GlobalUnlock(memory); free(source);
    HWND window=toolkit.GetHWnd(),previous=GetClipboardOwner();
    if (!OpenClipboard(window)) { GlobalFree(memory); SignalError(ee,"java/lang/IllegalStateException","clipboard unavailable"); return sp; }
    BOOL emptied=previous==window || EmptyClipboard();
    BOOL stored=emptied && SetClipboardData(CF_UNICODETEXT,memory)!=NULL;
    if (stored) toolkit.SetClipboardOwner((HObject *)sp[0].p);
    CloseClipboard();
    if (!stored) { GlobalFree(memory); SignalError(ee,"java/lang/IllegalStateException","cannot set clipboard text"); }
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WClipboard_getClipboardText_stub(stack_item *sp,ExecEnv *ee)
{
    sp[0].p=NULL;
    if (!OpenClipboard(AwtToolkit::GetInstance().GetHWnd())) {
        SignalError(ee,"java/lang/IllegalStateException","clipboard unavailable"); return sp+1;
    }
    HGLOBAL memory=GetClipboardData(CF_UNICODETEXT);
    WCHAR *copy=NULL; size_t length=0;
    if (memory) {
        const WCHAR *source=(const WCHAR *)GlobalLock(memory);
        size_t capacity=GlobalSize(memory)/sizeof(WCHAR);
        if (source) {
            while (length<capacity && source[length]) ++length;
            if (length<(size_t)INT_MAX/sizeof(WCHAR)) copy=(WCHAR *)malloc((length+1)*sizeof(WCHAR));
            if (copy) CopyMemory(copy,source,length*sizeof(WCHAR));
            GlobalUnlock(memory);
        }
    }
    CloseClipboard();
    if (!memory) return sp+1;
    if (!copy) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp+1; }
    int count=0;
    for (size_t i=0;i<length;i++) {
        if (copy[i]=='\r' && i+1<length && copy[i+1]=='\n') ++i;
        copy[count++]=copy[i];
    }
    HArrayOfChar *chars=(HArrayOfChar *)ArrayAlloc(T_CHAR,count);
    if (chars) {
        CopyMemory(unhand(chars)->body,copy,count*sizeof(WCHAR));
        sp[0].p=execute_java_constructor(ee,"java/lang/String",NULL,"([CII)",chars,0,count);
    }
    free(copy); return sp+1;
}
