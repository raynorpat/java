#ifndef AWT_FONT_H
#define AWT_FONT_H
#include "awt_Component.h"
extern "C" {
#include "sun_awt_FontDescriptor.h"
}
BOOL awtFontInfo(Hjava_awt_Font *, Hsun_awt_FontDescriptor *, LOGFONTW *);
/* Recovered control text helper, 1000DCE3: x/y are top-left (not baseline).
 * Runs on the toolkit owner thread and may invoke Java charset converters. */
SIZE awtFontText(HDC, Hjava_awt_Font *, Hjava_lang_String *, int x, int y, BOOL draw);
#endif
