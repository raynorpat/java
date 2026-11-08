/* DR2 Foundation alternate-name hook, libObjCJava 0x53d0424c/0x53d04344. */
#import "objects.h"
#include <stdlib.h>
extern Class (*_NSAltClassNameFromString)(NSString *);

static Class findJavaClass(NSString *name)
{
    ClassClass *clazz;
    char *buffer, *p;
    if ([name isEqual:@"String"] || [name isEqual:@"parseString"]) return Nil;
    buffer = malloc([name length] + 1);
    [name getCString:buffer];
    for (p = buffer; *p; ++p) if (*p == '.') *p = '/';
    clazz = FindClass(EE(), buffer, TRUE);
    free(buffer);
    return clazz ? NSJavaClassToObjcClass(clazz, YES) : Nil;
}
void ClassgenInitialization(void)
{
    if (_NSAltClassNameFromString)
        [NSException raise:NSInternalInconsistencyException
            format:@"ObjCJava: somebody else is using out NSClassFromString() hook!"];
    _NSAltClassNameFromString = findJavaClass;
}
