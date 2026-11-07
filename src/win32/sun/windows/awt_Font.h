#ifndef AWT_FONT_H
#define AWT_FONT_H
#include "awt_Component.h"
extern "C" {
#include "sun_awt_FontDescriptor.h"
}
BOOL awtFontInfo(Hjava_awt_Font *, Hsun_awt_FontDescriptor *, LOGFONTW *);
#endif
