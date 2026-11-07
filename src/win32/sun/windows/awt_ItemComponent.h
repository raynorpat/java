/* Shared item measurement/drawing: winawt_g100086A6/100086E7. */
#ifndef AWT_ITEM_COMPONENT_H
#define AWT_ITEM_COMPONENT_H
#include "awt_Component.h"
#include "awt_Font.h"
#include <stdlib.h>
class AwtItemComponent : public AwtComponent {
public:
    AwtItemComponent(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) {}
    virtual void MeasureItem(MEASUREITEMSTRUCT *item) {
        HObject *target = (HObject *)GetTarget();
        HObject *font = (HObject *)execute_java_dynamic_method(EE(),target,"getFont","()Ljava/awt/Font;");
        if (IsDisposed()) return;
        HObject *metrics = (HObject *)execute_java_dynamic_method(EE(),target,"getFontMetrics","(Ljava/awt/Font;)Ljava/awt/FontMetrics;",font);
        if (IsDisposed()) return;
        if (metrics) item->itemHeight = execute_java_dynamic_method(EE(),metrics,"getHeight","()I");
        if (IsDisposed()) return;
    }
    virtual void DrawItem(DRAWITEMSTRUCT *item) {
        HDC dc = item->hDC; int saved = SaveDC(dc); RECT rect = item->rcItem;
        BOOL selected = (item->itemState & ODS_SELECTED) != 0;
        COLORREF back = selected ? GetSysColor(COLOR_HIGHLIGHT) : background;
        HBRUSH brush = CreateSolidBrush(back); FillRect(dc,&rect,brush); DeleteObject(brush);
        if (item->itemID != (UINT)-1) {
            Hjava_lang_String *value = (Hjava_lang_String *)execute_java_dynamic_method(EE(),(HObject *)GetTarget(),"getItem","(I)Ljava/lang/String;",item->itemID);
            if (IsDisposed()) goto drawDone;
            if (value) {
                Hjava_awt_Font *javaFont = (Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)GetTarget(),"getFont","()Ljava/awt/Font;");
                if (IsDisposed()) goto drawDone;
                SIZE size = awtFontText(dc,javaFont,value,0,0,FALSE);
                if (IsDisposed()) goto drawDone;
                SetBkColor(dc,back); SetBkMode(dc,TRANSPARENT);
                SetTextColor(dc,selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : IsWindowEnabled(hwnd) ? foreground : GetSysColor(COLOR_GRAYTEXT));
                IntersectClipRect(dc,rect.left,rect.top,rect.right,rect.bottom);
                awtFontText(dc,javaFont,value,rect.left+1,(rect.top+rect.bottom-size.cy)/2,TRUE);
                if (IsDisposed()) goto drawDone;
            }
        }
        if ((item->itemState & ODS_FOCUS) && (item->itemAction & (ODA_DRAWENTIRE|ODA_FOCUS))) DrawFocusRect(dc,&rect);
drawDone:
        RestoreDC(dc,saved);
    }
};
#endif
