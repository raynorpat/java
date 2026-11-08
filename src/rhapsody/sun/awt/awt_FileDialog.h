#ifndef RHAPSODY_AWT_FILEDIALOG_H
#define RHAPSODY_AWT_FILEDIALOG_H
#import "awt_View.h"

/* DR2 reuses AwtView's tail padding: flags 109/110, strings 112/116/120. */
@interface AwtDummyFileDialogView : AwtView {
    BOOL _isSave;
    BOOL _wantsToFilterFiles;
    NSString *_title;
    NSString *_file;
    NSString *_directory;
}
- (id)initWithPeer:(int)peer isSavePanel:(BOOL)save title:(NSString *)title file:(NSString *)file directory:(NSString *)directory wantsToFilter:(BOOL)filter;
- (void)setFile:(NSString *)file;
- (void)setDirectory:(NSString *)directory;
- (void)setTitle:(NSString *)title;
- (void)setIsSavePanel:(BOOL)save;
- (void)setWantsToFilterFiles:(BOOL)filter;
- (void)showPanel;
- (BOOL)_pathIsDirectoryOrAutomount:(NSString *)path;
- (BOOL)filterAcceptsFilename:(NSString *)path forPanel:(NSSavePanel *)panel;
@end
#endif
