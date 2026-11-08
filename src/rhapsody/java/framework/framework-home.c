/* Private DR2 framework adaptation: derive Java home from this VM image,
 * not from the host application's executable or a system installation. */
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *frameworkJavaHome(void)
{
    static char home[PATH_MAX];
    unsigned i, j;
    unsigned long address = (unsigned long)frameworkJavaHome;
    if (home[0]) return home;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const struct mach_header *header = _dyld_get_image_header(i);
        const struct load_command *command = (const struct load_command *)(header + 1);
        long slide = _dyld_get_image_vmaddr_slide(i);
        for (j = 0; j < header->ncmds; ++j) {
            if (command->cmd == LC_SEGMENT) {
                const struct segment_command *segment = (const struct segment_command *)command;
                unsigned long start = segment->vmaddr + slide;
                if (address >= start && address - start < segment->vmsize) {
                    char *slash;
                    if (!realpath(_dyld_get_image_name(i), home)) goto invalid;
                    slash = strrchr(home, '/');
                    if (!slash) goto invalid;
                    *slash = 0;
                    slash = strrchr(home, '/');
                    if (!slash || strcmp(slash + 1, "Libraries")) goto invalid;
                    strcpy(slash, "/Home");
                    return home;
                }
            }
            command = (const struct load_command *)((const char *)command + command->cmdsize);
        }
    }
invalid:
    fprintf(stderr, "JavaVM.framework: cannot locate the private VM home\n");
    exit(1);
    return NULL;
}
