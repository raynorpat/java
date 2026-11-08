/* DR2 libObjCJava complaint state, 0x53d043ac and 0x53d04408. */
#import "diagnostics.h"
#include <stdarg.h>
#include <string.h>

static NSString *complaintBanner;
static unsigned char complaintTypes[4];
static BOOL displayedBanner;

void _BRIDGEResetComplaints(NSString *banner)
{
    [complaintBanner release];
    complaintBanner = [banner copy];
    memset(complaintTypes, 0, sizeof(complaintTypes));
    displayedBanner = NO;
}

void _BRIDGEComplain(int type, NSString *format, ...)
{
    va_list args;
    if (type) {
        if (complaintTypes[type]) return;
        complaintTypes[type] = 1;
    }
    if (!displayedBanner) {
        if (complaintBanner) {
            NSLog(@"ObjCJava: %@", complaintBanner);
            [complaintBanner release];
            complaintBanner = nil;
        } else NSLog(@"ObjCJava: from user code:");
        displayedBanner = YES;
    }
    va_start(args, format);
    NSLogv(format, args);
    va_end(args);
}
