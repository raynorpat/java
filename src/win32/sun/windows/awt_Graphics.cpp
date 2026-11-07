/* Component graphics foundation, recovered from winawt_g awt_Graphics.cpp.
 * All native state and GDI access are serialized on the toolkit owner thread. */
#include "awt_Graphics.h"
extern "C" {
#include "sun_awt_windows_WGraphics.h"
}
#include <stdlib.h>

struct AwtGraphics {
    AwtSurface *surface;
    COLORREF color;
    RECT clip;
    BOOL clipped;
    LOGFONTW font;
    AwtGraphics() : surface(NULL), color(0), clipped(FALSE) {
        SetRectEmpty(&clip); ZeroMemory(&font,sizeof(font));
    }
    ~AwtGraphics() { if (surface) surface->Release(); }
};
enum GraphicsOperation { FROM_COMPONENT, FROM_GRAPHICS, DESTROY, COLOR, FONT,
    CHANGE_CLIP, REMOVE_CLIP, GET_CLIP, CLEAR_RECT, FILL_RECT };
struct GraphicsRequest {
    Hsun_awt_windows_WGraphics *peer;
    GraphicsOperation operation;
    void *object;
    long v[5];
    BOOL failed;
};
static void graphicsRequest(void *argument)
{
    GraphicsRequest *r = (GraphicsRequest *)argument;
    AwtGraphics *g = (AwtGraphics *)unhand(r->peer)->pData;
    if (r->operation == FROM_COMPONENT) {
        AwtComponent *c = AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->object);
        if (!c || !IsWindow(c->GetHWnd())) { r->failed = TRUE; return; }
        if (!c->surface) c->surface = new AwtSurface(c);
        g = new AwtGraphics; g->surface = c->surface; g->surface->references++;
        g->color = c->foreground;
        RECT outer, client; GetWindowRect(c->GetHWnd(),&outer); GetClientRect(c->GetHWnd(),&client);
        if (!(GetWindowLong(c->GetHWnd(),GWL_STYLE) & WS_CHILD)) {
            MapWindowPoints(c->GetHWnd(),NULL,(POINT *)&client,2);
            unhand(r->peer)->originX = outer.left - client.left;
            unhand(r->peer)->originY = outer.top - client.top;
        }
        unhand(r->peer)->pData = (long)g; return;
    }
    if (r->operation == FROM_GRAPHICS) {
        Hsun_awt_windows_WGraphics *other = (Hsun_awt_windows_WGraphics *)r->object;
        AwtGraphics *source = other ? (AwtGraphics *)unhand(other)->pData : NULL;
        if (!source) { r->failed = TRUE; return; }
        g = new AwtGraphics(*source);
        if (g->surface) g->surface->references++;
        unhand(r->peer)->pData = (long)g; return;
    }
    if (!g) return;
    long ox = unhand(r->peer)->originX, oy = unhand(r->peer)->originY;
    switch (r->operation) {
    case DESTROY: unhand(r->peer)->pData = 0; delete g; return;
    case COLOR: g->color = RGB((r->v[0] >> 16)&255,(r->v[0] >> 8)&255,r->v[0]&255); return;
    case FONT: g->font = *(LOGFONTW *)r->object; return;
    case REMOVE_CLIP: g->clipped = FALSE; return;
    case GET_CLIP:
        r->v[4] = g->clipped;
        r->v[0] = g->clip.left - ox; r->v[1] = g->clip.top - oy;
        r->v[2] = g->clip.right - g->clip.left; r->v[3] = g->clip.bottom - g->clip.top; return;
    case CHANGE_CLIP: {
        RECT rect;
        SetRect(&rect,r->v[0]+ox,r->v[1]+oy,r->v[0]+ox,r->v[1]+oy);
        if (r->v[2] > 0 && r->v[3] > 0) { rect.right += r->v[2]; rect.bottom += r->v[3]; }
        if (!g->clipped || r->v[4]) g->clip = rect;
        else IntersectRect(&g->clip,&g->clip,&rect);
        g->clipped = TRUE; return;
    }
    }
    AwtComponent *c = g->surface ? g->surface->component : NULL;
    if (!c || r->v[2] <= 0 || r->v[3] <= 0) return;
    HWND window = c->GetHWnd(); HDC dc = GetDC(window);
    if (!dc) return;
    int saved = SaveDC(dc);
    if (g->clipped) IntersectClipRect(dc,g->clip.left,g->clip.top,g->clip.right,g->clip.bottom);
    RECT rect = {r->v[0]+ox,r->v[1]+oy,r->v[0]+ox+r->v[2],r->v[1]+oy+r->v[3]};
    HBRUSH brush = CreateSolidBrush(r->operation == CLEAR_RECT ? c->background : g->color);
    FillRect(dc,&rect,brush); DeleteObject(brush);
    RestoreDC(dc,saved); ReleaseDC(window,dc);
}
static void invokeGraphics(GraphicsRequest *r)
{
    AwtToolkit::GetInstance().Invoke(graphicsRequest,r);
    if (r->failed) SignalError(NULL,"java/lang/NullPointerException","invalid graphics source");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WGraphics_##name##_stub(stack_item *sp,struct execenv *ee)
#define PEER ((Hsun_awt_windows_WGraphics *)sp[0].p)
#define SIMPLE(name,op) STUB(name) { GraphicsRequest r = {PEER,op,NULL,{0},FALSE}; invokeGraphics(&r); return sp; }
SIMPLE(dispose,DESTROY) SIMPLE(_0005fdispose,DESTROY) SIMPLE(removeClip,REMOVE_CLIP)
STUB(createFromComponent) {
    GraphicsRequest r = {PEER,FROM_COMPONENT,sp[1].p,{0},FALSE}; invokeGraphics(&r); return sp;
}
STUB(createFromGraphics) {
    GraphicsRequest r = {PEER,FROM_GRAPHICS,sp[1].p,{0},FALSE}; invokeGraphics(&r); return sp;
}
STUB(pSetForeground) {
    GraphicsRequest r = {PEER,COLOR,NULL,{sp[1].i},FALSE}; invokeGraphics(&r); return sp;
}
STUB(pSetFont) {
    Hjava_awt_Font *font = (Hjava_awt_Font *)sp[1].p;
    if (!font) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    LOGFONTW info = {0}; info.lfHeight = -unhand(font)->size;
    info.lfWeight = unhand(font)->style & 1 ? FW_BOLD : FW_NORMAL;
    info.lfItalic = (BYTE)((unhand(font)->style & 2) != 0);
    WCHAR *name = awtUnicode(unhand(font)->name);
    if (name) { lstrcpynW(info.lfFaceName,name,LF_FACESIZE); free(name); }
    GraphicsRequest r = {PEER,FONT,&info,{0},FALSE}; invokeGraphics(&r); return sp;
}
STUB(changeClip) {
    GraphicsRequest r = {PEER,CHANGE_CLIP,NULL,{sp[1].i,sp[2].i,sp[3].i,sp[4].i,sp[5].i},FALSE};
    invokeGraphics(&r); return sp;
}
STUB(getClipBounds) {
    GraphicsRequest r = {PEER,GET_CLIP,NULL,{0},FALSE}; invokeGraphics(&r);
    sp[0].p = r.v[4] ? execute_java_constructor(ee,"java/awt/Rectangle",NULL,"(IIII)",r.v[0],r.v[1],r.v[2],r.v[3]) : NULL;
    return sp + 1;
}
#define RECT_STUB(name,op) STUB(name) { GraphicsRequest r = {PEER,op,NULL,{sp[1].i,sp[2].i,sp[3].i,sp[4].i},FALSE}; invokeGraphics(&r); return sp; }
RECT_STUB(clearRect,CLEAR_RECT) RECT_STUB(fillRect,FILL_RECT)
