/* Independent X11 client for the Motif clipboard integration fixture. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

int main(int argc,char **argv)
{
    Display *display=XOpenDisplay(NULL);
    Window window;
    Atom clipboard,targets,property,type;
    XEvent event,response;
    unsigned char *value;
    unsigned long length,remaining;
    int format;
    if (!display || argc!=2) return 1;
    alarm(10);
    window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,1,1,0,0,0);
    clipboard=XInternAtom(display,"CLIPBOARD",False); targets=XInternAtom(display,"TARGETS",False);
    property=XInternAtom(display,"TEST_SELECTION",False);
    if (!strcmp(argv[1],"read")) {
        XConvertSelection(display,clipboard,XA_STRING,property,window,CurrentTime); XFlush(display);
        do { XNextEvent(display,&event); } while (event.type!=SelectionNotify);
        if (event.xselection.property==None) return 2;
        XGetWindowProperty(display,window,property,0,1024,True,AnyPropertyType,&type,&format,&length,&remaining,&value);
        if (type!=XA_STRING || format!=8 || remaining) return 3;
        fwrite(value,1,length,stdout); putchar('\n'); XFree(value);
    } else {
        XSetSelectionOwner(display,clipboard,window,CurrentTime); XSync(display,False);
        if (XGetSelectionOwner(display,clipboard)!=window) return 4;
        puts("ready"); fflush(stdout);
        for (;;) {
            fd_set set; int fd=ConnectionNumber(display);
            if (!XPending(display)) {
                FD_ZERO(&set); FD_SET(0,&set); FD_SET(fd,&set);
                if (select(fd+1,&set,NULL,NULL,NULL)<0) return 5;
                if (FD_ISSET(0,&set)) break;
            }
            while (XPending(display)) {
                XNextEvent(display,&event);
                if (event.type!=SelectionRequest) continue;
                memset(&response,0,sizeof(response));
                response.xselection.type=SelectionNotify; response.xselection.display=display;
                response.xselection.requestor=event.xselectionrequest.requestor;
                response.xselection.selection=event.xselectionrequest.selection;
                response.xselection.target=event.xselectionrequest.target;
                response.xselection.time=event.xselectionrequest.time;
                property=event.xselectionrequest.property;
                if (property==None) property=event.xselectionrequest.target;
                if (event.xselectionrequest.target==targets) {
                    Atom supported[2]={targets,XA_STRING};
                    XChangeProperty(display,event.xselectionrequest.requestor,property,XA_ATOM,32,PropModeReplace,(unsigned char *)supported,2);
                    response.xselection.property=property;
                } else if (event.xselectionrequest.target==XA_STRING) {
                    XChangeProperty(display,event.xselectionrequest.requestor,property,XA_STRING,8,PropModeReplace,(unsigned char *)"from X11",8);
                    response.xselection.property=property;
                }
                XSendEvent(display,response.xselection.requestor,False,0,&response); XFlush(display);
            }
        }
    }
    XDestroyWindow(display,window); XCloseDisplay(display); return 0;
}
