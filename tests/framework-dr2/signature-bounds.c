/* Host sanitizer and DR2 test for long argument lists and truncated inputs. */
#include "signature.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char signature[260], expected[520];
    char *converted;
    int i;
    signature[0] = '(';
    for (i = 1; i <= 128; ++i) signature[i] = 'I';
    strcpy(signature + 129, ")J");
    strcpy(expected, "q0@0:0");
    for (i = 0; i < 128; ++i) strcat(expected, "i0");
    converted = NSJavaSignatureToObjc(signature);
    if (!converted || strcmp(converted, expected)) return 1;
    free(converted);
    converted = NSJavaSignatureToObjc("(L");
    if (!converted || strcmp(converted, "v0@0:0@0")) return 1;
    free(converted);
    converted = NSJavaSignatureToObjc("([L");
    if (!converted || strcmp(converted, "v0@0:0@0")) return 1;
    free(converted);
    puts("PASS signature bounds");
    return 0;
}
