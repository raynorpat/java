/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* javai_g.dll: valueOf0 10032f10, parser 10033062. See numeric evidence. */
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include "oobj.h"
#include "interpreter.h"
#include "javaString.h"
#include "utf.h"

/* reconstructed: local helper names; the parser states follow the binary. */
static int numericSpace(char c)
{
    return c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == ' ';
}

static int numericSuffix(char c)
{
    return c == 'd' || c == 'D' || c == 'f' || c == 'F';
}

static double numericParse(char *text, char **end)
{
    char *p, *decimal = localeconv()->decimal_point;
    char point = decimal ? *decimal : '.';
    int state = 0;

    for (p = text; state != 10 && *p; p++) {
        char c = *p;
        int digit = c >= '0' && c <= '9';
        switch (state) {
        case 0:
            if (numericSpace(c)) break;
            if (c == '+' || c == '-') { state = 1; break; }
            /* fall through */
        case 1:
            if (digit) state = 2;
            else if (c == '.') { *p = point; state = 3; }
            else state = 10;
            break;
        case 2:
        case 3:
            if (digit) break;
            if (state == 2 && c == '.') { *p = point; state = 3; }
            else if (c == 'e' || c == 'E') state = 5;
            else if (numericSuffix(c)) { *p = ' '; state = 8; }
            else if (numericSpace(c)) state = 9;
            else state = 10;
            break;
        case 5:
            if (digit) state = 7;
            else if (c == '+' || c == '-') state = 6;
            else state = 10;
            break;
        case 6:
            state = digit ? 7 : 10;
            break;
        case 7:
            if (digit) break;
            if (numericSuffix(c)) { *p = ' '; state = 8; }
            else if (numericSpace(c)) state = 9;
            else state = 10;
            break;
        case 8:
        case 9:
            state = numericSpace(c) ? 9 : 10;
            break;
        }
    }
    if (state == 2 || state == 3 || state == 7 || state == 8 || state == 9)
        return strtod(text, end);
    *end = text;
    return 0.0;
}

double
java_lang_Double_valueOf0(void *unused, Hjava_lang_String *string)
{
    char local[32], *text, *end;
    unicode *chars;
    int length, size;
    double result;

    if (string == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return 0.0;
    }
    length = unhand(string)->count;
    chars = unhand(unhand(string)->value)->body + unhand(string)->offset;
    size = unicode2utfstrlen(chars, length);
    text = size <= sizeof(local) ? local : (char *)malloc(size);
    if (text == NULL) {
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        return 0.0;
    }
    unicode2utf(chars, length, text, size);
    result = numericParse(text, &end);
    if (end == text) {
        unicode2utf(chars, length, text, size);
        SignalError(0, JAVAPKG "NumberFormatException", text);
        result = 0.0;
    } else if (errno == ERANGE && result == 0.0) {
        result = atof(text);
    }
    if (text != local) free(text);
    return result;
}
