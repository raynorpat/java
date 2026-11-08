/* DR2 ObjCJava JavaTypeToObjc / __NSJavaSignatureToObjc, 0x53d0a3ac–0x53d0a568. */
#include "signature.h"
#include <stdlib.h>
#include <string.h>

static char javaType(const char **signature)
{
    const char *p = *signature;
    char result;
    if (!*p) return 0;
    switch (*p++) {
    case 'B': case 'C': case 'Z': result = 'c'; break;
    case 'D': result = 'd'; break;
    case 'F': result = 'f'; break;
    case 'I': result = 'i'; break;
    case 'J': result = 'q'; break;
    case 'S': result = 's'; break;
    case 'V': result = 'v'; break;
    case 'L':
        result = '@';
        while (*p && *p != ';') ++p;
        if (*p) ++p;
        break;
    case '[':
        result = '@';
        while (*p == '[') ++p;
        if (*p == 'L') {
            while (*p && *p != ';') ++p;
            /* Donor leaves the object-array semicolon for the next iteration. */
        } else if (*p) ++p;
        break;
    default: result = 0; break;
    }
    *signature = p;
    return result;
}

char *NSJavaSignatureToObjc(const char *signature)
{
    const char *p = signature;
    char *result, *out;
    if (*p++ != '(') return NULL;
    /* Each input type can expand to two bytes. The donor's strlen+8 allocation
     * is insufficient for methods with many primitive parameters. */
    result = malloc(2 * strlen(signature) + 8);
    if (!result) return NULL;
    strcpy(result, "v0@0:0");
    out = result + 6;
    while (*p && *p != ')') {
        *out++ = javaType(&p);
        *out++ = '0';
    }
    if (*p) {
        ++p;
        *result = javaType(&p);
    }
    *out = 0;
    return result;
}
