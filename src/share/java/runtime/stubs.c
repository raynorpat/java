/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* XXX should put this into libjava and into javai.c instead */
#include "_stubs/java_lang_Compiler.c"
#include "_stubs/java_lang_Double.c"
#include "_stubs/java_lang_Throwable.c"
#include "_stubs/java_lang_Float.c"
#include "_stubs/java_lang_Integer.c"
#include "_stubs/java_lang_Long.c"
#include "_stubs/java_lang_Math.c"
#include "_stubs/java_lang_Object.c"
#include "_stubs/java_lang_String.c"
#include "_stubs/java_lang_Thread.c"
#include "_stubs/java_util_Date.c"
#include "_stubs/sun_awt_image_GifImageDecoder.c"

/* Contentious natives */
#include "_stubs/java_lang_Class.c"
#include "_stubs/java_lang_ClassLoader.c"
#include "_stubs/java_lang_Runtime.c"
#include "_stubs/java_lang_SecurityManager.c"
#include "_stubs/java_lang_System.c"
#include "_stubs/jdk115_boot.c"
#include "_stubs/resources115.c"
#include "_stubs/reflect115.c"
#include "_stubs/numeric115.c"
#include "_stubs/resourcebundle115.c"
#include "_stubs/serialization115.c"
#include "_stubs/java_lang_Win32Process.c"
#include "_stubs/core115.c"
#include "_stubs/string115.c"
#include "_stubs/fileextras115.c"
#include "_stubs/vm115.c"
#include "_stubs/java_io_FileOutputStream.c"

#include "_stubs/java_io_File.c"
#include "_stubs/java_io_FileDescriptor.c"
#include "_stubs/java_io_FileInputStream.c"
#include "_stubs/java_io_RandomAccessFile.c"

#ifdef BREAKPTS
#include "_stubs/sun_tools_debug_Agent.c"
#include "_stubs/sun_tools_debug_BreakpointHandler.c"
#endif

void _java_lang_init(void) { }

/* Transitional lookup for retained native adapters that are not public Sun
 * exports. Keep external library resolution separate from this private table. */
unsigned char *builtin_native_lookup(const char *name)
{
    static const struct {
        const char *name;
        stack_item *(*adapter)(stack_item *, struct execenv *);
    } natives[] = {
#include "builtin-natives.inc"
        { NULL, NULL }
    };
    int i;
    for (i = 0; natives[i].name != NULL; ++i)
        if (!strcmp(name, natives[i].name))
            return (unsigned char *)natives[i].adapter;
    return NULL;
}
