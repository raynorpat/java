/* Solaris libawt.so 30944..30a4c: locked native drawing-surface access. */
#include "awt_Image118.h"
#include "sun_awt_motif_MDrawingSurfaceInfo.h"
#include "java_awt_Component.h"

long sun_awt_motif_MDrawingSurfaceInfo_lock(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{
    Hsun_awt_motif_MComponentPeer *peer;
    AWT_LOCK();
    peer=unhand(self)->peer;
    if (peer && unhand(peer)->target) {
        Hjava_awt_Component *target=unhand(peer)->target;
        if (unhand(self)->w!=unhand(target)->width || unhand(self)->h!=unhand(target)->height) {
            unhand(self)->w=unhand(target)->width;
            unhand(self)->h=unhand(target)->height;
            unhand(self)->state++;
        }
    }
    return unhand(self)->state;
}
void sun_awt_motif_MDrawingSurfaceInfo_unlock(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{ XFlush(awt_display); AWT_UNLOCK(); }
long sun_awt_motif_MDrawingSurfaceInfo_getDisplay(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{ return (long)awt_display; }
long sun_awt_motif_MDrawingSurfaceInfo_getDrawable(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{
    Drawable drawable=None;
    AWT_LOCK();
    if (unhand(self)->peer) {
        AwtPeer *data=awt118_peer(unhand(self)->peer);
        if (data && data->widget && XtIsRealized(data->widget)) drawable=XtWindow(data->widget);
    } else if (unhand(self)->imgrep) {
        AwtImage *image=awt118_image(unhand(self)->imgrep,NULL);
        if (image) drawable=image->pixmap;
    }
    AWT_UNLOCK();
    if (!drawable) SignalError(EE(),"java/lang/NullPointerException","Unable to determine drawable");
    return drawable;
}
long sun_awt_motif_MDrawingSurfaceInfo_getDepth(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{ return awt_visInfo.depth; }
long sun_awt_motif_MDrawingSurfaceInfo_getVisualID(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{ return XVisualIDFromVisual(awt_visInfo.visual); }
long sun_awt_motif_MDrawingSurfaceInfo_getColormapID(Hsun_awt_motif_MDrawingSurfaceInfo *self)
{ return awt_cmap; }
