/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* javai_g.dll: setters 1004B8E5, 1004B95E, 1004B9D7; JNI field helpers
 * 1004B80A, 1004B853, 1004B89C. See tests/vm/SYSTEM115-EVIDENCE.md. */

#include <string.h>
#include "tree.h"
#include "interpreter.h"
#include "exceptions.h"

struct Hjava_lang_System;
struct Hjava_io_InputStream;
struct Hjava_io_PrintStream;

static void
setSystemStream(char *name, char *signature, HObject *stream)
{
    ClassClass *cb = FindClass(EE(), JAVAPKG "System", TRUE);
    struct fieldblock *field;
    unsigned int i;

    if (cb == NULL)
        return;
    /* deviation: map Sun's JNI GetStaticFieldID/SetStaticObjectField to the
     * current field storage. Native handles already represent the objects;
     * there is no JNI local-reference conversion in the current runtime. */
    for (i = 0, field = cbFields(cb); i < cbFieldsCount(cb); i++, field++) {
        if ((field->access & ACC_STATIC) && strcmp(fieldname(field), name) == 0
            && strcmp(fieldsig(field), signature) == 0) {
            *(HObject **)normal_static_address(field) = stream;
            return;
        }
    }
    SignalError(0, JAVAPKG "NoSuchFieldError", name);
}

void
java_lang_System_setIn0(struct Hjava_lang_System *unused,
                       struct Hjava_io_InputStream *stream)
{
    setSystemStream("in", "Ljava/io/InputStream;", (HObject *)stream);
}

void
java_lang_System_setOut0(struct Hjava_lang_System *unused,
                        struct Hjava_io_PrintStream *stream)
{
    setSystemStream("out", "Ljava/io/PrintStream;", (HObject *)stream);
}

void
java_lang_System_setErr0(struct Hjava_lang_System *unused,
                        struct Hjava_io_PrintStream *stream)
{
    setSystemStream("err", "Ljava/io/PrintStream;", (HObject *)stream);
}
