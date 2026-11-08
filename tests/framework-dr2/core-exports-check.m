#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
extern void ObjCJava_AInitialization(void);

static int contains(unsigned image, const void *address)
{
    const struct mach_header *header = _dyld_get_image_header(image);
    const struct load_command *command = (const void *)(header + 1);
    long slide = _dyld_get_image_vmaddr_slide(image);
    unsigned i;
    for (i = 0; i < header->ncmds; ++i) {
        if (command->cmd == LC_SEGMENT) {
            const struct segment_command *segment = (const void *)command;
            unsigned long start = segment->vmaddr + slide;
            if (segment->vmsize && (unsigned long)address >= start &&
                (unsigned long)address - start < segment->vmsize) return 1;
        }
        command = (const void *)((const char *)command + command->cmdsize);
    }
    return 0;
}

int main(int argc, char **argv)
{
    char target[PATH_MAX], core[PATH_MAX], actual[PATH_MAX], line[4096], *names[155];
    unsigned i, image = (unsigned)-1, count = 0, foundCore = 0;
    FILE *manifest;
    if (argc != 4 || !realpath(argv[1], target) || !realpath(argv[3], core)) return 2;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *name = _dyld_get_image_name(i);
        if (strstr(name,"/usr/lib/java/libObjCJava") ||
            strstr(name,"/System/Library/Frameworks/JavaVM.framework")) {
            fprintf(stderr,"donor image loaded: %s\n",name); return 1;
        }
        if (realpath(name, actual)) {
            if (!strcmp(target, actual)) image = i;
            if (!strcmp(core, actual)) foundCore = 1;
        }
    }
    if (image == (unsigned)-1) { fputs("target image not found\n",stderr); return 1; }
    if (!foundCore) { fputs("private VM core image not found\n",stderr); return 1; }
    if (!contains(image,(void *)ObjCJava_AInitialization)) return 1;
    manifest = fopen(argv[2],"r");
    if (!manifest || !fgets(line,sizeof(line),manifest)) return 2;
    while (fgets(line,sizeof(line),manifest)) {
        char *name = strchr(line,'\t'), *end;
        NSSymbol symbol;
        void *address;
        if (!name || count == 155) return 2;
        ++name; end = strchr(name,'\t'); if (!end) return 2; *end = 0;
        for (i = 0; i < count; ++i) if (!strcmp(names[i],name)) {
            fprintf(stderr,"duplicate manifest symbol: %s\n",name); return 1;
        }
        names[count++] = strdup(name);
        if (!NSIsSymbolNameDefined(name)) {
            fprintf(stderr,"missing symbol: %s\n",name); return 1;
        }
        symbol = NSLookupAndBindSymbol(name);
        address = symbol ? NSAddressOfSymbol(symbol) : NULL;
        if (!address || !contains(image,address)) {
            fprintf(stderr,"symbol outside reconstructed image: %s\n",name); return 1;
        }
    }
    fclose(manifest);
    for (i = 0; i < count; ++i) free(names[i]);
    if (count != 155) { fprintf(stderr,"expected 155 exports, found %u\n",count); return 1; }
    puts("PASS 155 core exports bound inside reconstructed ObjCJava image");
    return 0;
}
