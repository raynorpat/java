/* Printing reconstructed from winawt_g 10014777, 10014A15, 10014BC3,
 * 10014C4C, 10015110, 1001523A and 1001EDE0. State is toolkit-owned. */
#include "awt_PrintJob.h"
#include <commdlg.h>
#include <stdlib.h>
extern "C" {
#include "sun_awt_windows_WPrintJob.h"
#include "java_awt_Dimension.h"
__declspec(dllexport) stack_item *Java_sun_awt_windows_WGraphics_createFromHDC_stub(stack_item *,struct execenv *);
__declspec(dllexport) stack_item *Java_sun_awt_windows_WGraphics_dispose_stub(stack_item *,struct execenv *);
}
struct PrintPage {
    AwtSurface *surface;
    long owner;
    BOOL open;
    PrintPage *next;
    PrintPage(AwtSurface *s,long graphics,PrintPage *n) : surface(s),owner(graphics),open(TRUE),next(n) {++s->references;}
    ~PrintPage() {surface->Release();}
};
struct AwtPrintJob {
    HDC dc;
    PrintPage *pages;
    AwtPrintJob *next;
    unsigned references;
    BOOL ended;
    AwtPrintJob(HDC h) : dc(h),pages(NULL),next(NULL),references(1),ended(FALSE) {}
    void Retain() {++references;}
    void Release() {if(!--references)delete this;}
    ~AwtPrintJob() {
        while(pages){PrintPage *page=pages;pages=page->next;delete page;}
        if(dc)DeleteDC(dc);
    }
};
static AwtPrintJob *jobs;
class PrintLease {
    AwtPrintJob *job;
public:
    PrintLease(AwtPrintJob *j) : job(j) {if(job)job->Retain();}
    ~PrintLease(){if(job)job->Release();}
};
static void closePage(AwtPrintJob *job,PrintPage *page)
{
    AwtSurfaceLock lock(page->surface);
    if(page->open){page->open=FALSE;EndPage(job->dc);}
    page->surface->dc=NULL;
}
BOOL awtClosePrintGraphics(Hsun_awt_windows_WGraphics *graphics)
{
    AwtSurface *surface=awtGraphicsSurface(graphics);
    if(!surface)return FALSE;
    for(AwtPrintJob *job=jobs;job;job=job->next)
        for(PrintPage *page=job->pages;page;page=page->next) if(page->surface==surface && page->owner==unhand(graphics)->pData) {
            PrintLease lease(job);closePage(job,page);return TRUE;
        }
    return FALSE;
}
static void endJob(AwtPrintJob *job)
{
    if(job->ended)return;
    PrintLease lease(job);job->ended=TRUE;
    for(PrintPage *page=job->pages;page;page=page->next) closePage(job,page);
    EndDoc(job->dc);
    AwtPrintJob **link=&jobs;while(*link && *link!=job)link=&(*link)->next;
    if(*link)*link=job->next;
    job->Release();
}
void awtCleanupPrintJobs() {while(jobs)endJob(jobs);}
static UINT_PTR CALLBACK printHook(HWND window,UINT message,WPARAM,LPARAM)
{
    if(message==WM_INITDIALOG)SetForegroundWindow(window);return 0;
}
enum PrintOperation {CREATE_PRINT,CLOSE_PRINT,END_PRINT,PRINT_COMPONENT};
struct PrintRequest {
    Hsun_awt_windows_WGraphics *graphics;
    Hsun_awt_windows_WPrintJob *peer;
    Hsun_awt_windows_WComponentPeer *component;
    PrintOperation operation;
    char *error;
};
static void printRequest(void *argument)
{
    PrintRequest *r=(PrintRequest *)argument;
    if(r->operation==PRINT_COMPONENT) {
        if(!unhand(r->graphics)->pData)return;
        AwtComponent *component=AwtComponent::Get(r->component);
        if(!component){r->error="invalid peer";return;}
        AwtSurface *surface=awtGraphicsSurface(r->graphics);
        AwtPrintJob *printing=NULL;
        for(AwtPrintJob *job=jobs;job && !printing;job=job->next)
            for(PrintPage *page=job->pages;page;page=page->next)
                if(page->surface==surface){printing=job;break;}
        /* WM_PRINT can enter Java and end this job. Its HDC must remain
         * allocated until the native control and saved DC state unwind. */
        PrintLease printingLease(printing);
        AwtComponentLease pin(component);AwtDC lease;
        if(awtGraphicsDC(r->graphics,&lease)) {
            if(!component->IsDisposed()) {
                OffsetWindowOrgEx(lease.dc,-unhand(r->graphics)->originX,-unhand(r->graphics)->originY,NULL);
                SendMessage(component->GetHWnd(),WM_PRINT,(WPARAM)lease.dc,23);
            }
            awtReleaseDC(&lease);
        }
        return;
    }
    if(r->operation==CLOSE_PRINT) {
        if(awtClosePrintGraphics(r->graphics)) {
            stack_item args[1];args[0].p=r->graphics;
            Java_sun_awt_windows_WGraphics_dispose_stub(args,EE());
        }
        return;
    }
    AwtPrintJob *job=(AwtPrintJob *)unhand(r->peer)->pGraphics;
    if(r->operation==END_PRINT) {
        unhand(r->peer)->pGraphics=0;if(job)endJob(job);return;
    }
    if(!job) {
        PRINTDLGA dialog={0};dialog.lStructSize=sizeof(dialog);
        dialog.Flags=4492;dialog.lpfnPrintHook=printHook;
        BOOL accepted=PrintDlgA(&dialog);
        if(accepted && dialog.hDC) {
            DEVMODEA *mode=dialog.hDevMode?(DEVMODEA *)GlobalLock(dialog.hDevMode):NULL;
            if(mode && (mode->dmFields&DM_PRINTQUALITY)) {
                if(mode->dmPrintQuality>0)unhand(r->peer)->pageResolution=mode->dmPrintQuality;
                else if(mode->dmFields&DM_YRESOLUTION)unhand(r->peer)->pageResolution=mode->dmYResolution;
            }
            int dx=GetDeviceCaps(dialog.hDC,LOGPIXELSX),dy=GetDeviceCaps(dialog.hDC,LOGPIXELSY);
            Hjava_awt_Dimension *size=unhand(r->peer)->pageDimension;
            if(size && dx>0 && dy>0) {
                unhand(size)->width=(long)((double)GetDeviceCaps(dialog.hDC,PHYSICALWIDTH)*72/dx+0.5);
                unhand(size)->height=(long)((double)GetDeviceCaps(dialog.hDC,PHYSICALHEIGHT)*72/dy+0.5);
            }
            if(mode)GlobalUnlock(dialog.hDevMode);
            WCHAR *wide=awtUnicode(unhand(r->peer)->title);
            int length=wide?WideCharToMultiByte(CP_ACP,0,wide,-1,NULL,0,NULL,NULL):0;
            char *title=length?(char *)malloc(length):NULL;
            if(title)WideCharToMultiByte(CP_ACP,0,wide,-1,title,length,NULL,NULL);
            free(wide);
            DOCINFOA info={0};info.cbSize=sizeof(info);info.lpszDocName=title;
            int result=StartDocA(dialog.hDC,&info);free(title);
            if(result>0) {
                job=new AwtPrintJob(dialog.hDC);job->next=jobs;jobs=job;
                unhand(r->peer)->pGraphics=(long)job;
            } else {DeleteDC(dialog.hDC);r->error="printing StartDoc failed";}
        } else if(dialog.hDC)DeleteDC(dialog.hDC);
        if(dialog.hDevMode)GlobalFree(dialog.hDevMode);
        if(dialog.hDevNames)GlobalFree(dialog.hDevNames);
        /* initJob obtains the printer DC but starts no page. Cancellation
         * leaves both peer fields zero and needs no graphics allocation. */
        return;
    }
    PrintLease pin(job);
    if(job->ended)return;
    for(PrintPage *page=job->pages;page;page=page->next)if(page->open)closePage(job,page);
    if(job->ended)return;
    if(StartPage(job->dc)<=0){r->error="printing StartPage failed";return;}
    SetMapMode(job->dc,MM_ISOTROPIC);SetWindowExtEx(job->dc,72,72,NULL);
    SetViewportExtEx(job->dc,GetDeviceCaps(job->dc,LOGPIXELSX),GetDeviceCaps(job->dc,LOGPIXELSY),NULL);
    SetBkMode(job->dc,TRANSPARENT);
    stack_item args[2];args[0].p=r->graphics;args[1].i=(long)job->dc;
    Java_sun_awt_windows_WGraphics_createFromHDC_stub(args,EE());
    AwtSurface *surface=awtGraphicsSurface(r->graphics);
    if(surface)job->pages=new PrintPage(surface,unhand(r->graphics)->pData,job->pages);
    else EndPage(job->dc);
}
#define PRINT_STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_##name##_stub(stack_item *sp,struct execenv *ee)
static void invokePrint(PrintRequest *request,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(printRequest,request);
    if(request->error)SignalError(ee,request->operation==PRINT_COMPONENT?"java/lang/NullPointerException":"java/lang/InternalError",request->error);
}
PRINT_STUB(WGraphics_createFromPrintJob) {
    if(!sp[1].p){SignalError(ee,"java/lang/NullPointerException","null PrintJob");return sp;}
    PrintRequest r={(Hsun_awt_windows_WGraphics *)sp[0].p,(Hsun_awt_windows_WPrintJob *)sp[1].p,NULL,CREATE_PRINT,NULL};
    invokePrint(&r,ee);return sp;
}
PRINT_STUB(WGraphics_close) {
    PrintRequest r={(Hsun_awt_windows_WGraphics *)sp[0].p,NULL,NULL,CLOSE_PRINT,NULL};invokePrint(&r,ee);return sp;
}
PRINT_STUB(WPrintJob_end) {
    PrintRequest r={NULL,(Hsun_awt_windows_WPrintJob *)sp[0].p,NULL,END_PRINT,NULL};invokePrint(&r,ee);return sp;
}
PRINT_STUB(WGraphics_print) {
    PrintRequest r={(Hsun_awt_windows_WGraphics *)sp[0].p,NULL,(Hsun_awt_windows_WComponentPeer *)sp[1].p,PRINT_COMPONENT,NULL};invokePrint(&r,ee);return sp;
}
