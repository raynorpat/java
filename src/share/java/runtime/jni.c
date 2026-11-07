/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "jni_impl.h"
#include "tree.h"
#include "javaString.h"
#include "utf.h"
#include "monitor.h"
#include "exceptions.h"
#include "breakpoints.h"
#include "path.h"
extern ClassClass *allocClassClass(void);
extern sys_mon_t *_heap_lock;
#define HEAP_LOCK() sysMonitorEnter(_heap_lock)
#define HEAP_UNLOCK() sysMonitorExit(_heap_lock)

typedef JNIRefCell RefCell;
typedef JNIRefTable RefTable;
typedef struct JNIState {
    ExecEnv *ee;
    struct JNIState *next;
} JNIState;
typedef struct Pinned {
    JHandle *object;
    unsigned int count;
    struct Pinned *next;
} Pinned;

static const struct JNINativeInterface_ jni_NativeInterface;
static JNIState *environments;
JNIRefTable globalRefTable;
#define globalRefs globalRefTable
static Pinned *pinned;
#define JEE(env) JNIEnv2EE(env)

/* deviation: the current heap lock protects tables and pin counts, so the
 * old collector never suspends a thread while it owns a separate JNI lock. */
JNIEnv *jni_Env(ExecEnv *ee)
{
    JNIState *state;
    HEAP_LOCK();
    if (!ee->nativeInterface.functions) {
        state = (JNIState *)calloc(1, sizeof(*state));
        if (!state) {
            HEAP_UNLOCK();
            SignalError(ee, JAVAPKG "OutOfMemoryError", NULL);
            return NULL;
        }
        state->ee = ee;
        state->next = environments;
        environments = state;
        ee->nativeInterface.functions = &jni_NativeInterface;
    }
    HEAP_UNLOCK();
    return EE2JNIEnv(ee);
}

void jni_FreeEnv(ExecEnv *ee)
{
    JNIState **link, *state;
    HEAP_LOCK();
    for (link = &environments; *link && (*link)->ee != ee; link = &(*link)->next) {}
    state = *link;
    if (state) {
        *link = state->next;
        free(JNIEnvGetLocalRefs(EE2JNIEnv(ee))->elements);
        memset(&ee->nativeInterface, 0, sizeof(ee->nativeInterface));
        free(state);
    }
    HEAP_UNLOCK();
}

/* Called by the collector with the heap locked and other threads stopped. */
void jni_EnumerateRoots(void (*mark)(void *))
{
    JNIState *state;
    Pinned *pin;
    int i;
    for (i = 0; i < globalRefs.top; i++)
        if (globalRefs.elements[i].refCount) mark(globalRefs.elements[i].content);
    for (state = environments; state; state = state->next) {
        for (i = 0; i < JNIEnvGetLocalRefs(EE2JNIEnv(state->ee))->top; i++)
            if (JNIEnvGetLocalRefs(EE2JNIEnv(state->ee))->elements[i].refCount) mark(JNIEnvGetLocalRefs(EE2JNIEnv(state->ee))->elements[i].content);
        if (exceptionOccurred(state->ee)) mark(state->ee->exception.exc);
    }
    for (pin = pinned; pin; pin = pin->next) {
        mark(pin->object);
        mark(unhand(pin->object));
    }
}

/* Sun refcells are counted, positive local indices and negative globals. */
static int addRef(RefTable *table, JHandle *object)
{
    int i, empty = -1, capacity;
    RefCell *cells;
    if (object == NULL) return 0;
    for (i = table->top - 1; i >= table->base; i--) {
        if (table->elements[i].content == object) {
            table->elements[i].refCount++;
            return i + 1;
        }
        if (empty < 0 && table->elements[i].refCount == 0) empty = i;
    }
    if (empty < 0) {
        if (table->top == table->size) {
            capacity = table->size ? table->size * 2 : 4;
            cells = (RefCell *)realloc(table->elements, capacity * sizeof(*cells));
            if (cells == NULL) return 0;
            table->elements = cells;
            table->size = capacity;
        }
        empty = table->top++;
    }
    table->elements[empty].content = object;
    table->elements[empty].refCount = 1;
    return empty + 1;
}

static jobject localRef(JNIEnv *env, JHandle *object)
{
    int index;
    HEAP_LOCK();
    index = addRef(JNIEnvGetLocalRefs(env), object);
    HEAP_UNLOCK();
    if (object != NULL && index == 0)
        SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL);
    return (jobject)index;
}

static JHandle *resolveRef(JNIEnv *env, jobject ref)
{
    int index = (int)ref;
    JHandle *object;
    if (index == 0) return NULL;
    HEAP_LOCK();
    object = index > 0 ? JNIEnvGetLocalRefs(env)->elements[index - 1].content
                      : globalRefs.elements[-index - 1].content;
    HEAP_UNLOCK();
    return object;
}

static void JNICALL jni_FatalError(JNIEnv *env, const char *message)
{
    JavaFrame *frame;
    fprintf(stderr, "JNI panic: %s\n", message);
    for (frame = JEE(env)->current_frame; frame; frame = frame->prev) {
        if (frame->current_method) {
            char where[128];
            pc2string(frame->lastpc, frame->current_method, where, where + sizeof(where));
            fprintf(stderr, "\tat %s\n", where);
        }
    }
    sysExit(1);
}

static ClassClass *classRef(JNIEnv *env, jclass ref)
{
    JHandle *object = resolveRef(env, ref);
    if (object == NULL || !is_instance_of(object, classJavaLangClass, JEE(env))) {
        jni_FatalError(env, object ? "JNI received a class argument that is not a class"
                                  : "JNI received a null class");
        return NULL;
    }
    return (ClassClass *)object;
}

static int pinObject(JNIEnv *env, JHandle *object)
{
    Pinned *pin;
    HEAP_LOCK();
    for (pin = pinned; pin && pin->object != object; pin = pin->next) {}
    if (pin) pin->count++;
    else {
        pin = (Pinned *)malloc(sizeof(*pin));
        if (pin) {
            pin->object = object; pin->count = 1;
            pin->next = pinned; pinned = pin;
        }
    }
    HEAP_UNLOCK();
    return pin != NULL;
}

static void unpinObject(JHandle *object)
{
    Pinned **link, *pin;
    HEAP_LOCK();
    for (link = &pinned; *link && (*link)->object != object; link = &(*link)->next) {}
    pin = *link;
    if (pin && --pin->count == 0) { *link = pin->next; free(pin); }
    HEAP_UNLOCK();
}

static jint JNICALL jni_GetVersion(JNIEnv *env) { return 0x10001; }
static jobject JNICALL jni_NewGlobalRef(JNIEnv *env, jobject ref)
{
    int index;
    JHandle *object = resolveRef(env, ref);
    HEAP_LOCK();
    index = addRef(&globalRefs, object);
    HEAP_UNLOCK();
    return (jobject)-index;
}
static void JNICALL jni_DeleteGlobalRef(JNIEnv *env, jobject ref)
{
    if ((int)ref > 0) jni_FatalError(env, "DeleteGlobalRef received a local ref");
    if (ref) { HEAP_LOCK(); globalRefs.elements[-(int)ref - 1].refCount--; HEAP_UNLOCK(); }
}
static void JNICALL jni_DeleteLocalRef(JNIEnv *env, jobject ref)
{
    if ((int)ref < 0) jni_FatalError(env, "DeleteLocalRef received a global ref");
    if (ref) { HEAP_LOCK(); JNIEnvGetLocalRefs(env)->elements[(int)ref - 1].refCount--; HEAP_UNLOCK(); }
}
static jboolean JNICALL jni_IsSameObject(JNIEnv *env, jobject a, jobject b)
{ return resolveRef(env, a) == resolveRef(env, b); }
static jclass JNICALL jni_FindClass(JNIEnv *env, const char *name)
{
    ClassClass *cb = FindClassFromClass(JEE(env), (char *)name, TRUE, NULL);
    return cb ? localRef(env, (JHandle *)cbHandle(cb)) : NULL;
}
static jclass JNICALL jni_GetSuperclass(JNIEnv *env, jclass clazz)
{
    ClassClass *cb = classRef(env, clazz);
    return cbIsPrimitive(cb) || (cbAccess(cb) & ACC_INTERFACE) ? NULL
        : localRef(env, (JHandle *)cbSuperclass(cb));
}
static jboolean JNICALL jni_IsAssignableFrom(JNIEnv *env, jclass from, jclass to)
{
    extern bool_t reflect_is_assignable(ClassClass *, ClassClass *, ExecEnv *);
    return reflect_is_assignable(classRef(env, from), classRef(env, to), JEE(env));
}
static jint JNICALL jni_Throw(JNIEnv *env, jthrowable value)
{
    JHandle *object = resolveRef(env, value);
    exceptionThrow(JEE(env), object);
    return 0;
}
static jthrowable JNICALL jni_ExceptionOccurred(JNIEnv *env)
{ return exceptionOccurred(JEE(env)) ? localRef(env, JEE(env)->exception.exc) : NULL; }
static void JNICALL jni_ExceptionClear(JNIEnv *env) { exceptionClear(JEE(env)); }
static void JNICALL jni_ExceptionDescribe(JNIEnv *env)
{ if (exceptionOccurred(JEE(env))) exceptionDescribe(JEE(env)); }

static jobject JNICALL jni_AllocObject(JNIEnv *env, jclass clazz)
{
    ClassClass *cb = classRef(env, clazz);
    HObject *object;
    if (cbAccess(cb) & (ACC_INTERFACE | ACC_ABSTRACT)) {
        SignalError(JEE(env), JAVAPKG "InstantiationException", cbName(cb));
        return NULL;
    }
    object = ObjAlloc(cb, 0);
    if (!object) SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL);
    return localRef(env, object);
}
static jclass JNICALL jni_GetObjectClass(JNIEnv *env, jobject object)
{
    extern ClassClass *java_lang_Object_getClass(HObject *);
    return localRef(env, (JHandle *)java_lang_Object_getClass(resolveRef(env, object)));
}
static jboolean JNICALL jni_IsInstanceOf(JNIEnv *env, jobject object, jclass clazz)
{
    JHandle *value = resolveRef(env, object);
    return value == NULL || is_instance_of(value, classRef(env, clazz), JEE(env));
}

static int resolveClass(JNIEnv *env, ClassClass *cb)
{
    char *detail = NULL, *error;
    error = ResolveClass(cb, &detail);
    if (error) { SignalError(JEE(env), error, detail); return 0; }
    return 1;
}

static struct methodblock *findMethod(ClassClass *cb, const char *name, const char *sig)
{
    int i;
    struct methodblock *mb;
    for (; cb; cb = cbSuperclass(cb) ? cbSuperclass(cb) : NULL)
        for (i = 0, mb = cbMethods(cb); i < cbMethodsCount(cb); i++, mb++)
            if (!strcmp(fieldname(&mb->fb), name) && !strcmp(fieldsig(&mb->fb), sig))
                return mb;
    return NULL;
}
static jmethodID methodID(JNIEnv *env, jclass clazz, const char *name, const char *sig, int isStatic)
{
    ClassClass *cb = classRef(env, clazz);
    struct methodblock *mb;
    if (!resolveClass(env, cb)) return NULL;
    mb = findMethod(cb, name, sig);
    if (mb == NULL || !!(mb->fb.access & ACC_STATIC) != isStatic) {
        SignalError(JEE(env), JAVAPKG "NoSuchMethodError", (char *)name);
        return NULL;
    }
    return (jmethodID)mb;
}
static jmethodID JNICALL jni_GetMethodID(JNIEnv *e, jclass c, const char *n, const char *s)
{ return methodID(e, c, n, s, 0); }
static jmethodID JNICALL jni_GetStaticMethodID(JNIEnv *e, jclass c, const char *n, const char *s)
{ return methodID(e, c, n, s, 1); }

static struct fieldblock *findField(JNIEnv *env, ClassClass *cb, const char *name, const char *sig, int interfaces)
{
    ClassClass *current;
    struct fieldblock *fb;
    int i;
    for (current = cb; current; current = cbSuperclass(current) ? cbSuperclass(current) : NULL) {
        for (i = 0, fb = cbFields(current); i < cbFieldsCount(current); i++, fb++)
            if (!strcmp(fieldname(fb), name) && !strcmp(fieldsig(fb), sig)) return fb;
    }
    /* deviation: recover Sun's interface table through current constant pools. */
    if (interfaces) for (current = cb; current; current = cbSuperclass(current) ? cbSuperclass(current) : NULL) {
        for (i = 0; i < cbImplementsCount(current); i++) {
            int index = cbImplements(current)[i];
            if (!ResolveClassConstantFromClass(current, index, JEE(env), 1 << CONSTANT_Class)) return NULL;
            fb = findField(env, cbConstantPool(current)[index].p, name, sig, 1);
            if (fb) return fb;
        }
    }
    return NULL;
}
static jfieldID fieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig, int isStatic)
{
    ClassClass *cb = classRef(env, clazz);
    struct fieldblock *fb;
    if (!resolveClass(env, cb)) return NULL;
    fb = findField(env, cb, name, sig, isStatic);
    if (fb == NULL || !!(fb->access & ACC_STATIC) != isStatic) {
        SignalError(JEE(env), JAVAPKG "NoSuchFieldError", (char *)name);
        return NULL;
    }
    return (jfieldID)fb;
}
static jfieldID JNICALL jni_GetFieldID(JNIEnv *e, jclass c, const char *n, const char *s)
{ return fieldID(e, c, n, s, 0); }
static jfieldID JNICALL jni_GetStaticFieldID(JNIEnv *e, jclass c, const char *n, const char *s)
{ return fieldID(e, c, n, s, 1); }
static void *fieldAddress(JNIEnv *env, jobject object, jfieldID id, int isStatic)
{
    struct fieldblock *fb = (struct fieldblock *)id;
    if (isStatic) return fieldsig(fb)[0] == 'J' || fieldsig(fb)[0] == 'D' ?
        (void *)twoword_static_address(fb) : (void *)normal_static_address(fb);
    return (char *)unhand(resolveRef(env, object)) + fb->u.offset;
}
#define JNI_FIELD(Name, Type, Storage) \
static Type JNICALL jni_Get##Name##Field(JNIEnv *e, jobject o, jfieldID f) \
{ return (Type)*(Storage *)fieldAddress(e, o, f, 0); } \
static Type JNICALL jni_GetStatic##Name##Field(JNIEnv *e, jclass c, jfieldID f) \
{ return (Type)*(Storage *)fieldAddress(e, c, f, 1); } \
static void JNICALL jni_Set##Name##Field(JNIEnv *e, jobject o, jfieldID f, Type v) \
{ *(Storage *)fieldAddress(e, o, f, 0) = v; } \
static void JNICALL jni_SetStatic##Name##Field(JNIEnv *e, jclass c, jfieldID f, Type v) \
{ *(Storage *)fieldAddress(e, c, f, 1) = v; }
JNI_FIELD(Boolean, jboolean, int32_t)
JNI_FIELD(Byte, jbyte, int32_t)
JNI_FIELD(Char, jchar, int32_t)
JNI_FIELD(Short, jshort, int32_t)
JNI_FIELD(Int, jint, int32_t)
JNI_FIELD(Long, jlong, int64_t)
JNI_FIELD(Float, jfloat, float)
JNI_FIELD(Double, jdouble, double)
static jobject JNICALL jni_GetObjectField(JNIEnv *e, jobject o, jfieldID f)
{ return localRef(e, *(JHandle **)fieldAddress(e, o, f, 0)); }
static jobject JNICALL jni_GetStaticObjectField(JNIEnv *e, jclass c, jfieldID f)
{ return localRef(e, *(JHandle **)fieldAddress(e, c, f, 1)); }
static void JNICALL jni_SetObjectField(JNIEnv *e, jobject o, jfieldID f, jobject v)
{ *(JHandle **)fieldAddress(e, o, f, 0) = resolveRef(e, v); }
static void JNICALL jni_SetStaticObjectField(JNIEnv *e, jclass c, jfieldID f, jobject v)
{ *(JHandle **)fieldAddress(e, c, f, 1) = resolveRef(e, v); }

static const char *nextType(const char *p)
{
    while (*p == '[') p++;
    if (*p == 'L') while (*p != ';') p++;
    return p + 1;
}
static void varargsValues(struct methodblock *mb, va_list args, jvalue *values)
{
    const char *p;
    for (p = fieldsig(&mb->fb) + 1; *p != ')'; p = nextType(p), values++) {
        switch (*p) {
        case 'J': values->j = va_arg(args, jlong); break;
        case 'D': values->d = va_arg(args, jdouble); break;
        case 'F': values->f = (jfloat)va_arg(args, double); break;
        case 'L': case '[': values->l = va_arg(args, jobject); break;
        default: values->i = va_arg(args, jint); break;
        }
    }
}

/* deviation: use the existing interpreter's pseudo-frame entry rather than
 * duplicating its stack allocation; x86 va_list contains packed stack slots. */
static jvalue callMethod(JNIEnv *env, jobject receiver, jmethodID id, jvalue *values, int mode)
{
    struct methodblock *mb = (struct methodblock *)id;
    stack_item slots[256], *slot = slots;
    const char *p;
    JHandle *object;
    ClassClass *receiverClass;
    Java8 bits;
    long low = 0, high;
    jvalue result;
    memset(&result, 0, sizeof(result));
    object = mode == 2 ? (JHandle *)classRef(env, receiver) : resolveRef(env, receiver);
    if (mode != 2 && object == NULL) {
        jni_FatalError(env, "Native code passed a null object to a method");
        return result;
    }
    receiverClass = mode == 2 ? (ClassClass *)object : obj_array_classblock(object);
    {
        extern bool_t reflect_is_assignable(ClassClass *, ClassClass *, ExecEnv *);
        if (!reflect_is_assignable(receiverClass, fieldclass(&mb->fb), JEE(env))) {
            jni_FatalError(env, "Native code used a method with an incompatible receiver");
            return result;
        }
    }
    if (mode != 2 && ((mb->fb.access & ACC_PRIVATE) || fieldname(&mb->fb)[0] == '<') &&
        receiverClass != fieldclass(&mb->fb)) {
        jni_FatalError(env, "Private method or constructor invoked on a subclass");
        return result;
    }
    if (mode == 0 && !(mb->fb.access & ACC_PRIVATE) && fieldname(&mb->fb)[0] != '<') {
        struct methodblock *target = findMethod(obj_array_classblock(object), fieldname(&mb->fb), fieldsig(&mb->fb));
        if (target) mb = target;
    }
    for (p = fieldsig(&mb->fb) + 1; *p != ')'; p = nextType(p), values++) {
        switch (*p) {
        case 'J': SET_INT64(bits, slot, values->j); slot += 2; break;
        case 'D': SET_DOUBLE(bits, slot, values->d); slot += 2; break;
        case 'F': (slot++)->f = values->f; break;
        case 'L': case '[': (slot++)->h = resolveRef(env, values->l); break;
        default: (slot++)->i = values->i; break;
        }
    }
    high = do_execute_java_method_vararg(JEE(env), object, NULL, NULL, mb,
        mode == 2, (va_list)slots, &low, TRUE);
    if (exceptionOccurred(JEE(env))) return result;
    if (p[1] == 'J' || p[1] == 'D') {
        bits.x[0] = low; bits.x[1] = high;
        memcpy(&result, &bits, sizeof(bits));
    } else if (p[1] == 'L' || p[1] == '[') result.l = localRef(env, (JHandle *)high);
    else if (p[1] == 'F') { memcpy(&result.f, &high, sizeof(result.f)); }
    else result.i = high;
    return result;
}
#define JNI_CALL(Name, Type, Member) \
static Type JNICALL jni_Call##Name##MethodA(JNIEnv *e, jobject o, jmethodID m, jvalue *a) \
{ return callMethod(e, o, m, a, 0).Member; } \
static Type JNICALL jni_Call##Name##MethodV(JNIEnv *e, jobject o, jmethodID m, va_list a) \
{ jvalue v[256]; varargsValues((struct methodblock *)m, a, v); return callMethod(e, o, m, v, 0).Member; } \
static Type JNICALL jni_Call##Name##Method(JNIEnv *e, jobject o, jmethodID m, ...) \
{ Type r; va_list a; va_start(a, m); r = jni_Call##Name##MethodV(e, o, m, a); va_end(a); return r; } \
static Type JNICALL jni_CallNonvirtual##Name##MethodA(JNIEnv *e, jobject o, jclass c, jmethodID m, jvalue *a) \
{ return callMethod(e, o, m, a, 1).Member; } \
static Type JNICALL jni_CallNonvirtual##Name##MethodV(JNIEnv *e, jobject o, jclass c, jmethodID m, va_list a) \
{ jvalue v[256]; varargsValues((struct methodblock *)m, a, v); return callMethod(e, o, m, v, 1).Member; } \
static Type JNICALL jni_CallNonvirtual##Name##Method(JNIEnv *e, jobject o, jclass c, jmethodID m, ...) \
{ Type r; va_list a; va_start(a, m); r = jni_CallNonvirtual##Name##MethodV(e, o, c, m, a); va_end(a); return r; } \
static Type JNICALL jni_CallStatic##Name##MethodA(JNIEnv *e, jclass c, jmethodID m, jvalue *a) \
{ return callMethod(e, c, m, a, 2).Member; } \
static Type JNICALL jni_CallStatic##Name##MethodV(JNIEnv *e, jclass c, jmethodID m, va_list a) \
{ jvalue v[256]; varargsValues((struct methodblock *)m, a, v); return callMethod(e, c, m, v, 2).Member; } \
static Type JNICALL jni_CallStatic##Name##Method(JNIEnv *e, jclass c, jmethodID m, ...) \
{ Type r; va_list a; va_start(a, m); r = jni_CallStatic##Name##MethodV(e, c, m, a); va_end(a); return r; }
JNI_CALL(Object, jobject, l)
JNI_CALL(Boolean, jboolean, z)
JNI_CALL(Byte, jbyte, b)
JNI_CALL(Char, jchar, c)
JNI_CALL(Short, jshort, s)
JNI_CALL(Int, jint, i)
JNI_CALL(Long, jlong, j)
JNI_CALL(Float, jfloat, f)
JNI_CALL(Double, jdouble, d)

#define JNI_VOID(Prefix, Extra, Pass, Mode) \
static void JNICALL jni_Call##Prefix##VoidMethodA(JNIEnv *e, jobject o, Extra jmethodID m, jvalue *a) \
{ callMethod(e, o, m, a, Mode); } \
static void JNICALL jni_Call##Prefix##VoidMethodV(JNIEnv *e, jobject o, Extra jmethodID m, va_list a) \
{ jvalue v[256]; varargsValues((struct methodblock *)m, a, v); callMethod(e, o, m, v, Mode); } \
static void JNICALL jni_Call##Prefix##VoidMethod(JNIEnv *e, jobject o, Extra jmethodID m, ...) \
{ va_list a; va_start(a, m); jni_Call##Prefix##VoidMethodV(e, o, Pass m, a); va_end(a); }
#define JNI_CLASS_ARG jclass c,
#define JNI_CLASS_PASS c,
JNI_VOID(, , , 0)
JNI_VOID(Nonvirtual, JNI_CLASS_ARG, JNI_CLASS_PASS, 1)
JNI_VOID(Static, , , 2)

static jobject JNICALL jni_NewObjectA(JNIEnv *env, jclass clazz, jmethodID method, jvalue *args)
{
    jobject object = jni_AllocObject(env, clazz);
    if (object) {
        callMethod(env, object, method, args, 1);
        if (exceptionOccurred(JEE(env))) { jni_DeleteLocalRef(env, object); return NULL; }
    }
    return object;
}
static jobject JNICALL jni_NewObjectV(JNIEnv *e, jclass c, jmethodID m, va_list a)
{ jvalue v[256]; varargsValues((struct methodblock *)m, a, v); return jni_NewObjectA(e, c, m, v); }
static jobject JNICALL jni_NewObject(JNIEnv *e, jclass c, jmethodID m, ...)
{ jobject o; va_list a; va_start(a, m); o = jni_NewObjectV(e, c, m, a); va_end(a); return o; }

static Hjava_lang_String *stringRef(JNIEnv *env, jstring ref)
{
    JHandle *object = resolveRef(env, ref);
    if (!object || !is_instance_of(object, classJavaLangString, JEE(env))) {
        jni_FatalError(env, "JNI received an invalid string"); return NULL;
    }
    return (Hjava_lang_String *)object;
}
static jstring JNICALL jni_NewString(JNIEnv *env, const jchar *chars, jsize length)
{
    HArrayOfChar *array = (HArrayOfChar *)ArrayAlloc(T_CHAR, length);
    HObject *str;
    if (!array) { SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    if (length) memcpy(unhand(array)->body, chars, length * sizeof(jchar));
    str = (HObject *)execute_java_constructor(JEE(env), NULL, classJavaLangString, "([C)", array);
    return (jstring)localRef(env, str);
}
static jsize JNICALL jni_GetStringLength(JNIEnv *env, jstring ref)
{ return javaStringLength(stringRef(env, ref)); }
static const jchar *JNICALL jni_GetStringChars(JNIEnv *env, jstring ref, jboolean *copy)
{
    Hjava_lang_String *str = stringRef(env, ref);
    HArrayOfChar *array = unhand(str)->value;
    if (!pinObject(env, (JHandle *)array)) {
        SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL;
    }
    if (copy) *copy = JNI_FALSE;
    return unhand(array)->body + unhand(str)->offset;
}
static void JNICALL jni_ReleaseStringChars(JNIEnv *env, jstring ref, const jchar *chars)
{ unpinObject((JHandle *)unhand(stringRef(env, ref))->value); }
static jstring JNICALL jni_NewStringUTF(JNIEnv *env, const char *utf)
{
    int length;
    HArrayOfChar *array;
    HObject *str;
    if (!utf) return NULL;
    length = utfstrlen((char *)utf);
    array = (HArrayOfChar *)ArrayAlloc(T_CHAR, length);
    if (!array) { SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    utf2unicode((char *)utf, unhand(array)->body, length, &length);
    str = (HObject *)execute_java_constructor(JEE(env), NULL, classJavaLangString, "([C)", array);
    return (jstring)localRef(env, str);
}
static jsize JNICALL jni_GetStringUTFLength(JNIEnv *env, jstring ref)
{
    Hjava_lang_String *str = stringRef(env, ref);
    return unicode2utfstrlen(unhand(unhand(str)->value)->body + unhand(str)->offset,
                            unhand(str)->count) - 1;
}
static const char *JNICALL jni_GetStringUTFChars(JNIEnv *env, jstring ref, jboolean *copy)
{
    Hjava_lang_String *str = stringRef(env, ref);
    if (copy) *copy = JNI_TRUE;
    return unicode2utf(unhand(unhand(str)->value)->body + unhand(str)->offset,
                       unhand(str)->count, NULL, 0);
}
static void JNICALL jni_ReleaseStringUTFChars(JNIEnv *env, jstring ref, const char *chars)
{ free((void *)chars); }
static jint JNICALL jni_ThrowNew(JNIEnv *env, jclass clazz, const char *message)
{
    jmethodID constructor = jni_GetMethodID(env, clazz, "<init>", "(Ljava/lang/String;)V");
    jstring string;
    jobject exception;
    if (!constructor) return -1;
    string = jni_NewStringUTF(env, message);
    if (exceptionOccurred(JEE(env))) return -1;
    exception = jni_NewObject(env, clazz, constructor, string);
    jni_DeleteLocalRef(env, string);
    if (!exception) return -1;
    jni_Throw(env, exception);
    jni_DeleteLocalRef(env, exception);
    return 0;
}

static JHandle *arrayRef(JNIEnv *env, jarray ref, int type)
{
    JHandle *object = resolveRef(env, ref);
    if (!object || !obj_flags(object) || (type && obj_flags(object) != type)) {
        jni_FatalError(env, "JNI received an invalid array"); return NULL;
    }
    return object;
}
static jsize JNICALL jni_GetArrayLength(JNIEnv *env, jarray ref)
{ return obj_length(arrayRef(env, ref, 0)); }
static jobjectArray JNICALL jni_NewObjectArray(JNIEnv *env, jsize length, jclass clazz, jobject initial)
{
    ClassClass *component = classRef(env, clazz);
    HArrayOfObject *array = (HArrayOfObject *)ArrayAlloc(T_CLASS, length);
    JHandle *value = resolveRef(env, initial);
    int i;
    if (!array) { SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    ((ClassClass **)unhand(array)->body)[length] = component;
    if (value && !is_instance_of(value, component, JEE(env))) {
        SignalError(JEE(env), JAVAPKG "ArrayStoreException", NULL); return NULL;
    }
    for (i = 0; i < length; i++) unhand(array)->body[i] = value;
    return (jobjectArray)localRef(env, (JHandle *)array);
}
static int arrayBounds(JNIEnv *env, JHandle *array, jsize start, jsize length)
{
    if (start < 0 || length < 0 || start > (jsize)obj_length(array) ||
        length > (jsize)obj_length(array) - start) {
        SignalError(JEE(env), JAVAPKG "ArrayIndexOutOfBoundsException", NULL); return FALSE;
    }
    return TRUE;
}
static jobject JNICALL jni_GetObjectArrayElement(JNIEnv *env, jobjectArray ref, jsize index)
{
    HArrayOfObject *array = (HArrayOfObject *)arrayRef(env, ref, T_CLASS);
    if (!arrayBounds(env, (JHandle *)array, index, 1)) return NULL;
    return localRef(env, unhand(array)->body[index]);
}
static void JNICALL jni_SetObjectArrayElement(JNIEnv *env, jobjectArray ref, jsize index, jobject value)
{
    HArrayOfObject *array = (HArrayOfObject *)arrayRef(env, ref, T_CLASS);
    JHandle *object = resolveRef(env, value);
    ClassClass *component;
    if (!arrayBounds(env, (JHandle *)array, index, 1)) return;
    component = ((ClassClass **)unhand(array)->body)[obj_length(array)];
    if (object && !is_instance_of(object, component, JEE(env))) {
        SignalError(JEE(env), JAVAPKG "ArrayStoreException", NULL); return;
    }
    unhand(array)->body[index] = object;
}
#define JNI_ARRAY(Name, Type, Code) \
static Type##Array JNICALL jni_New##Name##Array(JNIEnv *env, jsize length) \
{ JHandle *a = ArrayAlloc(Code, length); \
  if (!a) SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); \
  return (Type##Array)localRef(env, a); } \
static Type *JNICALL jni_Get##Name##ArrayElements(JNIEnv *env, Type##Array ref, jboolean *copy) \
{ JHandle *a = arrayRef(env, ref, Code); \
  if (!pinObject(env, a)) { SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL; } \
  if (copy) *copy = JNI_FALSE; return (Type *)unhand(a); } \
static void JNICALL jni_Release##Name##ArrayElements(JNIEnv *env, Type##Array ref, Type *data, jint mode) \
{ unpinObject(arrayRef(env, ref, Code)); } \
static void JNICALL jni_Get##Name##ArrayRegion(JNIEnv *env, Type##Array ref, jsize start, jsize length, Type *data) \
{ JHandle *a = arrayRef(env, ref, Code); \
  if (arrayBounds(env, a, start, length)) memcpy(data, (Type *)unhand(a) + start, length * sizeof(Type)); } \
static void JNICALL jni_Set##Name##ArrayRegion(JNIEnv *env, Type##Array ref, jsize start, jsize length, Type *data) \
{ JHandle *a = arrayRef(env, ref, Code); \
  if (arrayBounds(env, a, start, length)) memcpy((Type *)unhand(a) + start, data, length * sizeof(Type)); }
JNI_ARRAY(Boolean, jboolean, T_BOOLEAN)
JNI_ARRAY(Byte, jbyte, T_BYTE)
JNI_ARRAY(Char, jchar, T_CHAR)
JNI_ARRAY(Short, jshort, T_SHORT)
JNI_ARRAY(Int, jint, T_INT)
JNI_ARRAY(Long, jlong, T_LONG)
JNI_ARRAY(Float, jfloat, T_FLOAT)
JNI_ARRAY(Double, jdouble, T_DOUBLE)

static jint JNICALL jni_RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods, jint count)
{
    ClassClass *cb = classRef(env, clazz);
    struct methodblock *mb;
    while (--count >= 0) {
        mb = findMethod(cb, methods[count].name, methods[count].signature);
        if (!mb || !(mb->fb.access & ACC_NATIVE)) {
            SignalError(JEE(env), JAVAPKG "NoSuchMethodError", methods[count].name); return -1;
        }
        mb->code = (unsigned char *)methods[count].fnPtr;
        mb->invoker = mb->fb.access & ACC_SYNCHRONIZED ?
            invokeSynchronizedJNINativeMethod : invokeJNINativeMethod;
    }
    return 0;
}
static jint JNICALL jni_UnregisterNatives(JNIEnv *env, jclass clazz)
{
    ClassClass *cb = classRef(env, clazz);
    int i;
    for (i = 0; i < cbMethodsCount(cb); i++) {
        struct methodblock *mb = &cbMethods(cb)[i];
        if (mb->fb.access & ACC_NATIVE) { mb->code = NULL; mb->invoker = invokeLazyNativeMethod; }
    }
    return 0;
}
static jint JNICALL jni_MonitorEnter(JNIEnv *env, jobject ref)
{ monitorEnter(obj_monitor(resolveRef(env, ref))); return 0; }
static jint JNICALL jni_MonitorExit(JNIEnv *env, jobject ref)
{ monitorExit(obj_monitor(resolveRef(env, ref))); return 0; }
static jint JNICALL jni_GetJavaVM(JNIEnv *env, JavaVM **vm)
{ *vm = (JavaVM *)&jni_vm; return 0; }

static jclass JNICALL jni_DefineClass(JNIEnv *env, const char *name, jobject loader,
                                     const jbyte *bytes, jsize length)
{
    ClassClass *cb = allocClassClass();
    JHandle *object = resolveRef(env, loader);
    char *error, *detail;
    int i, ok;
    if (!cb) { SignalError(JEE(env), JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    cbLoader(cb) = (struct Hjava_lang_ClassLoader *)object;
    lock_classes();
    BINCLASS_LOCK();
    if (name) for (i = 0; i < nbinclasses; i++) {
        if (cbLoader(binclasses[i]) == object && !strcmp(name, cbName(binclasses[i]))) {
            BINCLASS_UNLOCK();
            unlock_classes();
            SignalError(JEE(env), JAVAPKG "ClassFormatError", "duplicate name"); return NULL;
        }
    }
    BINCLASS_UNLOCK();
    ok = createInternalClass((unsigned char *)bytes, (unsigned char *)bytes + length, cb,
                             (struct Hjava_lang_ClassLoader *)object, (char *)name, &detail);
    if (!ok) {
        if (!exceptionOccurred(JEE(env))) SignalError(JEE(env), JAVAPKG "ClassFormatError", NULL);
        unlock_classes(); return NULL;
    }
    unlock_classes();
    error = InitializeClass(cb, &detail);
    if (error) { SignalError(JEE(env), error, detail); return NULL; }
    return (jclass)localRef(env, (JHandle *)cbHandle(cb));
}

static void invokeJNINativeMethodBody(JHandle *object, struct methodblock *mb, int size, ExecEnv *ee)
{
    JNIEnv *env = jni_Env(ee);
    JNIRefTable *locals;
    JavaFrame *frame = ee->current_frame;
    stack_item *args = frame->optop, *slot, *result;
    const char *sig;
    jobject clazz = NULL;
    int savedBase, savedTop;
    if (!env) { SignalError(ee, JAVAPKG "OutOfMemoryError", NULL); return; }
    /* The local table belongs to this EE; other attached EEs may detach. */
    locals = JNIEnvGetLocalRefs(env);
    HEAP_LOCK();
    savedBase = locals->base; savedTop = locals->top;
    locals->base = locals->top;
    HEAP_UNLOCK();
    slot = args;
    if (mb->fb.access & ACC_STATIC) clazz = localRef(env, (JHandle *)cbHandle(fieldclass(&mb->fb)));
    else { slot->p = localRef(env, slot->h); slot++; }
    for (sig = fieldsig(&mb->fb) + 1; *sig != ')'; sig = nextType(sig)) {
        if (*sig == '[' || *sig == 'L') slot->p = localRef(env, slot->h);
        slot += (*sig == 'J' || *sig == 'D') ? 2 : 1;
    }
    result = args;
    if (!exceptionOccurred(ee)) {
        result = (stack_item *)sysInvokeNative(env, mb->code, (long *)args, (char *)sig + 1, size, clazz);
        switch (sig[1]) {
        case 'L': case '[': args->h = resolveRef(env, (jobject)args->p); break;
        case 'B': args->i = (jbyte)args->i; break;
        case 'S': args->i = (jshort)args->i; break;
        case 'Z': args->i = (jboolean)args->i; break;
        case 'C': args->i = (jchar)args->i; break;
        }
    }
    HEAP_LOCK();
    locals->base = savedBase; locals->top = savedTop;
    HEAP_UNLOCK();
    frame->optop = result;
}
bool_t invokeJNINativeMethod(JHandle *object, struct methodblock *mb, int size, ExecEnv *ee)
{
    invokeJNINativeMethodBody(object, mb, size, ee);
#ifdef DEBUG
    /* Sun100131C0 reports after restoring JNI locals and the result stack. */
    if (exceptionOccurred(ee))
        notify_debugger_of_exception(mb->code, ee, ee->exception.exc);
#endif
    return !exceptionOccurred(ee);
}
bool_t invokeSynchronizedJNINativeMethod(JHandle *object, struct methodblock *mb, int size, ExecEnv *ee)
{
    monitorEnter(obj_monitor(object));
    invokeJNINativeMethodBody(object, mb, size, ee);
    monitorExit(obj_monitor(object));
#ifdef DEBUG
    /* Sun100135F7 releases the method monitor before the exception callback. */
    if (exceptionOccurred(ee))
        notify_debugger_of_exception(mb->code, ee, ee->exception.exc);
#endif
    return !exceptionOccurred(ee);
}

static char *jniMangle(char *out, const char *text, int length)
{
    char *end = (char *)text + length, *p = (char *)text;
    while (p < end) {
        unsigned int c = next_utf2unicode(&p);
        if (c == '/') *out++ = '_';
        else if (c == '_' || c == ';' || c == '[') {
            *out++ = '_'; *out++ = c == '_' ? '1' : c == ';' ? '2' : '3';
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9')) *out++ = (char)c;
        else { sprintf(out, "_0%04x", c); out += 6; }
    }
    *out = 0;
    return out;
}
bool_t dynoLinkJNI(struct methodblock *mb)
{
    char *name, *end, *sig = fieldsig(&mb->fb), *close = strchr(sig, ')');
    const char *className = cbName(fieldclass(&mb->fb)), *methodName = fieldname(&mb->fb);
    int capacity = 32 + 6 * (strlen(className) + strlen(methodName) + strlen(sig));
    int overload;
    name = (char *)malloc(capacity);
    if (!name) return FALSE;
    strcpy(name, "Java_");
    end = jniMangle(name + 5, className, strlen(className));
    *end++ = '_'; end = jniMangle(end, methodName, strlen(methodName));
    for (overload = 0; overload < 2; overload++) {
#ifdef WIN32
        char *decorated = (char *)malloc(capacity + 16);
        if (decorated) {
            sprintf(decorated, "_%s@%d", name,
                (mb->args_size + 1 + ((mb->fb.access & ACC_STATIC) != 0)) * 4);
            mb->code = (unsigned char *)sysDynamicLink(decorated);
            free(decorated);
        }
#endif
        if (!mb->code) mb->code = (unsigned char *)sysDynamicLink(name);
        if (mb->code) break;
        *end++ = '_'; *end++ = '_';
        jniMangle(end, sig + 1, close - sig - 1);
    }
    free(name);
    return mb->code != NULL;
}

static const struct JNINativeInterface_ jni_NativeInterface = {
    NULL,
    NULL,
    NULL,
    NULL,
    jni_GetVersion,
    jni_DefineClass,
    jni_FindClass,
    NULL,
    NULL,
    NULL,
    jni_GetSuperclass,
    jni_IsAssignableFrom,
    NULL,
    jni_Throw,
    jni_ThrowNew,
    jni_ExceptionOccurred,
    jni_ExceptionDescribe,
    jni_ExceptionClear,
    jni_FatalError,
    NULL,
    NULL,
    jni_NewGlobalRef,
    jni_DeleteGlobalRef,
    jni_DeleteLocalRef,
    jni_IsSameObject,
    NULL,
    NULL,
    jni_AllocObject,
    jni_NewObject,
    jni_NewObjectV,
    jni_NewObjectA,
    jni_GetObjectClass,
    jni_IsInstanceOf,
    jni_GetMethodID,
    jni_CallObjectMethod,
    jni_CallObjectMethodV,
    jni_CallObjectMethodA,
    jni_CallBooleanMethod,
    jni_CallBooleanMethodV,
    jni_CallBooleanMethodA,
    jni_CallByteMethod,
    jni_CallByteMethodV,
    jni_CallByteMethodA,
    jni_CallCharMethod,
    jni_CallCharMethodV,
    jni_CallCharMethodA,
    jni_CallShortMethod,
    jni_CallShortMethodV,
    jni_CallShortMethodA,
    jni_CallIntMethod,
    jni_CallIntMethodV,
    jni_CallIntMethodA,
    jni_CallLongMethod,
    jni_CallLongMethodV,
    jni_CallLongMethodA,
    jni_CallFloatMethod,
    jni_CallFloatMethodV,
    jni_CallFloatMethodA,
    jni_CallDoubleMethod,
    jni_CallDoubleMethodV,
    jni_CallDoubleMethodA,
    jni_CallVoidMethod,
    jni_CallVoidMethodV,
    jni_CallVoidMethodA,
    jni_CallNonvirtualObjectMethod,
    jni_CallNonvirtualObjectMethodV,
    jni_CallNonvirtualObjectMethodA,
    jni_CallNonvirtualBooleanMethod,
    jni_CallNonvirtualBooleanMethodV,
    jni_CallNonvirtualBooleanMethodA,
    jni_CallNonvirtualByteMethod,
    jni_CallNonvirtualByteMethodV,
    jni_CallNonvirtualByteMethodA,
    jni_CallNonvirtualCharMethod,
    jni_CallNonvirtualCharMethodV,
    jni_CallNonvirtualCharMethodA,
    jni_CallNonvirtualShortMethod,
    jni_CallNonvirtualShortMethodV,
    jni_CallNonvirtualShortMethodA,
    jni_CallNonvirtualIntMethod,
    jni_CallNonvirtualIntMethodV,
    jni_CallNonvirtualIntMethodA,
    jni_CallNonvirtualLongMethod,
    jni_CallNonvirtualLongMethodV,
    jni_CallNonvirtualLongMethodA,
    jni_CallNonvirtualFloatMethod,
    jni_CallNonvirtualFloatMethodV,
    jni_CallNonvirtualFloatMethodA,
    jni_CallNonvirtualDoubleMethod,
    jni_CallNonvirtualDoubleMethodV,
    jni_CallNonvirtualDoubleMethodA,
    jni_CallNonvirtualVoidMethod,
    jni_CallNonvirtualVoidMethodV,
    jni_CallNonvirtualVoidMethodA,
    jni_GetFieldID,
    jni_GetObjectField,
    jni_GetBooleanField,
    jni_GetByteField,
    jni_GetCharField,
    jni_GetShortField,
    jni_GetIntField,
    jni_GetLongField,
    jni_GetFloatField,
    jni_GetDoubleField,
    jni_SetObjectField,
    jni_SetBooleanField,
    jni_SetByteField,
    jni_SetCharField,
    jni_SetShortField,
    jni_SetIntField,
    jni_SetLongField,
    jni_SetFloatField,
    jni_SetDoubleField,
    jni_GetStaticMethodID,
    jni_CallStaticObjectMethod,
    jni_CallStaticObjectMethodV,
    jni_CallStaticObjectMethodA,
    jni_CallStaticBooleanMethod,
    jni_CallStaticBooleanMethodV,
    jni_CallStaticBooleanMethodA,
    jni_CallStaticByteMethod,
    jni_CallStaticByteMethodV,
    jni_CallStaticByteMethodA,
    jni_CallStaticCharMethod,
    jni_CallStaticCharMethodV,
    jni_CallStaticCharMethodA,
    jni_CallStaticShortMethod,
    jni_CallStaticShortMethodV,
    jni_CallStaticShortMethodA,
    jni_CallStaticIntMethod,
    jni_CallStaticIntMethodV,
    jni_CallStaticIntMethodA,
    jni_CallStaticLongMethod,
    jni_CallStaticLongMethodV,
    jni_CallStaticLongMethodA,
    jni_CallStaticFloatMethod,
    jni_CallStaticFloatMethodV,
    jni_CallStaticFloatMethodA,
    jni_CallStaticDoubleMethod,
    jni_CallStaticDoubleMethodV,
    jni_CallStaticDoubleMethodA,
    jni_CallStaticVoidMethod,
    jni_CallStaticVoidMethodV,
    jni_CallStaticVoidMethodA,
    jni_GetStaticFieldID,
    jni_GetStaticObjectField,
    jni_GetStaticBooleanField,
    jni_GetStaticByteField,
    jni_GetStaticCharField,
    jni_GetStaticShortField,
    jni_GetStaticIntField,
    jni_GetStaticLongField,
    jni_GetStaticFloatField,
    jni_GetStaticDoubleField,
    jni_SetStaticObjectField,
    jni_SetStaticBooleanField,
    jni_SetStaticByteField,
    jni_SetStaticCharField,
    jni_SetStaticShortField,
    jni_SetStaticIntField,
    jni_SetStaticLongField,
    jni_SetStaticFloatField,
    jni_SetStaticDoubleField,
    jni_NewString,
    jni_GetStringLength,
    jni_GetStringChars,
    jni_ReleaseStringChars,
    jni_NewStringUTF,
    jni_GetStringUTFLength,
    jni_GetStringUTFChars,
    jni_ReleaseStringUTFChars,
    jni_GetArrayLength,
    jni_NewObjectArray,
    jni_GetObjectArrayElement,
    jni_SetObjectArrayElement,
    jni_NewBooleanArray,
    jni_NewByteArray,
    jni_NewCharArray,
    jni_NewShortArray,
    jni_NewIntArray,
    jni_NewLongArray,
    jni_NewFloatArray,
    jni_NewDoubleArray,
    jni_GetBooleanArrayElements,
    jni_GetByteArrayElements,
    jni_GetCharArrayElements,
    jni_GetShortArrayElements,
    jni_GetIntArrayElements,
    jni_GetLongArrayElements,
    jni_GetFloatArrayElements,
    jni_GetDoubleArrayElements,
    jni_ReleaseBooleanArrayElements,
    jni_ReleaseByteArrayElements,
    jni_ReleaseCharArrayElements,
    jni_ReleaseShortArrayElements,
    jni_ReleaseIntArrayElements,
    jni_ReleaseLongArrayElements,
    jni_ReleaseFloatArrayElements,
    jni_ReleaseDoubleArrayElements,
    jni_GetBooleanArrayRegion,
    jni_GetByteArrayRegion,
    jni_GetCharArrayRegion,
    jni_GetShortArrayRegion,
    jni_GetIntArrayRegion,
    jni_GetLongArrayRegion,
    jni_GetFloatArrayRegion,
    jni_GetDoubleArrayRegion,
    jni_SetBooleanArrayRegion,
    jni_SetByteArrayRegion,
    jni_SetCharArrayRegion,
    jni_SetShortArrayRegion,
    jni_SetIntArrayRegion,
    jni_SetLongArrayRegion,
    jni_SetFloatArrayRegion,
    jni_SetDoubleArrayRegion,
    jni_RegisterNatives,
    jni_UnregisterNatives,
    jni_MonitorEnter,
    jni_MonitorExit,
    jni_GetJavaVM
};
