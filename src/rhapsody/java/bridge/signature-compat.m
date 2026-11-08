/* Deprecated exported buffer interface, DR2 libObjCJava 0x53d0a568. */
#import "diagnostics.h"
#include "signature.h"
#include <stdlib.h>
#include <string.h>

char *_NSJavaSignatureToObjc(const char *signature, char *buffer, int size)
{
    char *result;
    size_t required;
    _BRIDGEComplain(2, @"_NSJavaSignatureToObjc() was a private function (hence the '_' in the name!) and is soon to be removed.  There is no replacement for this functionality.  Why are you calling this in the first place?  Go bug Toby.");
    result = NSJavaSignatureToObjc(signature);
    if (!result) return NULL;
    required = strlen(result) + 1;
    /* Preserve the donor's buffer-selection threshold while bounding writes
     * for descriptors whose encoding exceeds its strlen(signature)+8 guess. */
    if (required < strlen(signature) + 8) required = strlen(signature) + 8;
    if (size >= 0 && (size_t)size >= required) {
        memcpy(buffer, result, strlen(result) + 1);
        free(result);
        return buffer;
    }
    return result;
}
