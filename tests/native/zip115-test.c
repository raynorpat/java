/* Exercise the repository ZIP reader against compressed resource fixtures. */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "zip.h"

int main(int argc, char **argv)
{
    zip_t *zip;
    struct stat st;
    char bytes[64];
    unsigned int i;
    if (argc != 2) return 1;
    zip = zip_open(argv[1]);
    if (zip == NULL) return 2;
    if (!zip_stat(zip, "vm115.properties", &st) || st.st_size >= sizeof(bytes))
        return 3;
    if (!zip_get(zip, "vm115.properties", bytes, st.st_size)) return 4;
    bytes[st.st_size] = 0;
    if (strstr(bytes, "answer=115") == NULL) return 5;
    if (zip_get(zip, "vm115.properties", bytes, st.st_size - 1)) return 6;
    if (zip_stat(zip, "missing", &st)) return 7;
    /* A stored entry cannot claim a smaller compressed size to bypass bounds. */
    for (i = 0; i < zip->nel; i++) {
        if (!strcmp(zip->dir[i].fn, "vm115.properties")) {
            zip->dir[i].method = 0;
            zip->dir[i].size = 1;
            if (zip_get(zip, "vm115.properties", bytes, zip->dir[i].len))
                return 8;
        }
    }
    zip_close(zip);
    puts("zip115-test: passed");
    return 0;
}
