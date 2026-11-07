/* Bounded, process-owned GUI demo runner. No desktop-wide input, window
 * broadcasts, vendor writes, printer operations or clipboard operations. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WINDOWS 32
typedef struct {
    HWND window,content;
    char title[512],className[128];
    int width,height,contentWidth,contentHeight,colors,contentColors,responsive;
    unsigned long hash,contentHash;
} Sample;
typedef struct {DWORD process;int count;Sample items[MAX_WINDOWS];} Windows;
typedef struct {DWORD process;HWND panel,canvas;long panelArea,canvasArea;} Content;
typedef struct {DWORD process;const char *label,*className;HWND result;LONG top;int highest;} Find;
static DWORD ownedProcess;
static FILE *inputTrace;
typedef struct {HANDLE process,done;DWORD pid;char directory[MAX_PATH];} Watchdog;
static DWORD WINAPI watchdog(void *argument)
{
    Watchdog *guard=(Watchdog *)argument;char path[MAX_PATH];FILE *file;
    if(WaitForSingleObject(guard->done,30000)==WAIT_OBJECT_0)return 0;
    /* This handle identifies the child even if its numeric PID is reused.
     * Killing the child also releases any synchronous cross-process UI call. */
    TerminateProcess(guard->process,137);
    _snprintf(path,sizeof(path),"%s/result.json",guard->directory);file=fopen(path,"wb");
    if(file){fprintf(file,"{\"status\":\"watchdog-timeout\",\"pid\":%lu,\"forcedTermination\":true}\n",guard->pid);fclose(file);}
    ExitProcess(124);return 0;
}

static int owned(HWND window)
{
    DWORD process=0;GetWindowThreadProcessId(window,&process);
    return process==ownedProcess && IsWindow(window);
}
static int responsive(HWND window)
{
    DWORD_PTR result;
    return owned(window) && SendMessageTimeoutA(window,WM_NULL,0,0,
        SMTO_ABORTIFHUNG|SMTO_BLOCK,500,&result)!=0;
}
static BOOL CALLBACK listWindow(HWND window,LPARAM value)
{
    Windows *windows=(Windows *)value;DWORD process;Sample *sample;RECT rect;
    GetWindowThreadProcessId(window,&process);
    if(process!=windows->process || !IsWindowVisible(window) || GetWindow(window,GW_OWNER))return TRUE;
    GetClientRect(window,&rect);if(rect.right<24 || rect.bottom<24)return TRUE;
    if(windows->count==MAX_WINDOWS)return TRUE;
    sample=&windows->items[windows->count++];ZeroMemory(sample,sizeof(*sample));sample->window=window;
    GetWindowTextA(window,sample->title,sizeof(sample->title));GetClassNameA(window,sample->className,sizeof(sample->className));
    sample->width=rect.right;sample->height=rect.bottom;return TRUE;
}
static int compareWindows(const void *left,const void *right)
{
    const Sample *a=(const Sample *)left,*b=(const Sample *)right;
    int result=lstrcmpA(a->title,b->title);RECT ra,rb;
    if(result)return result;
    GetWindowRect(a->window,&ra);GetWindowRect(b->window,&rb);
    if(ra.top!=rb.top)return ra.top<rb.top?-1:1;
    return ra.left<rb.left?-1:ra.left!=rb.left;
}
static void enumerate(Windows *windows)
{
    windows->process=ownedProcess;windows->count=0;
    EnumWindows(listWindow,(LPARAM)windows);
    qsort(windows->items,windows->count,sizeof(Sample),compareWindows);
}
static void alignWindows(Windows *windows,const Windows *before)
{
    Windows copy=*windows;BYTE used[MAX_WINDOWS];int i,j;
    ZeroMemory(used,sizeof(used));windows->count=0;
    for(i=0;i<before->count;i++)for(j=0;j<copy.count;j++)if(copy.items[j].window==before->items[i].window) {
        windows->items[windows->count++]=copy.items[j];used[j]=1;break;
    }
    for(j=0;j<copy.count;j++)if(!used[j])windows->items[windows->count++]=copy.items[j];
}
static BOOL CALLBACK findContent(HWND window,LPARAM value)
{
    Content *content=(Content *)value;char name[128];RECT rect;long area;
    if(!owned(window) || !IsWindowVisible(window))return TRUE;
    GetClassNameA(window,name,sizeof(name));GetClientRect(window,&rect);area=rect.right*rect.bottom;
    if(strstr(name,"Canvas") && area>content->canvasArea){content->canvas=window;content->canvasArea=area;}
    if(strstr(name,"Panel") && area>content->panelArea){content->panel=window;content->panelArea=area;}
    return TRUE;
}
static HWND contentWindow(HWND window)
{
    Content content;ZeroMemory(&content,sizeof(content));content.process=ownedProcess;
    EnumChildWindows(window,findContent,(LPARAM)&content);
    if(content.canvas && content.canvasArea>=content.panelArea/2)return content.canvas;
    return content.panel?content.panel:window;
}
static BOOL CALLBACK findChild(HWND window,LPARAM value)
{
    Find *find=(Find *)value;char label[512],name[128];RECT rect;
    if(!owned(window) || !IsWindowVisible(window) || !IsWindowEnabled(window))return TRUE;
    GetWindowTextA(window,label,sizeof(label));GetClassNameA(window,name,sizeof(name));
    if((!find->label || !lstrcmpiA(label,find->label)) && (!find->className || strstr(name,find->className))) {
        if(!find->highest){find->result=window;return FALSE;}
        GetWindowRect(window,&rect);if(rect.top<find->top){find->top=rect.top;find->result=window;}
    }
    return TRUE;
}
static HWND topChoice(HWND window)
{
    Find find;ZeroMemory(&find,sizeof(find));find.className="ComboBox";find.highest=1;find.top=MAXLONG;
    EnumChildWindows(window,findChild,(LPARAM)&find);
    if(!find.result){find.className="Choice";EnumChildWindows(window,findChild,(LPARAM)&find);}return find.result;
}
static HWND control(HWND window,const char *label,const char *className)
{
    Find find;ZeroMemory(&find,sizeof(find));find.process=ownedProcess;find.label=label;find.className=className;
    EnumChildWindows(window,findChild,(LPARAM)&find);return find.result;
}
static int bitmapFile(const char *path,const DWORD *bits,int width,int height)
{
    BITMAPFILEHEADER fileHeader;BITMAPINFOHEADER header;FILE *file=fopen(path,"wb");int result;
    if(!file)return 0;ZeroMemory(&fileHeader,sizeof(fileHeader));ZeroMemory(&header,sizeof(header));
    fileHeader.bfType=0x4d42;fileHeader.bfOffBits=sizeof(fileHeader)+sizeof(header);
    fileHeader.bfSize=fileHeader.bfOffBits+width*height*4;
    header.biSize=sizeof(header);header.biWidth=width;header.biHeight=-height;
    header.biPlanes=1;header.biBitCount=32;header.biSizeImage=width*height*4;
    result=fwrite(&fileHeader,sizeof(fileHeader),1,file)==1 && fwrite(&header,sizeof(header),1,file)==1 &&
        fwrite(bits,4,width*height,file)==(size_t)(width*height);
    fclose(file);return result;
}
static int capture(HWND window,HWND root,const char *path,int *width,int *height,int *colors,unsigned long *hash)
{
    HDC source,destination;HBITMAP bitmap;HGDIOBJ old;BITMAPINFO info;DWORD *bits;
    RECT bounds,clip;POINT origin={0,0},rootOrigin={0,0};int x,y,index,result;
    BYTE used[4096];unsigned long value=2166136261UL;
    if(!owned(window) || !owned(root))return 0;
    GetClientRect(window,&bounds);ClientToScreen(window,&origin);OffsetRect(&bounds,origin.x,origin.y);
    GetClientRect(root,&clip);ClientToScreen(root,&rootOrigin);OffsetRect(&clip,rootOrigin.x,rootOrigin.y);
    IntersectRect(&bounds,&bounds,&clip);
    SetRect(&clip,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));IntersectRect(&bounds,&bounds,&clip);
    *width=bounds.right-bounds.left;*height=bounds.bottom-bounds.top;
    if(*width<1 || *height<1 || *width>4096 || *height>4096)return 0;
    ZeroMemory(&info,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=*width;info.bmiHeader.biHeight=-*height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    source=GetDC(window);destination=CreateCompatibleDC(source);
    bitmap=CreateDIBSection(source,&info,DIB_RGB_COLORS,(void **)&bits,NULL,0);
    if(!source || !destination || !bitmap){if(bitmap)DeleteObject(bitmap);if(destination)DeleteDC(destination);if(source)ReleaseDC(window,source);return 0;}
    old=SelectObject(destination,bitmap);
    result=BitBlt(destination,0,0,*width,*height,source,bounds.left-origin.x,bounds.top-origin.y,SRCCOPY);
    GdiFlush();ZeroMemory(used,sizeof(used));*colors=0;
    for(y=0;y<*height;y++)for(x=0;x<*width;x++) {
        DWORD pixel=bits[y*(*width)+x]&0xffffff;
        value=(value^pixel)*16777619UL;
        index=((pixel>>12)&0xf00)|((pixel>>8)&0xf0)|((pixel>>4)&0xf);
        if(!used[index]){used[index]=1;(*colors)++;}
    }
    *hash=value;
    if(result)result=bitmapFile(path,bits,*width,*height);
    SelectObject(destination,old);DeleteObject(bitmap);DeleteDC(destination);ReleaseDC(window,source);
    return result;
}
static int snapshots(Windows *windows,const char *directory,const char *stage,int repaint)
{
    int i,valid=1;char path[MAX_PATH];
    for(i=0;i<windows->count;i++) {
        Sample *sample=&windows->items[i];
        sample->responsive=responsive(sample->window);if(!sample->responsive){valid=0;continue;}
        if(repaint) {
            SetWindowPos(sample->window,HWND_TOP,30,30,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
            SetForegroundWindow(sample->window);
            /* Preserve the settled client pixels. Forcing a fresh Java paint
             * here would capture a thousands-line model halfway through. */
            Sleep(100);
        }
        sample->content=contentWindow(sample->window);
        _snprintf(path,sizeof(path),"%s/%s-%02d.bmp",directory,stage,i);
        if(!capture(sample->window,sample->window,path,&sample->width,&sample->height,&sample->colors,&sample->hash))valid=0;
        _snprintf(path,sizeof(path),"%s/%s-%02d-content.bmp",directory,stage,i);
        if(!capture(sample->content,sample->window,path,&sample->contentWidth,&sample->contentHeight,&sample->contentColors,&sample->contentHash))valid=0;
        if(sample->contentColors<2)valid=0;
    }
    return valid && windows->count>0;
}
static int post(HWND window,UINT message,WPARAM w,LPARAM l)
{
    return owned(window) && PostMessageA(window,message,w,l)!=0;
}
static HWND hitWindow(HWND window,int *x,int *y)
{
    POINT point;HWND child;
    point.x=*x;point.y=*y;
    while(owned(window)) {
        child=ChildWindowFromPointEx(window,point,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
        if(!child || child==window || !owned(child))break;
        MapWindowPoints(window,child,&point,1);window=child;
    }
    *x=point.x;*y=point.y;return window;
}
static int mouseAt(HWND window,UINT message,WPARAM flags,int x,int y)
{
    window=hitWindow(window,&x,&y);return post(window,message,flags,MAKELPARAM(x,y));
}
static int hoverAt(HWND window,int x,int y)
{
    POINT point;HWND at;RECT rect;char targetClass[128],actualClass[128];DWORD actualProcess=0;
    window=hitWindow(window,&x,&y);point.x=x;point.y=y;ClientToScreen(window,&point);
    at=WindowFromPoint(point);
    GetWindowThreadProcessId(at,&actualProcess);GetWindowRect(window,&rect);
    GetClassNameA(window,targetClass,sizeof(targetClass));GetClassNameA(at,actualClass,sizeof(actualClass));
    if(inputTrace){fprintf(inputTrace,"hover target=%p/%s rect=%ld,%ld,%ld,%ld point=%ld,%ld actual=%p/%s pid=%lu expected=%lu ancestor=%d descendant=%d\n",
        window,targetClass,rect.left,rect.top,rect.right,rect.bottom,point.x,point.y,at,actualClass,actualProcess,ownedProcess,IsChild(at,window),IsChild(window,at));fflush(inputTrace);}
    if(!owned(at) || (at!=window && !IsChild(window,at)))return 0;
    /* TrackMouseEvent observes the real pointer. A synthetic move with the
     * pointer outside immediately generates a real WM_MOUSELEAVE. */
    if(!SetCursorPos(point.x,point.y))return 0;
    Sleep(60);return post(window,WM_MOUSEMOVE,0,MAKELPARAM(x,y));
}
static int click(HWND window,int x,int y)
{
    LPARAM point;int count;
    window=hitWindow(window,&x,&y);point=MAKELPARAM(x,y);
    count=post(window,WM_MOUSEMOVE,0,point);
    count+=post(window,WM_LBUTTONDOWN,MK_LBUTTON,point);
    count+=post(window,WM_LBUTTONUP,0,point);return count;
}
static int inputOne(HWND window,const char *profile)
{
    HWND target=contentWindow(window);RECT rect;int x,y,i,count=0;const char *text;
    GetClientRect(target,&rect);x=rect.right/3;y=rect.bottom/2;
    if(!strcmp(profile,"dither")) {
        DWORD_PTR selected;
        target=topChoice(window);if(!target)return 0;
        count+=post(target,WM_KEYDOWN,VK_DOWN,1);count+=post(target,WM_KEYUP,VK_DOWN,0xc0000001);Sleep(200);
        if(!SendMessageTimeoutA(target,CB_GETCURSEL,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,500,&selected) || selected!=2)return 0;
        target=control(window,"New Image",NULL);if(!target)return 0;
        GetClientRect(target,&rect);return count+click(target,rect.right/2,rect.bottom/2);
    }
    if(!strncmp(profile,"button:",7) || !strcmp(profile,"i18n-applet")) {
        target=control(window,!strcmp(profile,"i18n-applet")?"Demo":profile+7,NULL);
        if(!target)return 0;GetClientRect(target,&rect);return click(target,rect.right/2,rect.bottom/2);
    }
    if(!strcmp(profile,"choice") || !strcmp(profile,"symbol")) {
        target=control(window,NULL,!strcmp(profile,"symbol")?"Edit":"ComboBox");
        if(!target)target=control(window,NULL,!strcmp(profile,"symbol")?"TextField":"Choice");
        if(!target)return 0;
        if(!strcmp(profile,"symbol")) {
            DWORD_PTR ignored;char value[64];DWORD started;
            GetClientRect(target,&rect);click(target,4,rect.bottom/2);Sleep(150);
            if(!SendMessageTimeoutA(target,EM_SETSEL,0,-1,SMTO_ABORTIFHUNG|SMTO_BLOCK,500,&ignored))return 0;
            text="2200";for(i=0;text[i];i++)count+=post(target,WM_CHAR,text[i],1);
            /* Sun replays character messages through the Java queue before
             * applying them to EDIT. Do not let Return overtake the text. */
            started=GetTickCount();
            do {
                value[0]=0;
                if(!SendMessageTimeoutA(target,WM_GETTEXT,sizeof(value),(LPARAM)value,SMTO_ABORTIFHUNG|SMTO_BLOCK,500,&ignored))return 0;
                if(!lstrcmpA(value,"2200"))break;
                Sleep(50);
            } while(GetTickCount()-started<2000);
            if(lstrcmpA(value,"2200"))return 0;
            count+=post(target,WM_KEYDOWN,VK_RETURN,1);count+=post(target,WM_CHAR,'\r',1);count+=post(target,WM_KEYUP,VK_RETURN,0xc0000001);
        } else {
            count+=post(target,WM_KEYDOWN,VK_DOWN,1);count+=post(target,WM_KEYUP,VK_DOWN,0xc0000001);
        }
        return count;
    }
    if(!strcmp(profile,"resize")) {
        GetWindowRect(window,&rect);
        return owned(window) && SetWindowPos(window,NULL,0,0,rect.right-rect.left+24,rect.bottom-rect.top+16,
            SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
    if(!strcmp(profile,"move") || !strcmp(profile,"imagemap")) {
        if(!strcmp(profile,"imagemap")){x=280;y=190;}
        else {x-=x%3;y-=y%3;} /* MouseTrack repaints only at multiples of3. */
        count+=hoverAt(target,8,8);
        count+=hoverAt(target,x,y);return count;
    }
    if(!strcmp(profile,"drag")) {
        count+=mouseAt(target,WM_LBUTTONDOWN,MK_LBUTTON,x,y);
        for(i=1;i<=8;i++)count+=mouseAt(target,WM_MOUSEMOVE,MK_LBUTTON,x+i*4,y+i*2);
        count+=mouseAt(target,WM_LBUTTONUP,0,x+32,y+16);return count;
    }
    if(!strcmp(profile,"tic")){x=rect.right/6;y=rect.bottom/6;}
    if(!strcmp(profile,"round")){x=rect.right/6;y=45;}
    if(!strcmp(profile,"openlook")){x=rect.right/6;y=18;}
    count=click(target,x,y);
    if(!strcmp(profile,"sheet")) {
        target=hitWindow(target,&x,&y);Sleep(100);
        count+=post(target,WM_KEYDOWN,'1',1);count+=post(target,WM_CHAR,'1',1);count+=post(target,WM_KEYUP,'1',0xc0000001);
    }
    return count;
}
static void jsonString(FILE *file,const char *text)
{
    const unsigned char *p=(const unsigned char *)text;fputc('"',file);
    for(;*p;p++) {
        if(*p=='"' || *p=='\\'){fputc('\\',file);fputc(*p,file);}
        else if(*p<32 || *p>=127)fprintf(file,"\\u%04x",*p);
        else fputc(*p,file);
    }
    fputc('"',file);
}
static void jsonWindows(FILE *file,const Windows *windows)
{
    int i;fputc('[',file);
    for(i=0;i<windows->count;i++) {
        const Sample *s=&windows->items[i];if(i)fputc(',',file);
        fprintf(file,"{\"hwnd\":\"%p\",\"title\":",s->window);jsonString(file,s->title);
        fputs(",\"class\":",file);jsonString(file,s->className);
        fprintf(file,",\"width\":%d,\"height\":%d,\"colors\":%d,\"hash\":\"%08lx\",\"responsive\":%s,\"contentWidth\":%d,\"contentHeight\":%d,\"contentColors\":%d,\"contentHash\":\"%08lx\"}",
            s->width,s->height,s->colors,s->hash,s->responsive?"true":"false",s->contentWidth,s->contentHeight,s->contentColors,s->contentHash);
    }
    fputc(']',file);
}
static void closeWindows(void)
{
    Windows windows;int i;enumerate(&windows);
    for(i=0;i<windows.count;i++)post(windows.items[i].window,WM_CLOSE,0,0);
}
static int run(const char *exe,const char *arguments,const char *work,const char *directory,const char *profile,int expected,int settle)
{
    SECURITY_ATTRIBUTES security={sizeof(SECURITY_ATTRIBUTES),NULL,TRUE};STARTUPINFOA startup;PROCESS_INFORMATION process;
    char command[32768],path[MAX_PATH];HANDLE log,input;FILE *file;DWORD start,ready=0,exitCode=0;int i,inputs=0,changed=0,forced=0,beforeOK=0,afterOK=0;
    Windows before,after,motion1,motion2,late;int movingBefore=-1,pauseStable=-1;
    int topmostRequested=!strcmp(profile,"move") || !strcmp(profile,"imagemap");
    const char *status="startup-timeout";Watchdog guard;HANDLE guardThread=NULL;
    ZeroMemory(&before,sizeof(before));ZeroMemory(&after,sizeof(after));ZeroMemory(&motion1,sizeof(motion1));ZeroMemory(&motion2,sizeof(motion2));ZeroMemory(&late,sizeof(late));ZeroMemory(&startup,sizeof(startup));ZeroMemory(&process,sizeof(process));
    _snprintf(path,sizeof(path),"%s/process.log",directory);
    log=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    input=CreateFileA("NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,NULL);
    startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdOutput=startup.hStdError=log;startup.hStdInput=input;
    _snprintf(command,sizeof(command),"\"%s\" %s",exe,arguments);
    if(log==INVALID_HANDLE_VALUE || input==INVALID_HANDLE_VALUE || !CreateProcessA(exe,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,work,&startup,&process)) {
        status="launch-failed";goto result;
    }
    _snprintf(path,sizeof(path),"%s/input-trace.log",directory);inputTrace=fopen(path,"wb");
    CloseHandle(process.hThread);ownedProcess=process.dwProcessId;start=GetTickCount();
    ZeroMemory(&guard,sizeof(guard));guard.process=process.hProcess;guard.pid=process.dwProcessId;
    lstrcpynA(guard.directory,directory,sizeof(guard.directory));guard.done=CreateEvent(NULL,TRUE,FALSE,NULL);
    if(guard.done)guardThread=CreateThread(NULL,0,watchdog,&guard,0,NULL);
    if(!guardThread){TerminateProcess(process.hProcess,137);status="watchdog-init-failed";goto cleanup;}
    while(GetTickCount()-start<15000+(DWORD)settle) {
        if(WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0){status="early-exit";goto cleanup;}
        enumerate(&before);
        for(i=0;i<before.count;i++)if(!lstrcmpiA(before.items[i].title,"Copyright Notice")) {
            snapshots(&before,directory,"before",1);status="unexpected-startup-notice";goto cleanup;
        }
        if(before.count>=expected) {
            if(!ready) {
                /* Physical hover requires the intended owned child to be
                 * exposed. Never send input to an obstructing foreign HWND. */
                if(topmostRequested)for(i=0;i<before.count;i++)if(responsive(before.items[i].window)) {
                    int raised=SetWindowPos(before.items[i].window,HWND_TOPMOST,0,0,0,0,
                        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
                    if(inputTrace){fprintf(inputTrace,"owned-topmost window=%p requested=1 succeeded=%d\n",before.items[i].window,raised);fflush(inputTrace);}
                }
                ready=GetTickCount();
            }
            if(GetTickCount()-ready>=(DWORD)settle)break;
        } else ready=0;
        Sleep(50);
    }
    if(before.count<expected)goto cleanup;
    beforeOK=snapshots(&before,directory,"before",1);
    if(!beforeOK){status="unrendered-or-unresponsive";goto cleanup;}
    if(!strcmp(profile,"pause")) {
        Sleep(250);enumerate(&motion1);alignWindows(&motion1,&before);movingBefore=0;
        if(!snapshots(&motion1,directory,"motion1",0)){status="pre-input-unrendered";goto cleanup;}
        for(i=0;i<before.count && i<motion1.count;i++)if(before.items[i].contentHash!=motion1.items[i].contentHash)movingBefore=1;
        if(!movingBefore) {
            Sleep(250);enumerate(&motion2);alignWindows(&motion2,&before);
            if(!snapshots(&motion2,directory,"motion2",0)){status="pre-input-unrendered";goto cleanup;}
            for(i=0;i<before.count && i<motion2.count;i++)if(before.items[i].contentHash!=motion2.items[i].contentHash)movingBefore=1;
        }
        if(!movingBefore){status="pre-input-animation-unobserved";goto cleanup;}
    }
    for(i=0;i<before.count;i++) {
        if(owned(before.items[i].window)) {
            SetWindowPos(before.items[i].window,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            SetForegroundWindow(before.items[i].window);Sleep(80);
            inputs+=inputOne(before.items[i].window,profile);
        }
    }
    Sleep(settle>900?settle:900);enumerate(&after);alignWindows(&after,&before);
    if(!strcmp(profile,"i18n-applet")) {
        for(i=0;i<after.count;i++)if(control(after.items[i].window,NULL,"ComboBox"))inputs+=inputOne(after.items[i].window,"choice");
        Sleep(400);enumerate(&after);alignWindows(&after,&before);
    }
    afterOK=after.count>=expected && snapshots(&after,directory,"after",1);
    if(!strcmp(profile,"i18n-applet") && after.count<=before.count)afterOK=0;
    if(afterOK && !strcmp(profile,"pause")) {
        Sleep(350);enumerate(&late);alignWindows(&late,&after);pauseStable=late.count==after.count && snapshots(&late,directory,"late",0);
        for(i=0;i<after.count && i<late.count;i++)if(after.items[i].contentHash!=late.items[i].contentHash)pauseStable=0;
    }
    changed=before.count!=after.count;
    for(i=0;i<before.count && i<after.count;i++)if(before.items[i].hash!=after.items[i].hash ||
        before.items[i].width!=after.items[i].width || before.items[i].height!=after.items[i].height)changed=1;
    status=!inputs?"input-target-missing":!afterOK?"post-input-unrendered-or-unresponsive":"captured";
    if(pauseStable==0)status="pause-not-observed";
cleanup:
    closeWindows();
    if(WaitForSingleObject(process.hProcess,2500)!=WAIT_OBJECT_0) {
        forced=1;TerminateProcess(process.hProcess,137);WaitForSingleObject(process.hProcess,2000);
        if(!strcmp(status,"captured"))status="cleanup-timeout";
    }
    GetExitCodeProcess(process.hProcess,&exitCode);
    if(exitCode && !forced && !strcmp(status,"captured"))status="child-exit-failure";
    if(guardThread){SetEvent(guard.done);WaitForSingleObject(guardThread,1000);CloseHandle(guardThread);}
    if(guard.done)CloseHandle(guard.done);CloseHandle(process.hProcess);
result:
    if(inputTrace){fclose(inputTrace);inputTrace=NULL;}
    if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);
    _snprintf(path,sizeof(path),"%s/result.json",directory);file=fopen(path,"wb");
    if(file) {
        fputs("{\"status\":",file);jsonString(file,status);
        fprintf(file,",\"pid\":%lu,\"processExit\":%lu,\"forcedTermination\":%s,\"expectedWindows\":%d,\"inputMessages\":%d,\"observedChange\":%s,\"topmostRequested\":%s,\"before\":",
            process.dwProcessId,exitCode,forced?"true":"false",expected,inputs,changed?"true":"false",topmostRequested?"true":"false");
        jsonWindows(file,&before);fputs(",\"after\":",file);jsonWindows(file,&after);
        fprintf(file,",\"movingBefore\":%s,\"pauseStable\":%s",movingBefore<0?"null":movingBefore?"true":"false",pauseStable<0?"null":pauseStable?"true":"false");
        fputs(",\"motion1\":",file);jsonWindows(file,&motion1);fputs(",\"motion2\":",file);jsonWindows(file,&motion2);fputs(",\"late\":",file);jsonWindows(file,&late);
        fputs("}\n",file);fclose(file);
    }
    return strcmp(status,"captured")!=0;
}
static DWORD *readBitmap(const char *path,int *width,int *height)
{
    FILE *file=fopen(path,"rb");BITMAPFILEHEADER header;BITMAPINFOHEADER info;DWORD *pixels=NULL;long size;
    if(!file)return NULL;
    if(fread(&header,sizeof(header),1,file)!=1 || fread(&info,sizeof(info),1,file)!=1 ||
        header.bfType!=0x4d42 || info.biBitCount!=32 || info.biCompression!=BI_RGB || info.biWidth<1 || info.biWidth>4096 || info.biHeight>=0 || info.biHeight < -4096)goto done;
    *width=info.biWidth;*height=-info.biHeight;size=(*width)*(*height);
    pixels=(DWORD *)malloc(size*4);if(!pixels)goto done;
    fseek(file,header.bfOffBits,SEEK_SET);
    if(fread(pixels,4,size,file)!=(size_t)size){free(pixels);pixels=NULL;}
done:fclose(file);return pixels;
}
static int compareImages(const char *first,const char *second)
{
    int w1=0,h1=0,w2=0,h2=0,x,y,different=0,width,height;DWORD *a=readBitmap(first,&w1,&h1),*b=readBitmap(second,&w2,&h2);
    if(!a || !b){free(a);free(b);return 1;}
    width=w1<w2?w1:w2;height=h1<h2?h1:h2;
    for(y=0;y<height;y++)for(x=0;x<width;x++)if((a[y*w1+x]&0xffffff)!=(b[y*w2+x]&0xffffff))different++;
    printf("{\"sunWidth\":%d,\"sunHeight\":%d,\"oursWidth\":%d,\"oursHeight\":%d,\"overlapPixels\":%d,\"differentPixels\":%d,\"differentPercent\":%.4f,\"sameSize\":%s}\n",
        w1,h1,w2,h2,width*height,different,100.0*different/(width*height),w1==w2 && h1==h2?"true":"false");
    free(a);free(b);return 0;
}
static int fixtureMidHang,fixtureHangArmed,fixtureAnimate,fixtureAnimation,fixtureFrame;
static LRESULT CALLBACK fixtureProc(HWND window,UINT message,WPARAM w,LPARAM l)
{
    static int changed;
    if(message==WM_LBUTTONUP){if(fixtureMidHang)fixtureHangArmed=1;if(fixtureAnimate){KillTimer(window,1);fixtureAnimate=0;}else changed=!changed;InvalidateRect(window,NULL,TRUE);return 0;}
    if(message==WM_TIMER){fixtureFrame++;InvalidateRect(window,NULL,TRUE);return 0;}
    if(message==WM_WINDOWPOSCHANGING && fixtureHangArmed){Sleep(40000);return 0;}
    if(message==WM_PAINT){PAINTSTRUCT paint;RECT rect;HBRUSH brush;
        HDC dc=BeginPaint(window,&paint);GetClientRect(window,&rect);brush=CreateSolidBrush(fixtureAnimation?RGB((fixtureFrame*17)&255,(fixtureFrame*31)&255,(fixtureFrame*7)&255):changed?RGB(0,0,255):RGB(255,0,0));
        FillRect(dc,&rect,brush);DeleteObject(brush);TextOutA(dc,12,12,"Rendered fixture",16);EndPaint(window,&paint);return 0;}
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcA(window,message,w,l);
}
static int fixture(const char *mode)
{
    WNDCLASSA wc;HWND window,outer;MSG message;
    if(!strcmp(mode,"error")){fputs("java.lang.Error: expected fixture startup rejection\n",stderr);return 1;}
    if(!strcmp(mode,"nowindow")){Sleep(20000);return 0;}
    ZeroMemory(&wc,sizeof(wc));wc.lpfnWndProc=!strcmp(mode,"blank")?DefWindowProcA:fixtureProc;
    wc.hInstance=GetModuleHandle(NULL);wc.lpszClassName="AwtDemoFixture";wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassA(&wc);
    window=CreateWindowA(wc.lpszClassName,!strcmp(mode,"notice")?"Copyright Notice":"AwtDemoFixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,50,50,240,160,NULL,NULL,wc.hInstance,NULL);
    fixtureMidHang=!strcmp(mode,"midhang");
    fixtureAnimation=fixtureAnimate=!strcmp(mode,"animation");if(fixtureAnimate)SetTimer(window,1,50,NULL);
    if(!strcmp(mode,"nested")) {
        wc.lpszClassName="AwtDemoOuterPanel";wc.lpfnWndProc=DefWindowProcA;RegisterClassA(&wc);
        outer=CreateWindowA(wc.lpszClassName,"",WS_CHILD|WS_VISIBLE,0,0,200,100,window,NULL,wc.hInstance,NULL);
        wc.lpszClassName="AwtDemoInnerPanel";wc.lpfnWndProc=fixtureProc;RegisterClassA(&wc);
        CreateWindowA(wc.lpszClassName,"",WS_CHILD|WS_VISIBLE,0,0,200,100,outer,NULL,wc.hInstance,NULL);
    }
    if(!strcmp(mode,"hung")){Sleep(20000);return 0;}
    while(GetMessage(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessage(&message);}return window?0:1;
}
static int selftest(const char *directory,const char *selected)
{
    char exe[MAX_PATH],path[MAX_PATH],argumentFile[MAX_PATH],command[4096];const char *modes[]={"render","blank","hung","nowindow","error","midhang","notice","nested","animation"};
    HWND sentinel;int i,result,failed=0,tested=0,positive;FILE *file;STARTUPINFOA startup;PROCESS_INFORMATION process;
    HANDLE job;JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;DWORD exitCode;
    CreateDirectoryA(directory,NULL);GetModuleFileNameA(NULL,exe,sizeof(exe));
    sentinel=CreateWindowA("STATIC","Unrelated sentinel",WS_POPUP,0,0,40,40,NULL,NULL,GetModuleHandle(NULL),NULL);
    for(i=0;i<9;i++) {
        if(selected && strcmp(selected,modes[i]))continue;
        tested++;
        _snprintf(path,sizeof(path),"%s/%s",directory,modes[i]);CreateDirectoryA(path,NULL);
        _snprintf(argumentFile,sizeof(argumentFile),"%s/arguments.txt",path);file=fopen(argumentFile,"wb");
        if(!file){failed++;continue;}fprintf(file,"--fixture %s",modes[i]);fclose(file);
        _snprintf(command,sizeof(command),"\"%s\" --run \"%s\" \"%s\" \"%s\" \"%s\" %s 1 100",exe,exe,argumentFile,directory,path,!strcmp(modes[i],"animation")?"pause":"click");
        ZeroMemory(&startup,sizeof(startup));startup.cb=sizeof(startup);ZeroMemory(&process,sizeof(process));
        job=CreateJobObject(NULL,NULL);ZeroMemory(&limits,sizeof(limits));limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!job || !SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)) ||
            !CreateProcessA(exe,command,NULL,NULL,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,NULL,directory,&startup,&process)) {
            if(job)CloseHandle(job);failed++;continue;
        }
        if(!AssignProcessToJobObject(job,process.hProcess)){TerminateProcess(process.hProcess,137);failed++;}
        ResumeThread(process.hThread);CloseHandle(process.hThread);
        if(WaitForSingleObject(process.hProcess,35000)!=WAIT_OBJECT_0){CloseHandle(job);job=NULL;failed++;}
        GetExitCodeProcess(process.hProcess,&exitCode);CloseHandle(process.hProcess);if(job)CloseHandle(job);
        result=exitCode!=0;
        positive=!strcmp(modes[i],"render") || !strcmp(modes[i],"nested") || !strcmp(modes[i],"animation");
        if((positive && result) || (!positive && !result) || !IsWindow(sentinel))failed++;
        if(!strcmp(modes[i],"nested")) {
            DWORD *pixels;int width=0,height=0;
            _snprintf(argumentFile,sizeof(argumentFile),"%s/after-00-content.bmp",path);
            pixels=readBitmap(argumentFile,&width,&height);
            if(!pixels || width<5 || height<5 || (pixels[4*width+4]&0xffffff)!=0x0000ff)failed++;
            free(pixels);
        }
        if(!strcmp(modes[i],"animation")) {
            const char *stages[]={"before","motion1","after","late"};int j,k,wa,ha,wb,hb,different;DWORD *a,*b;
            for(j=0;j<2;j++) {
                _snprintf(argumentFile,sizeof(argumentFile),"%s/%s-00-content.bmp",path,stages[j*2]);a=readBitmap(argumentFile,&wa,&ha);
                _snprintf(argumentFile,sizeof(argumentFile),"%s/%s-00-content.bmp",path,stages[j*2+1]);b=readBitmap(argumentFile,&wb,&hb);
                different=0;
                if(!a || !b || wa!=wb || ha!=hb)failed++;
                else {for(k=0;k<wa*ha;k++)if((a[k]&0xffffff)!=(b[k]&0xffffff))different++;
                    if((j==0 && !different) || (j==1 && different))failed++;}
                free(a);free(b);
            }
        }
    }
    if(!tested)failed++;
    DestroyWindow(sentinel);printf("native demo capture, blank/hang/exit rejection and process ownership (%d cases): %s\n",tested,failed?"FAILED":"passed");return failed!=0;
}
int main(int argc,char **argv)
{
    FILE *file;char arguments[30000];size_t count;
    if(argc==4 && !strcmp(argv[1],"--compare"))return compareImages(argv[2],argv[3]);
    if(argc==3 && !strcmp(argv[1],"--fixture"))return fixture(argv[2]);
    if((argc==3 || argc==4) && !strcmp(argv[1],"--selftest"))return selftest(argv[2],argc==4?argv[3]:NULL);
    if(argc!=9 || strcmp(argv[1],"--run")){fputs("usage: awtdemos115 --run exe arguments-file cwd out profile expected-windows settle-ms\n",stderr);return 2;}
    file=fopen(argv[3],"rb");if(!file)return 2;count=fread(arguments,1,sizeof(arguments)-1,file);arguments[count]=0;fclose(file);
    return run(argv[2],arguments,argv[4],argv[5],argv[6],atoi(argv[7]),atoi(argv[8]));
}
