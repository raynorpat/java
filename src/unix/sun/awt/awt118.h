/* Solaris 1.1.8 Motif contracts, using the JDK 1.1.5 generated object layout. */
#ifndef AWT118_H
#define AWT118_H
#include "StubPreamble.h"
#include "monitor.h"
#include "sun_awt_motif_MToolkit.h"
#include "sun_awt_motif_MComponentPeer.h"
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <Xm/Xm.h>
extern Display *awt_display;
extern XtAppContext awt_appContext;
extern XVisualInfo awt_visInfo;
extern Colormap awt_cmap;
extern int awt_screen;
extern Hsun_awt_motif_MToolkit *awt_lock;
struct Hjava_awt_Font;
struct Hsun_awt_FontDescriptor;
XFontStruct *awt118_font(struct Hjava_awt_Font *font);
XmFontList awt118_font_list(struct Hjava_awt_Font *font);
XmString awt118_string(Hjava_lang_String *value,HObject *target);
XFontStruct *awt118_segment_font(struct Hjava_awt_Font *font, struct Hsun_awt_FontDescriptor *descriptor);
typedef struct AwtPeer {
    Widget widget, shell, content, container, menuBar;
    Cursor cursor;
    Window iconWindow;
    Pixmap iconPixmap;
    int references;
    int imHeight;
    int menuHeight, warningHeight;
    Widget warning;
    int decorLeft, decorRight, decorTop, decorBottom;
    int modalActive;
    struct AwtPeer *modalPrevious;
    Hsun_awt_motif_MComponentPeer *javaPeer;
    struct AwtPeer *next;
    void (*cleanup)(struct AwtPeer *);
} AwtPeer;
struct Classjava_awt_Insets;
void awt118_initial_extents(AwtPeer *data,struct Classjava_awt_Insets *insets);
void awt118_create_warning(AwtPeer *data);
void awt118_shell_event(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch);
AwtPeer *awt118_peer(Hsun_awt_motif_MComponentPeer *peer);
AwtPeer *awt118_new_peer(Hsun_awt_motif_MComponentPeer *peer);
void awt118_release_peer(AwtPeer *data);
void awt118_dispose_peer(Hsun_awt_motif_MComponentPeer *peer);
void awt118_register_widget(Hsun_awt_motif_MComponentPeer *peer);
void awt118_callback_done(void);
unsigned long awt118_pixel(unsigned long rgb);
void awt118_init_colors(void);
unsigned long awt118_index_pixel(unsigned long rgb);
struct Hjava_awt_image_ColorModel *awt118_index_model(void);
Widget awt118_menu_widget(HObject *peer);
int awt118_modal_allows(Widget widget);
int awt118_modifiers(unsigned int state);
void awt118_end_modal(AwtPeer *data);
void awt118_register_input(AwtPeer *peer);
void awt118_forget_input(AwtPeer *peer);
int awt118_filter_input(XEvent *event);
int awt118_xim_event(XEvent *event);
int awt118_dispatch_putback(void);
void awt118_focus_event(Hsun_awt_motif_MComponentPeer *peer,XEvent *event);
#define AWT_LOCK() monitorEnter((unsigned int)awt_lock)
#define AWT_UNLOCK() monitorExit((unsigned int)awt_lock)
#endif
