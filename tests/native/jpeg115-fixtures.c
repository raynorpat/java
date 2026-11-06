/* Deterministic generated image data; no vendor binary fixtures in git. */
#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"

static int generate(const char *directory, const char *name, int gray, int progressive)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr error;
    FILE *file;
    char path[1024];
    unsigned char row[17 * 3];
    JSAMPROW rows[1];
    int x, y;
    sprintf(path, "%s/%s.jpg", directory, name);
    file = fopen(path, "wb");
    if (!file) return 0;
    cinfo.err = jpeg_std_error(&error);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, file);
    cinfo.image_width = 17; cinfo.image_height = 9;
    cinfo.input_components = gray ? 1 : 3;
    cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    if (progressive) jpeg_simple_progression(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);
    rows[0] = row;
    for (y = 0; y < 9; ++y) {
        for (x = 0; x < 17; ++x) {
            if (gray) row[x] = (unsigned char)(x * 13 + y * 7);
            else {
                row[x * 3] = (unsigned char)(x * 15);
                row[x * 3 + 1] = (unsigned char)(y * 29);
                row[x * 3 + 2] = (unsigned char)((x + y) * 9);
            }
        }
        jpeg_write_scanlines(&cinfo, rows, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return fclose(file) == 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    return !(generate(argv[1], "rgb", 0, 0) &&
             generate(argv[1], "gray", 1, 0) &&
             generate(argv[1], "progressive", 0, 1));
}
