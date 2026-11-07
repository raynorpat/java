/* Exercise recovered native entry points without starting a debug server. */
#include <stdio.h>
#include <string.h>
#include "StubPreamble.h"
#include "java_lang_Thread.h"
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
typedef stack_item *(*stub_fn)(stack_item *, struct execenv *);
#define DECLARE(name) extern stack_item *Java_sun_tools_debug_Agent_##name##_stub(stack_item *, struct execenv *)
DECLARE(systemThread); DECLARE(addSystemThread); DECLARE(removeSystemThread);
DECLARE(objectId); DECLARE(getSingleStep);
static int errors;
void SignalError(struct execenv *ee, char *name, char *message)
{ (void)ee; (void)name; (void)message; ++errors; }

int main(int argc, char **argv)
{
    const char *names[] = { "systemThread", "addSystemThread", "removeSystemThread", "objectId", "getSingleStep" };
    stub_fn calls[] = { Java_sun_tools_debug_Agent_systemThread_stub,
        Java_sun_tools_debug_Agent_addSystemThread_stub,
        Java_sun_tools_debug_Agent_removeSystemThread_stub,
        Java_sun_tools_debug_Agent_objectId_stub,
        Java_sun_tools_debug_Agent_getSingleStep_stub };
    Classjava_lang_Thread threads[65];
    Hjava_lang_Thread handles[65];
    long sun_threads[65][12];
    stack_item args[3];
    int i, round, index, sun = argc == 2;
#ifdef _WIN32
    HMODULE dll = NULL;
    char path[MAX_PATH], symbol[128];
    if (sun) {
        CHECK(GetFullPathNameA(argv[1], sizeof(path), path, NULL) < sizeof(path));
        dll = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        CHECK(dll);
        for (i = 0; i < 5; ++i) {
            sprintf(symbol, "Java_sun_tools_debug_Agent_%s_stub", names[i]);
            calls[i] = (stub_fn)GetProcAddress(dll, symbol);
            CHECK(calls[i]);
        }
    }
#else
    CHECK(!sun);
    (void)argv; (void)names;
#endif
    memset(threads, 0, sizeof(threads));
    memset(sun_threads, 0, sizeof(sun_threads));
    memset(handles, 0, sizeof(handles));
    for (i = 0; i < 65; ++i)
        handles[i].obj = sun ? (Classjava_lang_Thread *)sun_threads[i] : &threads[i];
    /* Sun's daemon field is at 28; our generated Thread layout uses 24. */
    threads[64].daemon = 1; sun_threads[64][7] = 1;
    for (round = 0; round < 3; ++round) {
        for (i = 0; i < 65; ++i) {
            args[0].p = &handles[i]; args[1].i = 0x12345678;
            CHECK(calls[0](args, NULL) == args + 1);
            CHECK(args[0].i == (i == 64) && args[1].i == 0x12345678);
        }
        for (i = 0; i < 64; ++i) {
            args[0].p = &handles[i];
            CHECK(calls[1](args, NULL) == args);
            CHECK(calls[1](args, NULL) == args); /* duplicate is not another node */
        }
        /* Remove head, tail, and interior nodes, with an absent removal twice. */
        for (i = 0; i < 64; ++i) {
            index = (i == 0) ? 63 : i - 1;
            args[0].p = &handles[index];
            CHECK(calls[0](args, NULL) == args + 1 && args[0].i == 1);
            args[0].p = &handles[index];
            CHECK(calls[2](args, NULL) == args);
            CHECK(calls[2](args, NULL) == args);
            CHECK(calls[0](args, NULL) == args + 1 && args[0].i == 0);
        }
    }
    /* An explicitly registered daemon remains excluded after removal. */
    args[0].p = &handles[64]; CHECK(calls[1](args, NULL) == args);
    CHECK(calls[2](args, NULL) == args);
    CHECK(calls[0](args, NULL) == args + 1 && args[0].i == 1);
    for (i = 0; i < 3; ++i) {
        threads[0].single_step = sun_threads[0][5] = i ? (i == 1 ? 1 : -1) : 0;
        args[0].p = NULL; args[1].p = &handles[0]; args[2].i = 0x12345678;
        CHECK(calls[4](args, NULL) == args + 1 && args[0].i == (i != 0));
        CHECK(args[1].p == &handles[0] && args[2].i == 0x12345678);
    }
    for (i = 0; i < 66; ++i) {
        args[0].p = NULL; args[1].p = i == 65 ? NULL : &handles[i];
        CHECK(calls[3](args, NULL) == args + 1);
        CHECK(args[0].p == args[1].p); /* identity is the handle, not its body */
    }
    CHECK(errors == 0);
#ifdef _WIN32
    if (dll) FreeLibrary(dll);
#endif
    puts("agent registry, daemon exclusion, step queries and handle identity passed");
    return 0;
}
