/* GDI raster operations recovered from winawt_g 10012368..100143C0.
 * Native state and GDI resources are serialized on the toolkit owner thread. */
#include "awt_Graphics.h"
#include "awt_Font.h"
#include "awt_Image.h"
extern "C" {
#include "java_awt_Color.h"
}
#include <stdlib.h>
#include <math.h>
#include <wchar.h>

AwtSurface::~AwtSurface()
{
    if (dc && oldBitmap) SelectObject(dc,oldBitmap);
    if (palette && oldPalette) SelectPalette(dc,oldPalette,FALSE);
    if (palette) awtReleaseImagePalette();
    if (bitmap) DeleteObject(bitmap);
    if (dc && ownsDC) DeleteDC(dc);
    free(alpha); free(coverage); free(rows); free(ditherErrors);
}
struct AwtGraphics {
    AwtSurface *surface;
    COLORREF color, xorColor;
    RECT clip;
    BOOL clipped, xormode;
    LOGFONTW font;
    AwtGraphics() : surface(NULL), color(0), xorColor(0), clipped(FALSE), xormode(FALSE) {
        SetRectEmpty(&clip); ZeroMemory(&font,sizeof(font));
    }
    ~AwtGraphics() { if (surface) surface->Release(); }
};
static AwtGraphics *getGraphics(Hsun_awt_windows_WGraphics *peer)
{ return peer ? (AwtGraphics *)unhand(peer)->pData : NULL; }
AwtSurface *awtGraphicsSurface(Hsun_awt_windows_WGraphics *peer)
{ AwtGraphics *g = getGraphics(peer); return g ? g->surface : NULL; }
COLORREF awtGraphicsColor(Hsun_awt_windows_WGraphics *peer)
{ AwtGraphics *g = getGraphics(peer); return g ? g->color ^ (g->xormode ? g->xorColor : 0) : 0; }
COLORREF awtGraphicsXORColor(Hsun_awt_windows_WGraphics *peer)
{ AwtGraphics *g = getGraphics(peer); return g ? g->xorColor : 0; }
BOOL awtGraphicsXOR(Hsun_awt_windows_WGraphics *peer)
{ AwtGraphics *g = getGraphics(peer); return g && g->xormode; }
BOOL awtGraphicsDC(Hsun_awt_windows_WGraphics *peer,AwtDC *lease)
{
    ZeroMemory(lease,sizeof(*lease));
    AwtGraphics *g = getGraphics(peer); AwtSurface *s = g ? g->surface : NULL;
    if (!s) return FALSE;
    if (s->dc) lease->dc = s->dc;
    else if (s->component && IsWindow(s->component->GetHWnd())) {
        lease->window = s->component->GetHWnd(); lease->dc = GetDC(lease->window);
    }
    if (!lease->dc) return FALSE;
    lease->saved = SaveDC(lease->dc);
    HPALETTE palette=s->palette?s->palette:awtImagePalette();
    if(palette){SelectPalette(lease->dc,palette,FALSE);RealizePalette(lease->dc);}
    if (g->clipped) IntersectClipRect(lease->dc,g->clip.left,g->clip.top,g->clip.right,g->clip.bottom);
    SetBkMode(lease->dc,TRANSPARENT); SetROP2(lease->dc,g->xormode ? R2_XORPEN : R2_COPYPEN);
    SetTextColor(lease->dc,g->color|0x02000000); SetPolyFillMode(lease->dc,ALTERNATE);
    return TRUE;
}
void awtReleaseDC(AwtDC *lease)
{
    if (!lease->dc) return;
    RestoreDC(lease->dc,lease->saved);
    if (lease->window) ReleaseDC(lease->window,lease->dc);
    GdiFlush(); lease->dc = NULL;
}
enum GraphicsOperation { FROM_COMPONENT, FROM_GRAPHICS, FROM_IMAGE, FROM_HDC,
    DESTROY, COLOR, FONT, PAINT, XOR_MODE, CHANGE_CLIP, REMOVE_CLIP, GET_CLIP,
    CLEAR_RECT, FILL_RECT, DRAW_RECT, LINE, COPY_AREA, DRAW_ROUND, FILL_ROUND,
    DRAW_OVAL, FILL_OVAL, DRAW_ARC, FILL_ARC, POLYGON, POLYLINE, FILL_POLYGON,
    TEXT_W, TEXT_A };
struct GraphicsRequest {
    Hsun_awt_windows_WGraphics *peer;
    GraphicsOperation operation;
    void *object;
    long v[6];
    BOOL failed;
    LOGFONTW *textFont;
};
static void graphicsRequest(void *argument)
{
    GraphicsRequest *r = (GraphicsRequest *)argument;
    AwtGraphics *g = getGraphics(r->peer);
    if (r->operation == FROM_COMPONENT) {
        AwtComponent *c = AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->object);
        if (!r->object) { r->failed = TRUE; return; }
        /* A queued paint can retain its Java peer while native retirement has
         * already cleared pData. Such graphics must remain safely inert. */
        if (!c || !IsWindow(c->GetHWnd())) {
            unhand(r->peer)->pData=(long)new AwtGraphics; return;
        }
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
        AwtGraphics *source = getGraphics((Hsun_awt_windows_WGraphics *)r->object);
        if (!source) { r->failed = TRUE; return; }
        g = new AwtGraphics(*source);
        if (g->surface) g->surface->references++;
        unhand(r->peer)->pData = (long)g; return;
    }
    if (r->operation == FROM_IMAGE || r->operation == FROM_HDC) {
        AwtSurface *s;
        if (r->operation == FROM_IMAGE) {
            s = awtImageSurface((Hsun_awt_image_ImageRepresentation *)r->object,FALSE);
            if (s) s->references++;
        } else {
            s = r->object ? new AwtSurface(NULL) : NULL;
            if (s) s->dc = (HDC)r->object; /* borrowed HDC, never deleted */
        }
        if (!s) { r->failed = TRUE; return; }
        g = new AwtGraphics; g->surface = s; unhand(r->peer)->pData = (long)g; return;
    }
    if (!g) return;
    long ox = unhand(r->peer)->originX, oy = unhand(r->peer)->originY;
    switch (r->operation) {
    case DESTROY: unhand(r->peer)->pData = 0; delete g; return;
    case COLOR: g->color = RGB((r->v[0] >> 16)&255,(r->v[0] >> 8)&255,r->v[0]&255); return;
    case FONT: g->font = *(LOGFONTW *)r->object; return;
    case PAINT: g->xormode = FALSE; return;
    case XOR_MODE: g->xormode = TRUE; g->xorColor = RGB((r->v[0]>>16)&255,(r->v[0]>>8)&255,r->v[0]&255); return;
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
    AwtDC lease; if (!awtGraphicsDC(r->peer,&lease)) return; HDC dc = lease.dc;
    COLORREF color = awtGraphicsColor(r->peer);
    HBRUSH brush = CreateSolidBrush(color|0x02000000); HPEN pen = CreatePen(PS_SOLID,1,color|0x02000000);
    SelectObject(dc,brush); SelectObject(dc,pen);
    DWORD rop = g->xormode ? 0x5a0049 : PATCOPY;
    int x = r->v[0]+ox, y = r->v[1]+oy, w = r->v[2], h = r->v[3];
    switch (r->operation) {
    case CLEAR_RECT:
        if (w > 0 && h > 0) {
            AwtSurface *s = g->surface;
            HBRUSH bg = CreateSolidBrush((s->component ? s->component->background : s->background)|0x02000000);
            RECT rect = {x,y,x+w,y+h}; FillRect(dc,&rect,bg); DeleteObject(bg);
        } break;
    case FILL_RECT: if (w > 0 && h > 0) PatBlt(dc,x,y,w,h,rop); break;
    case DRAW_RECT:
        if (w >= 0 && h >= 0) {
            PatBlt(dc,x,y,w+1,1,rop); PatBlt(dc,x+w,y,1,h+1,rop);
            PatBlt(dc,x,y,1,h+1,rop); PatBlt(dc,x,y+h,w+1,1,rop);
        } break;
    case LINE: {
        int x2 = w+ox, y2 = h+oy;
        if (x == x2) PatBlt(dc,x,min(y,y2),1,abs(y2-y)+1,rop);
        else if (y == y2) PatBlt(dc,min(x,x2),y,abs(x2-x)+1,1,rop);
        else { MoveToEx(dc,x,y,NULL); LineTo(dc,x2,y2); PatBlt(dc,x2,y2,1,1,rop); }
        break;
    }
    case COPY_AREA:
        if (w > 0 && h > 0) {
            RECT dest = {x+r->v[4],y+r->v[5],x+r->v[4]+w,y+r->v[5]+h};
            if (!g->clipped || IntersectRect(&dest,&dest,&g->clip)) {
                SelectClipRgn(dc,NULL);
                BitBlt(dc,dest.left,dest.top,dest.right-dest.left,dest.bottom-dest.top,dc,
                    dest.left-r->v[4],dest.top-r->v[5],g->xormode ? SRCINVERT : SRCCOPY);
            }
        } break;
    case DRAW_ROUND: case FILL_ROUND: case DRAW_OVAL: case FILL_OVAL:
        if (w >= 0 && h >= 0 && ((r->operation == DRAW_ROUND || r->operation == DRAW_OVAL) || (w && h))) {
            BOOL fill = r->operation == FILL_ROUND || r->operation == FILL_OVAL;
            SelectObject(dc,GetStockObject(fill ? NULL_PEN : NULL_BRUSH));
            if (r->operation == DRAW_ROUND || r->operation == FILL_ROUND) RoundRect(dc,x,y,x+w+1,y+h+1,r->v[4],r->v[5]);
            else Ellipse(dc,x,y,x+w+1,y+h+1);
        } break;
    case DRAW_ARC: case FILL_ARC:
        if (w > 0 && h > 0 && r->v[5]) {
            int start = r->v[4], end = start+r->v[5], sx,sy,ex,ey;
            if (r->v[5] < 0) { int t = start; start = end; end = t; }
            if (r->v[5] >= 360 || r->v[5] <= -360) { sx=ex=x+w; sy=ey=y+h/2; }
            else {
                double a = 3.1415926535*2.0/360.0;
                sx=x+w/2+(int)(cos(start*a)*w); sy=y+h/2-(int)(sin(start*a)*h);
                ex=x+w/2+(int)(cos(end*a)*w); ey=y+h/2-(int)(sin(end*a)*h);
            }
            SelectObject(dc,GetStockObject(r->operation == FILL_ARC ? NULL_PEN : NULL_BRUSH));
            if (r->operation == FILL_ARC) Pie(dc,x,y,x+w+1,y+h+1,sx,sy,ex,ey);
            else Arc(dc,x,y,x+w+1,y+h+1,sx,sy,ex,ey);
        } break;
    case POLYGON: case POLYLINE: case FILL_POLYGON: {
        POINT *points = (POINT *)r->object; int count = r->v[0];
        for (int i=0;i<count;i++) { points[i].x += ox; points[i].y += oy; }
        if (count) {
            SelectObject(dc,GetStockObject(r->operation == FILL_POLYGON ? NULL_PEN : NULL_BRUSH));
            if (r->operation == FILL_POLYGON) Polygon(dc,points,count);
            else Polyline(dc,points,count);
        } break;
    }
    case TEXT_W: case TEXT_A: {
        HFONT font = CreateFontIndirectW(r->textFont ? r->textFont : &g->font);
        HGDIOBJ old = font ? SelectObject(dc,font) : NULL; SIZE size = {0,0};
        /* The binary uses the primary font ascent for every descriptor. */
        HFONT primary = CreateFontIndirectW(&g->font); TEXTMETRICW tm;
        HGDIOBJ segment = primary ? SelectObject(dc,primary) : NULL; GetTextMetricsW(dc,&tm);
        if (primary) { SelectObject(dc,segment); DeleteObject(primary); }
        if (g->xormode) BeginPath(dc);
        if (r->operation == TEXT_W) {
            TextOutW(dc,x,y-tm.tmAscent,(WCHAR *)r->object,w);
            GetTextExtentPoint32W(dc,(WCHAR *)r->object,w,&size);
        } else {
            TextOutA(dc,x,y-tm.tmAscent,(char *)r->object,w);
            GetTextExtentPoint32A(dc,(char *)r->object,w,&size);
        }
        if (g->xormode) { EndPath(dc); FillPath(dc); }
        if (font) { SelectObject(dc,old); DeleteObject(font); }
        r->v[4] = size.cx; break;
    }
    }
    awtReleaseDC(&lease); DeleteObject(pen); DeleteObject(brush);
}
static void invokeGraphics(GraphicsRequest *r)
{
    AwtToolkit::GetInstance().Invoke(graphicsRequest,r);
    if (r->failed) SignalError(NULL,"java/lang/NullPointerException","invalid graphics source");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WGraphics_##name##_stub(stack_item *sp,struct execenv *ee)
#define PEER ((Hsun_awt_windows_WGraphics *)sp[0].p)
#define SIMPLE(name,op) STUB(name) { GraphicsRequest r = {PEER,op,NULL,{0},FALSE}; invokeGraphics(&r); return sp; }
SIMPLE(dispose,DESTROY) SIMPLE(_0005fdispose,DESTROY) SIMPLE(removeClip,REMOVE_CLIP) SIMPLE(setPaintMode,PAINT)
#define CREATE(name,op) STUB(name) { GraphicsRequest r = {PEER,op,sp[1].p,{0},FALSE}; invokeGraphics(&r); return sp; }
CREATE(createFromComponent,FROM_COMPONENT) CREATE(createFromGraphics,FROM_GRAPHICS)
CREATE(imageCreate,FROM_IMAGE) CREATE(createFromHDC,FROM_HDC)
STUB(pSetForeground) {
    GraphicsRequest r = {PEER,COLOR,NULL,{sp[1].i},FALSE}; invokeGraphics(&r); return sp;
}
STUB(setXORMode) {
    Hjava_awt_Color *color = (Hjava_awt_Color *)sp[1].p;
    if (!color) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    GraphicsRequest r = {PEER,XOR_MODE,NULL,{unhand(color)->value},FALSE}; invokeGraphics(&r); return sp;
}
STUB(pSetFont) {
    Hjava_awt_Font *font = (Hjava_awt_Font *)sp[1].p;
    if (!font) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    LOGFONTW info;
    if (!awtFontInfo(font,NULL,&info)) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp; }
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
RECT_STUB(clearRect,CLEAR_RECT) RECT_STUB(fillRect,FILL_RECT) RECT_STUB(drawRect,DRAW_RECT)
RECT_STUB(drawLine,LINE) RECT_STUB(drawOval,DRAW_OVAL) RECT_STUB(fillOval,FILL_OVAL)
#define SIX_STUB(name,op) STUB(name) { GraphicsRequest r = {PEER,op,NULL,{sp[1].i,sp[2].i,sp[3].i,sp[4].i,sp[5].i,sp[6].i},FALSE}; invokeGraphics(&r); return sp; }
SIX_STUB(copyArea,COPY_AREA) SIX_STUB(drawRoundRect,DRAW_ROUND) SIX_STUB(fillRoundRect,FILL_ROUND)
SIX_STUB(drawArc,DRAW_ARC) SIX_STUB(fillArc,FILL_ARC)
static void polygon(stack_item *sp,execenv *ee,GraphicsOperation op)
{
    HArrayOfInt *xs=(HArrayOfInt *)sp[1].p,*ys=(HArrayOfInt *)sp[2].p; int n=sp[3].i;
    if (!xs || !ys) { SignalError(ee,"java/lang/NullPointerException",NULL); return; }
    if (n < 0 || n > obj_length(xs) || n > obj_length(ys)) { SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL); return; }
    POINT *points=(POINT *)malloc(((size_t)n+1)*sizeof(POINT));
    if (!points) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return; }
    for (int i=0;i<n;i++) { points[i].x=unhand(xs)->body[i]; points[i].y=unhand(ys)->body[i]; }
    if (op == POLYGON && n && (points[0].x != points[n-1].x || points[0].y != points[n-1].y)) points[n++]=points[0];
    GraphicsRequest r={PEER,op,points,{n},FALSE}; invokeGraphics(&r); free(points);
}
STUB(drawPolygon) { polygon(sp,ee,POLYGON); return sp; }
STUB(drawPolyline) { polygon(sp,ee,POLYLINE); return sp; }
STUB(fillPolygon) { polygon(sp,ee,FILL_POLYGON); return sp; }
static long text(stack_item *sp,execenv *ee,BOOL multi,BOOL bytes)
{
    int a=multi?3:1; HArrayOfChar *chars=(HArrayOfChar *)sp[a].p;
    long offset=multi && bytes ? 0 : sp[a+1].i;
    long count=sp[a+(multi && bytes ? 1 : 2)].i;
    int xy=a+(multi && bytes ? 2 : 3); LOGFONTW info;
    if (!chars || (multi && (!sp[1].p || !sp[2].p))) { SignalError(ee,"java/lang/NullPointerException",NULL); return 0; }
    if (offset < 0 || count < 0 || offset > obj_length(chars) || count > obj_length(chars)-offset) {
        SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL); return 0;
    }
    if (multi && !awtFontInfo((Hjava_awt_Font *)sp[1].p,(Hsun_awt_FontDescriptor *)sp[2].p,&info)) {
        SignalError(ee,"java/lang/OutOfMemoryError",NULL); return 0;
    }
    size_t size=((size_t)count+1)*(bytes?1:sizeof(WCHAR));
    void *data=malloc(size);
    if (!data) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return 0; }
    if (bytes) {
        memcpy(data,unhand((HArrayOfByte *)chars)->body+offset,count); ((char *)data)[count]=0;
    } else {
        memcpy(data,unhand(chars)->body+offset,count*sizeof(WCHAR)); ((WCHAR *)data)[count]=0;
    }
    /* drawBytes is ACP -> Unicode, and the single-font path stops at NUL. */
    if (bytes && !multi) {
        WCHAR *wide=(WCHAR *)malloc(((size_t)count+1)*sizeof(WCHAR));
        if (!wide) { free(data); SignalError(ee,"java/lang/OutOfMemoryError",NULL); return 0; }
        count=MultiByteToWideChar(CP_ACP,0,(char *)data,-1,wide,count+1);
        free(data); data=wide; count=count?count-1:0; bytes=FALSE;
    } else if (!multi) count=(long)wcslen((WCHAR *)data);
    GraphicsRequest r={PEER,bytes?TEXT_A:TEXT_W,data,{sp[xy].i,sp[xy+1].i,count},FALSE,multi?&info:NULL};
    invokeGraphics(&r); free(data); return r.v[4];
}
STUB(drawSFChars) { text(sp,ee,FALSE,FALSE); return sp; }
STUB(drawBytes) { text(sp,ee,FALSE,TRUE); return sp; }
STUB(drawMFCharsSegment) { sp[0].i=text(sp,ee,TRUE,FALSE); return sp+1; }
STUB(drawMFCharsConvertedSegment) { sp[0].i=text(sp,ee,TRUE,TRUE); return sp+1; }
