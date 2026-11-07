/* Compare reconstructed native ABI directly with Sun's DLL, without a JVM. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "StubPreamble.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
typedef stack_item *(__cdecl *native_stub)(stack_item *, struct execenv *);
extern stack_item *Java_sun_awt_windows_WToolkit_getScreenWidth_stub(stack_item *, struct execenv *);
extern stack_item *Java_sun_awt_windows_WToolkit_getScreenHeight_stub(stack_item *, struct execenv *);
extern stack_item *Java_sun_awt_windows_WToolkit_getScreenResolution_stub(stack_item *, struct execenv *);
extern stack_item *Java_sun_awt_windows_WToolkit_loadSystemColors_stub(stack_item *, struct execenv *);
int main(int argc, char **argv)
{
    const char *names[] = { "getScreenWidth", "getScreenHeight", "getScreenResolution" };
    native_stub ours[] = { Java_sun_awt_windows_WToolkit_getScreenWidth_stub,
        Java_sun_awt_windows_WToolkit_getScreenHeight_stub,
        Java_sun_awt_windows_WToolkit_getScreenResolution_stub };
    HMODULE dll;
    native_stub sun;
    stack_item a[2], b[2];
    long colors_a[28], colors_b[28];
    HArrayOfInt ha, hb;
    char symbol[128];
    char library[MAX_PATH];
    int i, length;
    CHECK(argc == 2);
    CHECK(GetFullPathNameA(argv[1], sizeof(library), library, NULL) < sizeof(library));
    dll = LoadLibraryExA(library, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!dll) fprintf(stderr, "LoadLibrary %s: %lu\n", argv[1], GetLastError());
    CHECK(dll != NULL);
    for (i = 0; i < 3; i++) {
        sprintf(symbol, "Java_sun_awt_windows_WToolkit_%s_stub", names[i]);
        sun = (native_stub)GetProcAddress(dll, symbol); CHECK(sun);
        memset(a, 0, sizeof(a)); memset(b, 0, sizeof(b));
        a[1].i = b[1].i = 0x12345678;
        CHECK(sun(a, NULL) == a + 1 && ours[i](b, NULL) == b + 1);
        CHECK(a[0].i == b[0].i && b[0].i > 0 && b[1].i == 0x12345678);
    }
    sun = (native_stub)GetProcAddress(dll, "Java_sun_awt_windows_WToolkit_loadSystemColors_stub");
    CHECK(sun);
    for (length = 0; length <= 26; length++) {
        for (i = 0; i < 28; i++) colors_a[i] = colors_b[i] = 0x12345678;
        ha.obj = (ClassArrayOfInt *)colors_a; hb.obj = (ClassArrayOfInt *)colors_b;
        ha.methods = hb.methods = (struct methodtable *)((length << 5) | 10);
        a[0].p = b[0].p = NULL; a[1].p = &ha; b[1].p = &hb;
        CHECK(sun(a, NULL) == a);
        CHECK(Java_sun_awt_windows_WToolkit_loadSystemColors_stub(b, NULL) == b);
        CHECK(!memcmp(colors_a, colors_b, sizeof(colors_a)));
        for (i = length; i < 28; i++) CHECK(colors_b[i] == 0x12345678);
    }
    /* Sun uses sizeof(table) as an element count. Do not repeat its overread. */
    hb.methods = (struct methodtable *)((28 << 5) | 10);
    CHECK(Java_sun_awt_windows_WToolkit_loadSystemColors_stub(b, NULL) == b);
    CHECK(colors_b[26] == 0x12345678 && colors_b[27] == 0x12345678);
    FreeLibrary(dll);
    puts("winawt toolkit queries match Sun; bounded system colors passed");
    return 0;
}
