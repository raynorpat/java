#ifndef AWT_MENU_H
#define AWT_MENU_H
#include "awt_Component.h"
/* All registry and HMENU access runs on the toolkit owner thread. */
BOOL awtMenuMessage(AwtComponent *,UINT,WPARAM,LPARAM,LRESULT *);
void awtMenuCleanup();
void awtSetMenuBar(Hsun_awt_windows_WComponentPeer *,HObject *);
#endif
