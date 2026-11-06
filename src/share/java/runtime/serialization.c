/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Primary evidence: javai_g.dll 100515A0 through 1005328F. */
#include <string.h>
#include <stdarg.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "exceptions.h"

extern ClassClass *reflect_find_class(char *, ClassClass *, char **);
extern HObject *reflect_new_array(ClassClass *, int);

static HObject *serial_error(char *name, char *detail)
{ SignalError(0, name, detail); return NULL; }

static struct methodblock *serial_hook(ClassClass *cb, int writer)
{
    int i; struct methodblock *mb;
    char *name = writer ? "writeObject" : "readObject";
    char *signature = writer ? "(Ljava/io/ObjectOutputStream;)V" : "(Ljava/io/ObjectInputStream;)V";
    for (i = cbMethodsCount(cb) - 1; i >= 0; i--) {
        mb = &cbMethods(cb)[i];
        if ((mb->fb.access & ACC_PRIVATE) && !(mb->fb.access & ACC_STATIC)
            && !strcmp(mb->fb.name, name) && !strcmp(mb->fb.signature, signature)) return mb;
    }
    return NULL;
}
long java_io_ObjectStreamClass_hasWriteObject(HObject *unused, ClassClass *type)
{ return serial_hook(type, TRUE) != NULL && !exceptionOccurred(EE()); }
long java_io_ObjectStreamClass_getClassAccess(HObject *unused, ClassClass *type)
{ return cbAccess(type) & 0xfff; }
int64_t java_io_ObjectStreamClass_getSerialVersionUID(HObject *unused, ClassClass *type)
{
    ClassClass *cb = type; int i; struct fieldblock *fb; Java8 tmp;
    for (i = cbFieldsCount(cb) - 1; i >= 0; i--) {
        fb = &cbFields(cb)[i];
        if ((fb->access & (ACC_STATIC | ACC_FINAL)) == (ACC_STATIC | ACC_FINAL)
            && fb->signature[0] == 'J' && !strcmp(fb->name, "serialVersionUID"))
            return GET_INT64(tmp, twoword_static_address(fb));
    }
    return int2ll(0);
}

static HArrayOfObject *serial_signatures(ClassClass *cb, int methods)
{
    int count = methods ? cbMethodsCount(cb) : cbFieldsCount(cb), i;
    HArrayOfObject *array = (HArrayOfObject *)reflect_new_array(classJavaLangString, count);
    struct fieldblock *fb; size_t length; char local[1024], *buffer;
    if (!array) return NULL;
    for (i = count - 1; i >= 0; i--) {
        fb = methods ? &cbMethods(cb)[i].fb : &cbFields(cb)[i];
        length = strlen(fb->name) + strlen(fb->signature) + 2;
        buffer = length <= sizeof(local) ? local : sysMalloc(length);
        if (!buffer) { serial_error(JAVAPKG "OutOfMemoryError", NULL); return NULL; }
        strcpy(buffer, fb->name); strcat(buffer, " "); strcat(buffer, fb->signature);
        unhand(array)->body[i] = (HObject *)makeJavaString(buffer, strlen(buffer));
        if (buffer != local) sysFree(buffer);
        if (exceptionOccurred(EE())) return NULL;
    }
    return array;
}
HArrayOfObject *java_io_ObjectStreamClass_getMethodSignatures(HObject *unused, ClassClass *type)
{ return serial_signatures(type, TRUE); }
HArrayOfObject *java_io_ObjectStreamClass_getFieldSignatures(HObject *unused, ClassClass *type)
{ return serial_signatures(type, FALSE); }
static long serial_access(ClassClass *cb, HString *signature, int methods)
{
    int count = methods ? cbMethodsCount(cb) : cbFieldsCount(cb), i;
    char *buffer, *space; struct fieldblock *fb;
    buffer = allocCString(signature);
    if (!buffer) return 0;
    space = strchr(buffer, ' ');
    if (space) {
        *space++ = 0;
        for (i = count - 1; i >= 0; i--) {
            fb = methods ? &cbMethods(cb)[i].fb : &cbFields(cb)[i];
            if (!strcmp(fb->name, buffer) && !strcmp(fb->signature, space)) {
                sysFree(buffer); return fb->access & 0xfff;
            }
        }
    }
    sysFree(buffer); return 0;
}
long java_io_ObjectStreamClass_getMethodAccess(HObject *unused, ClassClass *type, HString *signature)
{ return serial_access(type, signature, TRUE); }
long java_io_ObjectStreamClass_getFieldAccess(HObject *unused, ClassClass *type, HString *signature)
{ return serial_access(type, signature, FALSE); }

HArrayOfObject *java_io_ObjectStreamClass_getFields0(HObject *self, ClassClass *type)
{
    ClassClass *cb = type, *fieldclass;
    struct fieldblock *fb;
    HArrayOfObject *array; HString *name, *signature;
    int count = 0, i, slot = 0;
    for (i = 0; i < cbFieldsCount(cb); i++)
        if (!(cbFields(cb)[i].access & (ACC_STATIC | ACC_TRANSIENT))) count++;
    fieldclass = FindClassFromClass(EE(), "java/io/ObjectStreamField", TRUE, obj_classblock(self));
    if (!fieldclass) return NULL;
    array = (HArrayOfObject *)reflect_new_array(fieldclass, count);
    if (!array) return NULL;
    for (i = 0; i < cbFieldsCount(cb); i++) {
        fb = &cbFields(cb)[i];
        if (fb->access & (ACC_STATIC | ACC_TRANSIENT)) continue;
        name = makeJavaString(fb->name, strlen(fb->name));
        signature = fieldIsArray(fb) || fieldIsClass(fb)
            ? makeJavaString(fb->signature, strlen(fb->signature)) : NULL;
        /* ObjectStreamField calls this value offset, but Sun passes field index. */
        unhand(array)->body[slot++] = execute_java_constructor(EE(), NULL, fieldclass,
            "(Ljava/lang/String;CILjava/lang/String;)", name, (long)fb->signature[0], (long)i, signature);
        if (exceptionOccurred(EE())) return NULL;
    }
    return array;
}

long java_io_ObjectOutputStream_invokeObjectWriter(HObject *self, HObject *object, ClassClass *type)
{
    struct methodblock *mb = serial_hook(type, TRUE);
    if (!mb || exceptionOccurred(EE())) return FALSE;
    /* deviation: existing VM invocation performs stack growth/overflow checks. */
    do_execute_java_method(EE(), object, NULL, NULL, mb, FALSE, self);
    return TRUE;
}
long java_io_ObjectInputStream_invokeObjectReader(HObject *self, HObject *object, ClassClass *type)
{
    struct methodblock *mb = serial_hook(type, FALSE);
    if (!mb || exceptionOccurred(EE())) return FALSE;
    do_execute_java_method(EE(), object, NULL, NULL, mb, FALSE, self);
    return TRUE;
}
HObject *java_io_ObjectInputStream_allocateNewObject(HObject *unused, ClassClass *type, ClassClass *inittype)
{
    ClassClass *cb = type, *init = inittype;
    int i; struct methodblock *mb; HObject *object;
    if (cbAccess(cb) & (ACC_INTERFACE | ACC_ABSTRACT))
        return serial_error(JAVAPKG "InstantiationException", cbName(cb));
    for (i = 0; i < cbMethodsCount(init); i++) {
        mb = &cbMethods(init)[i];
        if (!strcmp(mb->fb.name, "<init>") && !strcmp(mb->fb.signature, "()V")) {
            if (!(mb->fb.access & ACC_PUBLIC)) return serial_error(JAVAPKG "IllegalAccessException", NULL);
            object = newobject(cb, NULL, EE());
            if (object) do_execute_java_method(EE(), object, NULL, NULL, mb, FALSE);
            return object;
        }
    }
    return serial_error(JAVAPKG "NoSuchMethodError", NULL);
}
HObject *java_io_ObjectInputStream_allocateNewArray(HObject *unused, ClassClass *type, long length)
{
    ClassClass *cb = type, *component;
    if (cbName(cb)[0] != '[') return serial_error("java/io/InvalidClassException", cbName(cb));
    component = reflect_find_class(cbName(cb) + 1, cb, NULL);
    return component ? reflect_new_array(component, length) : NULL;
}
ClassClass *java_io_ObjectInputStream_loadClass0(HObject *self, ClassClass *type, HString *name)
{
    ClassClass *context = type ? type : NULL, *cb;
    JavaFrame *frame; char buffer[256], *p;
    if (!context) for (frame = EE()->current_frame; frame; frame = frame->prev) {
        if (frame->current_method) {
            context = frame->current_method->fb.clazz;
            if (cbLoader(context)) break;
        }
    }
    javaString2CString(name, buffer, sizeof(buffer));
    for (p = buffer; *p; p++) if (*p == '.') *p = '/';
    cb = FindClassFromClass(EE(), buffer, TRUE, context);
    if (!cb) { serial_error(JAVAPKG "ClassNotFoundException", buffer); return NULL; }
    return cbHandle(cb);
}

void java_io_ObjectOutputStream_outputClassFields(HObject *self, HObject *object,
    ClassClass *type, HArrayOfInt *sequence)
{
    ClassClass *cb = type; int i, index, sig;
    struct fieldblock *fb; char *address; Java8 tmp;
    for (i = 0; i < (int)obj_length(sequence); i += 2) {
        index = unhand(sequence)->body[i + 1]; sig = unhand(sequence)->body[i];
        if (index < 0 || index >= cbFieldsCount(cb)) {
            serial_error("java/io/InvalidClassException", "field number too large"); return;
        }
        fb = &cbFields(cb)[index]; address = (char *)unhand(object) + fb->u.offset;
        switch (sig) {
        case 'B': execute_java_dynamic_method(EE(), self, "writeByte", "(I)V", *(int32_t *)address); break;
        case 'C': execute_java_dynamic_method(EE(), self, "writeChar", "(I)V", *(int32_t *)address); break;
        case 'S': execute_java_dynamic_method(EE(), self, "writeShort", "(I)V", *(int32_t *)address); break;
        case 'I': execute_java_dynamic_method(EE(), self, "writeInt", "(I)V", *(int32_t *)address); break;
        case 'Z': execute_java_dynamic_method(EE(), self, "writeBoolean", "(Z)V", *(int32_t *)address); break;
        case 'J': execute_java_dynamic_method(EE(), self, "writeLong", "(J)V", GET_INT64(tmp, address)); break;
        case 'F': execute_java_dynamic_method(EE(), self, "writeFloat", "(F)V", (double)*(float *)address); break;
        case 'D': execute_java_dynamic_method(EE(), self, "writeDouble", "(D)V", GET_DOUBLE(tmp, address)); break;
        case 'L': case '[':
            execute_java_dynamic_method(EE(), self, "writeObject", "(Ljava/lang/Object;)V", *(HObject **)address); break;
        default: serial_error("java/io/InvalidClassException", cbName(cb)); return;
        }
        if (exceptionOccurred(EE())) return;
    }
}

static long serial_read_wide(HObject *self, char *name, char *signature, long *other, ...)
{
    va_list args; long result;
    va_start(args, other);
    result = do_execute_java_method_vararg(EE(), self, name, signature, NULL, FALSE, args, other, FALSE);
    va_end(args); return result;
}
void java_io_ObjectInputStream_inputClassFields(HObject *self, HObject *object,
    ClassClass *type, HArrayOfInt *sequence)
{
    ClassClass *cb = type ? type : NULL, *fieldtype;
    int i, index, sig; long result, other;
    struct fieldblock *fb; char *address;
    HObject *value;
    for (i = 0; i < (int)obj_length(sequence); i += 2) {
        index = unhand(sequence)->body[i + 1]; sig = unhand(sequence)->body[i];
        fb = NULL;
        if (index >= 0 && cb) {
            if (index >= cbFieldsCount(cb)) { serial_error("java/io/InvalidClassException", "field number too large"); return; }
            fb = &cbFields(cb)[index];
        }
        other = 0;
        switch (sig) {
        case 'B': result = execute_java_dynamic_method(EE(), self, "readByte", "()B"); break;
        case 'C': result = execute_java_dynamic_method(EE(), self, "readChar", "()C"); break;
        case 'S': result = execute_java_dynamic_method(EE(), self, "readShort", "()S"); break;
        case 'I': result = execute_java_dynamic_method(EE(), self, "readInt", "()I"); break;
        case 'Z': result = execute_java_dynamic_method(EE(), self, "readBoolean", "()Z"); break;
        case 'F': result = execute_java_dynamic_method(EE(), self, "readFloat", "()F"); break;
        case 'J': result = serial_read_wide(self, "readLong", "()J", &other); break;
        case 'D': result = serial_read_wide(self, "readDouble", "()D", &other); break;
        case 'L': case '[': result = execute_java_dynamic_method(EE(), self, "readObject", "()Ljava/lang/Object;"); break;
        default: serial_error("java/io/InvalidClassException", cb ? cbName(cb) : NULL); return;
        }
        if (exceptionOccurred(EE())) return;
        if (!fb || !object) continue;
        address = (char *)unhand(object) + fb->u.offset;
        if (sig == 'L' || sig == '[') {
            value = (HObject *)result;
            fieldtype = reflect_find_class(fb->signature, cb, NULL);
            if (!fieldtype) return;
            if (value && !is_instance_of(value, fieldtype, EE())) {
                serial_error(JAVAPKG "ClassCastException", cbName(obj_array_classblock(value))); return;
            }
            *(HObject **)address = value;
        } else if (sig == 'J' || sig == 'D') {
            ((int32_t *)address)[0] = other; ((int32_t *)address)[1] = result;
        } else *(int32_t *)address = result;
    }
}
