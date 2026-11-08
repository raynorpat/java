/* DR2 thread entry and deferred releases, 0x53d05d38 and 0x53d06b08-0x53d06ca4. */
#import "bootstrap.h"
#include <stdlib.h>
NSRecursiveLock *_NSBridgeLock;
cthread_t _NSFirstThread;
int _NSJavaSafeDeallocation = 1;
static id *safeItems;
static unsigned safeCount, safeCapacity;

void GoThreadSafe(void)
{
    [NSThread detachNewThreadSelector:@selector(class) toTarget:[NSObject class] withObject:nil];
}
int _NSJavaStubEnter(void) { return 0; }
void _NSJavaSafeRelease(id object)
{
    if (!_NSJavaSafeDeallocation) { [object release]; return; }
    [_NSBridgeLock lock];
    if (safeCount == safeCapacity) {
        safeCapacity = safeCapacity ? safeCapacity * 2 : 30;
        safeItems = realloc(safeItems, safeCapacity * sizeof(id));
    }
    safeItems[safeCount++] = object;
    [_NSBridgeLock unlock];
}
void _NSJavaSafePurge(void)
{
    if (_NSJavaSafeDeallocation && safeCount && _NSFirstThread == cthread_self()) {
        [_NSBridgeLock lock];
        while (safeCount) [safeItems[--safeCount] release];
        [_NSBridgeLock unlock];
    }
}
void _NSJavaStubExit(int state) { _NSJavaSafePurge(); }
void _NSJavaEnableSafeDeallocation(BOOL enabled)
{
    if (enabled) ++_NSJavaSafeDeallocation;
    else if (--_NSJavaSafeDeallocation == 0) _NSJavaSafePurge();
}
