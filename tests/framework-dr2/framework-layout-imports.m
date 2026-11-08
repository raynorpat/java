#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#import <objc/objc-runtime.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Full-image binding also forces DR2's duplicate CRT/System startup stubs.
 * Bind every actual external import instead, including normally lazy symbols. */
static int bindImports(const char *path)
{
    FILE *file = fopen(path, "rb");
    struct mach_header header;
    struct symtab_command table;
    unsigned i;
    char *strings;
    int found = 0;
    if (!file || fread(&header, sizeof(header), 1, file) != 1) return 0;
    for (i = 0; i < header.ncmds; ++i) {
        struct load_command command;
        long position = ftell(file);
        if (fread(&command, sizeof(command), 1, file) != 1) return 0;
        if (command.cmd == LC_SYMTAB) {
            fseek(file, position, SEEK_SET);
            if (fread(&table, sizeof(table), 1, file) != 1) return 0;
            found = 1;
        }
        fseek(file, position + command.cmdsize, SEEK_SET);
    }
    if (!found) return 0;
    strings = malloc(table.strsize);
    fseek(file, table.stroff, SEEK_SET);
    if (!strings || fread(strings, table.strsize, 1, file) != 1) return 0;
    fseek(file, table.symoff, SEEK_SET);
    for (i = 0; i < table.nsyms; ++i) {
        struct nlist symbol;
        if (fread(&symbol, sizeof(symbol), 1, file) != 1) return 0;
        if ((symbol.n_type & N_EXT) && (symbol.n_type & N_TYPE) == N_UNDF && !symbol.n_value) {
            const char *name = strings + symbol.n_un.n_strx;
            if (!strncmp(name, ".objc_class_name_", 17)) {
                unsigned long address = 0;
                void *module = NULL;
                _dyld_lookup_and_bind_objc(name, &address, &module);
                if (module) continue;
            } else if (NSIsSymbolNameDefined(name) && NSLookupAndBindSymbol(name)) continue;
            {
                fprintf(stderr, "unresolved import in %s: %s\n", path, name);
                return 0;
            }
        }
    }
    free(strings);
    fclose(file);
    return 1;
}
int main(int argc, char **argv)
{
    const char *names[] = {"javai", "net", "zip", "math", "sysresource", "jpeg", "agent", "awt", "mmedia", "ObjCJava"};
    char root[PATH_MAX], path[PATH_MAX];
    unsigned i, j;
    if (argc != 2 || !realpath(argv[1], root)) return 1;
    for (i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        int found = 0;
        sprintf(path, "%s/Libraries/lib%s.dylib", root, names[i]);
        printf("loading %s\n", names[i]);
        fflush(stdout);
        if (!NSAddLibrary(path)) return 2;
        for (j = 0; j < _dyld_image_count(); ++j) {
            char actual[PATH_MAX];
            if (realpath(_dyld_get_image_name(j), actual) && !strcmp(path, actual)) {
                if (!bindImports(path)) return 3;
                ++found;
            }
        }
        if (found != 1) return 4;
        printf("bound %s\n", names[i]);
    }
    puts("PASS private native imports");
    return 0;
}
