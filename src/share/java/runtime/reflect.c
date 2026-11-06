/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Primary addresses and transitional representations: REFLECT115-EVIDENCE.md. */
#include <string.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "exceptions.h"
#include "opcodes.h"

/* The Java fields and their order are confirmed by Sun's reflection classes
 * and the creation routines 100324B6, 100357C1 and 10036568. */
typedef struct { ClassClass *clazz; long slot; HString *name; ClassClass *type; } ReflectField;
typedef struct { ClassClass *clazz; long slot; HString *name; ClassClass *type;
    HArrayOfObject *parameters; HArrayOfObject *exceptions; } ReflectMethod;
typedef struct { ClassClass *clazz; long slot; HArrayOfObject *parameters;
    HArrayOfObject *exceptions; } ReflectConstructor;
typedef union { int32_t i; int64_t l; float f; double d; HObject *h; } ReflectValue;
extern ClassClass *FindPrimitiveClass(char *);

static HObject *reflect_error(char *type, char *detail)
{
    SignalError(0, type, detail);
    return NULL;
}

HObject *reflect_new_array(ClassClass *type, int length)
{
    int code = CCIs(type, Primitive) ? cbTypeCode(type) : T_CLASS;
    HObject *array = ArrayAlloc(code, length);
    if (array == NULL) return reflect_error(JAVAPKG "OutOfMemoryError", NULL);
    if (code == T_CLASS)
        ((ClassClass **)unhand((HArrayOfObject *)array)->body)[length] = type;
    return array;
}

HArrayOfObject *reflect_new_class_array(int length)
{
    return (HArrayOfObject *)reflect_new_array(classJavaLangClass, length);
}

long java_lang_Class_isArray(ClassClass *self)
{ return cbName(self)[0] == '['; }
long java_lang_Class_getModifiers(ClassClass *self)
{ return cbAccess(self) & 0xfff; }

ClassClass *reflect_find_class(char *signature, ClassClass *owner, char **next)
{
    char name[260], *p = signature, *out = name, *primitive = NULL;
    ClassClass *type;
    switch (*p++) {
    case 'V': primitive = "void"; break;
    case 'Z': primitive = "boolean"; break;
    case 'B': primitive = "byte"; break;
    case 'C': primitive = "char"; break;
    case 'S': primitive = "short"; break;
    case 'I': primitive = "int"; break;
    case 'J': primitive = "long"; break;
    case 'F': primitive = "float"; break;
    case 'D': primitive = "double"; break;
    case 'L':
        while (*p != ';' && *p && out < name + 259) *out++ = *p++;
        if (*p == ';') p++;
        break;
    case '[':
        p = signature;
        while (*p == '[' && out < name + 259) *out++ = *p++;
        if (out < name + 259) *out++ = *p++;
        if (out[-1] == 'L') {
            while (*p && out < name + 259) {
                *out++ = *p++;
                if (out[-1] == ';') break;
            }
        }
        break;
    default:
        reflect_error(JAVAPKG "NoClassDefFoundError", signature); return NULL;
    }
    if (out >= name + 259) {
        reflect_error(JAVAPKG "InternalError", "signature overflow"); return NULL;
    }
    *out = 0;
    if (primitive) type = FindPrimitiveClass(primitive);
    else {
        for (out = name; *out; out++) if (*out == '.') *out = '/';
        type = FindClassFromClass(0, name, FALSE, owner);
    }
    if (next) *next = p;
    return type;
}

ClassClass *java_lang_Class_getComponentType(ClassClass *self)
{
    ClassClass *cb = self, *component;
    if (cbName(cb)[0] != '[') return NULL;
    component = reflect_find_class(cbName(cb) + 1, cb, NULL);
    return component ? cbHandle(component) : NULL;
}

static HArrayOfObject *reflect_parameters(char *sig, ClassClass *owner, char **end)
{
    char *p = sig + 1;
    int count = 0, i;
    HArrayOfObject *array;
    ClassClass *type;
    while (*p != ')') {
        if (!reflect_find_class(p, owner, &p)) return NULL;
        count++;
    }
    array = reflect_new_class_array(count);
    if (!array) return NULL;
    p = sig + 1;
    for (i = 0; i < count; i++) {
        type = reflect_find_class(p, owner, &p);
        if (!type) return NULL;
        unhand(array)->body[i] = (HObject *)cbHandle(type);
    }
    if (end) *end = p + 1;
    return array;
}

static HArrayOfObject *reflect_exceptions(struct methodblock *mb)
{
    HArrayOfObject *array = reflect_new_class_array(mb->nexceptions);
    int i;
    ClassClass *owner = mb->fb.clazz;
    if (!array) return NULL;
    for (i = 0; i < mb->nexceptions; i++) {
        if (!ResolveClassConstant(cbConstantPool(owner), mb->exceptions[i], EE(),
                                  1 << CONSTANT_Class)) return NULL;
        unhand(array)->body[i] = (HObject *)cbHandle((ClassClass *)
            cbConstantPool(owner)[mb->exceptions[i]].p);
    }
    return array;
}

static HObject *reflect_make_field(struct fieldblock *fb)
{
    ClassClass *type = reflect_find_class(fb->signature, fb->clazz, NULL);
    HString *name;
    ClassClass *clazz;
    HObject *object;
    ReflectField *field;
    if (!type) return NULL;
    name = makeJavaString(fb->name, strlen(fb->name));
    if (!name) return NULL;
    clazz = FindClass(0, JAVAPKG "reflect/Field", TRUE);
    if (!clazz) return NULL;
    object = ObjAlloc(clazz, 0);
    if (!object) return reflect_error(JAVAPKG "OutOfMemoryError", NULL);
    field = (ReflectField *)unhand(object);
    field->clazz = cbHandle(fb->clazz); field->slot = fb - cbFields(fb->clazz);
    field->name = name; field->type = cbHandle(type);
    return object;
}

static HObject *reflect_make_method(struct methodblock *mb, int constructor)
{
    char *end;
    HArrayOfObject *parameters = reflect_parameters(mb->fb.signature, mb->fb.clazz, &end);
    HArrayOfObject *exceptions;
    ClassClass *type, *clazz;
    HString *name;
    HObject *object;
    ReflectMethod *method;
    ReflectConstructor *ctor;
    if (!parameters) return NULL;
    type = reflect_find_class(end, mb->fb.clazz, NULL);
    if (!type) return NULL;
    exceptions = reflect_exceptions(mb);
    if (!exceptions) return NULL;
    name = makeJavaString(mb->fb.name, strlen(mb->fb.name));
    if (!name) return NULL;
    clazz = FindClass(0, constructor ? JAVAPKG "reflect/Constructor" : JAVAPKG "reflect/Method", TRUE);
    if (!clazz) return NULL;
    object = ObjAlloc(clazz, 0);
    if (!object) return reflect_error(JAVAPKG "OutOfMemoryError", NULL);
    if (constructor) {
        ctor = (ReflectConstructor *)unhand(object);
        ctor->clazz = cbHandle(mb->fb.clazz); ctor->slot = mb - cbMethods(mb->fb.clazz);
        ctor->parameters = parameters; ctor->exceptions = exceptions;
    } else {
        method = (ReflectMethod *)unhand(object);
        method->clazz = cbHandle(mb->fb.clazz); method->slot = mb - cbMethods(mb->fb.clazz);
        method->name = name; method->type = cbHandle(type);
        method->parameters = parameters; method->exceptions = exceptions;
    }
    return object;
}

static int reflect_resolve(ClassClass *cb)
{
    char *detail = NULL, *error;
    if (CCIs(cb, Resolved)) return TRUE;
    error = ResolveClass(cb, &detail);
    if (!error) return TRUE;
    SignalError(0, error, detail); return FALSE;
}

bool_t reflect_is_assignable(ClassClass *from, ClassClass *to, ExecEnv *ee)
{
    cp_item_type *source, *destination;
    if (CCIs(from, Primitive) || CCIs(to, Primitive)) return from == to;
    if (cbName(from)[0] == '[') {
        if (cbName(to)[0] == '[') {
            source = cbConstantPool(from); destination = cbConstantPool(to);
            if (source[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i != destination[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i
                || source[CONSTANT_POOL_ARRAY_TYPE_INDEX].i != destination[CONSTANT_POOL_ARRAY_TYPE_INDEX].i) return FALSE;
            if (source[CONSTANT_POOL_ARRAY_TYPE_INDEX].i != T_CLASS) return TRUE;
            if (!ResolveClassConstantFromClass(from, CONSTANT_POOL_ARRAY_CLASS_INDEX, ee, 1 << CONSTANT_Class)
                || !ResolveClassConstantFromClass(to, CONSTANT_POOL_ARRAY_CLASS_INDEX, ee, 1 << CONSTANT_Class)) return FALSE;
            return reflect_is_assignable(source[CONSTANT_POOL_ARRAY_CLASS_INDEX].p,
                destination[CONSTANT_POOL_ARRAY_CLASS_INDEX].p, ee);
        }
        return to == classJavaLangObject || to == interfaceJavaLangCloneable
            || to == FindClass(ee, "java/io/Serializable", TRUE);
    }
    return reflect_resolve(from) && is_subclass_of(from, to, ee);
}
long java_lang_Class_isAssignableFrom(ClassClass *self, ClassClass *from)
{
    if (!from) { reflect_error(JAVAPKG "NullPointerException", NULL); return FALSE; }
    return reflect_is_assignable(from, self, EE());
}
long java_lang_Class_isInstance(ClassClass *self, HObject *object)
{ return object && !CCIs(self, Primitive) && is_instance_of(object, self, EE()); }

/* deviation: recover the interface closure from current constant pools rather
 * than Sun 1.1's imethodtable, which the current ClassClass does not contain. */
static int reflect_add_interface(ClassClass *cb, ClassClass ***list, int *count)
{
    int i;
    ClassClass **expanded, *parent;
    for (i = 0; i < *count; i++) if ((*list)[i] == cb) return TRUE;
    if (cbIsInterface(cb)) {
        expanded = sysRealloc(*list, (*count + 1) * sizeof(ClassClass *));
        if (!expanded) { reflect_error(JAVAPKG "OutOfMemoryError", NULL); return FALSE; }
        *list = expanded; (*list)[(*count)++] = cb;
    }
    for (i = 0; i < cbImplementsCount(cb); i++) {
        if (!ResolveClassConstant(cbConstantPool(cb), cbImplements(cb)[i], EE(), 1 << CONSTANT_Class)) return FALSE;
        parent = cbConstantPool(cb)[cbImplements(cb)[i]].p;
        if (!reflect_add_interface(parent, list, count)) return FALSE;
    }
    if (!cbIsInterface(cb) && cbSuperclass(cb))
        return reflect_add_interface(cbSuperclass(cb), list, count);
    return TRUE;
}

static HArrayOfObject *reflect_public_members(ClassClass *cb, ClassClass *memberclass, int kind)
{
    ClassClass **interfaces = NULL, *cursor;
    int ninterfaces = 0, count = 0, pass, i, j, index, total;
    struct fieldblock **slots = NULL, *fb;
    struct methodblock *mb;
    HArrayOfObject *array = NULL;
    HObject *member;
    if (kind == 0 || cbIsInterface(cb))
        if (!reflect_add_interface(cb, &interfaces, &ninterfaces)) goto failed;
    if (kind == 0 && !cbIsInterface(cb)) {
        if (!makeslottable(cb)) { reflect_error(JAVAPKG "OutOfMemoryError", NULL); goto failed; }
        slots = cbSlotTable(cb);
    }
    for (pass = 0; pass < 2; pass++) {
        index = count;
        if (!cbIsInterface(cb)) {
            total = kind == 0 ? cbSlotTableSize(cb) : cbMethodTableSize(cb);
            for (i = total - 1; i >= (kind == 0 ? 0 : 1); i--) {
                mb = kind == 1 ? cbMethodTable(cb)->methods[i] : NULL;
                fb = kind == 0 ? slots[i] : &mb->fb;
                if (!(fb->access & ACC_PUBLIC) || (kind == 1 && fb->name[0] == '<')) continue;
                if (!pass) count++;
                else {
                    member = kind == 0 ? reflect_make_field(fb) : reflect_make_method(mb, 0);
                    if (!member) goto failed;
                    unhand(array)->body[--index] = member;
                }
            }
            if (kind == 1) {
                for (cursor = cb; cursor; cursor = cbSuperclass(cursor) ? cbSuperclass(cursor) : NULL)
                    for (i = cbMethodsCount(cursor) - 1; i >= 0; i--) {
                        mb = &cbMethods(cursor)[i];
                        if ((mb->fb.access & (ACC_PUBLIC | ACC_STATIC)) != (ACC_PUBLIC | ACC_STATIC)
                            || mb->fb.name[0] == '<') continue;
                        if (!pass) count++;
                        else {
                            member = reflect_make_method(mb, 0);
                            if (!member) goto failed;
                            unhand(array)->body[--index] = member;
                        }
                    }
            }
        }
        for (j = 0; j < ninterfaces; j++) {
            cursor = interfaces[j];
            total = kind == 0 ? cbFieldsCount(cursor) : cbMethodsCount(cursor);
            for (i = total - 1; i >= 0; i--) {
                if (!pass) count++;
                else {
                    member = kind == 0 ? reflect_make_field(&cbFields(cursor)[i]) :
                        reflect_make_method(&cbMethods(cursor)[i], 0);
                    if (!member) goto failed;
                    unhand(array)->body[--index] = member;
                }
            }
        }
        if (!pass) {
            array = (HArrayOfObject *)reflect_new_array(memberclass, count);
            if (!array) goto failed;
        }
    }
    sysFree(interfaces); return array;
failed:
    sysFree(interfaces); return NULL;
}

/* Declared members follow class-file order. */
static HArrayOfObject *reflect_members(ClassClass *cb, long which, int kind)
{
    ClassClass *cursor, *memberclass;
    int pass, count = 0, index = 0, i, total, accept;
    struct fieldblock *fb;
    HArrayOfObject *array = NULL;
    HObject *member;
    if (!reflect_resolve(cb)) return NULL;
    memberclass = FindClass(0, kind == 0 ? JAVAPKG "reflect/Field" :
        kind == 1 ? JAVAPKG "reflect/Method" : JAVAPKG "reflect/Constructor", TRUE);
    if (!memberclass) return NULL;
    if (CCIs(cb, Primitive) || (kind != 1 && cbName(cb)[0] == '[')
        || (kind == 2 && cbIsInterface(cb)))
        return (HArrayOfObject *)reflect_new_array(memberclass, 0);
    if (!which && kind != 2) return reflect_public_members(cb, memberclass, kind);
    for (pass = 0; pass < 2; pass++) {
        for (cursor = cb; cursor; cursor = (which || kind == 2 || !cbSuperclass(cursor))
                ? NULL : cbSuperclass(cursor)) {
            total = kind == 0 ? cbFieldsCount(cursor) : cbMethodsCount(cursor);
            for (i = 0; i < total; i++) {
                fb = kind == 0 ? &cbFields(cursor)[i] : &cbMethods(cursor)[i].fb;
                accept = which || (fb->access & ACC_PUBLIC);
                if (kind == 1 && fb->name[0] == '<') accept = 0;
                if (kind == 2 && strcmp(fb->name, "<init>")) accept = 0;
                if (!accept) continue;
                if (!pass) count++;
                else {
                    member = kind == 0 ? reflect_make_field(fb) :
                        reflect_make_method(&cbMethods(cursor)[i], kind == 2);
                    if (!member) return NULL;
                    unhand(array)->body[index++] = member;
                }
            }
        }
        if (!pass) {
            array = (HArrayOfObject *)reflect_new_array(memberclass, count);
            if (!array) return NULL;
        }
    }
    return array;
}

HArrayOfObject *java_lang_Class_getFields0(ClassClass *self, long which)
{ return reflect_members(self, which, 0); }
HArrayOfObject *java_lang_Class_getMethods0(ClassClass *self, long which)
{ return reflect_members(self, which, 1); }
HArrayOfObject *java_lang_Class_getConstructors0(ClassClass *self, long which)
{ return reflect_members(self, which, 2); }

static int reflect_match_parameters(struct methodblock *mb, HArrayOfObject *args)
{
    char *p = mb->fb.signature + 1;
    int i = 0, count = args ? obj_length(args) : 0;
    ClassClass *type;
    while (*p != ')') {
        type = reflect_find_class(p, mb->fb.clazz, &p);
        if (!type || i >= count || (HObject *)cbHandle(type) != unhand(args)->body[i++]) return FALSE;
    }
    return i == count;
}

HObject *java_lang_Class_getField0(ClassClass *self, HString *name, long which)
{
    ClassClass *cb = self;
    char buffer[256]; int i;
    HArrayOfObject *publicfields;
    ReflectField *field;
    if (!name) return reflect_error(JAVAPKG "NullPointerException", NULL);
    javaString2CString(name, buffer, sizeof(buffer));
    if (!which) {
        publicfields = reflect_members(cb, 0, 0);
        if (!publicfields) return NULL;
        for (i = 0; i < (int)obj_length(publicfields); i++) {
            field = (ReflectField *)unhand(unhand(publicfields)->body[i]);
            if (!strcmp(cbFields(field->clazz)[field->slot].name, buffer))
                return unhand(publicfields)->body[i];
        }
        return reflect_error(JAVAPKG "NoSuchFieldException", buffer);
    }
    for (; cb; cb = which || !cbSuperclass(cb) ? NULL : cbSuperclass(cb)) {
        if (!reflect_resolve(cb)) return NULL;
        for (i = 0; i < cbFieldsCount(cb); i++)
            if (!strcmp(cbFields(cb)[i].name, buffer) && (which || (cbFields(cb)[i].access & ACC_PUBLIC)))
                return reflect_make_field(&cbFields(cb)[i]);
    }
    return reflect_error(JAVAPKG "NoSuchFieldException", buffer);
}

static HObject *reflect_find_method(ClassClass *self, char *name, HArrayOfObject *args, long which, int ctor)
{
    ClassClass *cb = self; int i;
    struct methodblock *mb;
    HArrayOfObject *publicmethods;
    ReflectMethod *member;
    if (!which && !ctor) {
        publicmethods = reflect_members(cb, 0, 1);
        if (!publicmethods) return NULL;
        for (i = 0; i < (int)obj_length(publicmethods); i++) {
            member = (ReflectMethod *)unhand(unhand(publicmethods)->body[i]);
            mb = &cbMethods(member->clazz)[member->slot];
            if (!strcmp(mb->fb.name, name) && reflect_match_parameters(mb, args))
                return unhand(publicmethods)->body[i];
        }
        return reflect_error(JAVAPKG "NoSuchMethodException", name);
    }
    for (; cb; cb = which || ctor || !cbSuperclass(cb) ? NULL : cbSuperclass(cb)) {
        if (!reflect_resolve(cb)) return NULL;
        for (i = 0; i < cbMethodsCount(cb); i++) {
            mb = &cbMethods(cb)[i];
            if (!strcmp(mb->fb.name, name) && (which || (mb->fb.access & ACC_PUBLIC))
                && reflect_match_parameters(mb, args)) return reflect_make_method(mb, ctor);
        }
    }
    return reflect_error(JAVAPKG "NoSuchMethodException", name);
}
HObject *java_lang_Class_getMethod0(ClassClass *self, HString *name, HArrayOfObject *args, long which)
{
    char buffer[256];
    if (!name) return reflect_error(JAVAPKG "NullPointerException", NULL);
    javaString2CString(name, buffer, sizeof(buffer));
    return reflect_find_method(self, buffer, args, which, 0);
}
HObject *java_lang_Class_getConstructor0(ClassClass *self, HArrayOfObject *args, long which)
{ return reflect_find_method(self, "<init>", args, which, 1); }

static struct methodblock *reflect_method_block(HObject *self)
{
    ReflectMethod *m = (ReflectMethod *)unhand(self);
    ClassClass *cb = m->clazz;
    if (m->slot < 0 || m->slot >= cbMethodsCount(cb)) return NULL;
    return &cbMethods(cb)[m->slot];
}
long java_lang_reflect_Field_getModifiers(HObject *self)
{
    ReflectField *f = (ReflectField *)unhand(self);
    ClassClass *cb = f->clazz;
    return f->slot < 0 || f->slot >= cbFieldsCount(cb) ? 0 : cbFields(cb)[f->slot].access & 0xfff;
}
long java_lang_reflect_Method_getModifiers(HObject *self)
{ struct methodblock *mb = reflect_method_block(self); return mb ? mb->fb.access & 0xfff : 0; }
long java_lang_reflect_Constructor_getModifiers(HObject *self)
{ return java_lang_reflect_Method_getModifiers(self); }

bool_t reflect_check_access(ExecEnv *ee, ClassClass *owner, int access, ClassClass *target)
{
    JavaFrame *frame;
    ClassClass *caller = NULL;
    if (!ee) ee = EE();
    for (frame = ee->current_frame; frame; frame = frame->prev) {
        if (frame->current_method) { caller = frame->current_method->fb.clazz; break; }
    }
    if ((caller == owner || (VerifyClassAccess(caller, owner, FALSE)
            && VerifyFieldAccess(caller, owner, access, FALSE)))
        && (!(access & ACC_PROTECTED) || caller == target
            || IsSameClassPackage(caller, owner) || is_subclass_of(target, caller, ee))) return TRUE;
    reflect_error(JAVAPKG "IllegalAccessException", cbName(owner));
    return FALSE;
}

static int reflect_widen(ReflectValue *v, int from, int to)
{
    int integral = from == T_BYTE || from == T_CHAR || from == T_SHORT || from == T_INT;
    if (from == to) return TRUE;
    switch (to) {
    case T_SHORT: return from == T_BYTE;
    case T_INT: return from == T_BYTE || from == T_CHAR || from == T_SHORT;
    case T_LONG: if (integral) { v->l = int2ll(v->i); return TRUE; } break;
    case T_FLOAT:
        if (integral) { v->f = (float)v->i; return TRUE; }
        if (from == T_LONG) { v->f = (float)ll2float(v->l); return TRUE; } break;
    case T_DOUBLE:
        if (integral) { v->d = (double)v->i; return TRUE; }
        if (from == T_LONG) { v->d = ll2double(v->l); return TRUE; }
        if (from == T_FLOAT) { v->d = (double)v->f; return TRUE; } break;
    }
    return FALSE;
}

static ReflectValue reflect_read(void *p, int type, int packed)
{
    ReflectValue v; Java8 tmp;
    memset(&v, 0, sizeof(v));
    switch (type) {
    case T_BOOLEAN: case T_BYTE: if (packed) { v.i = *(signed char *)p; break; }
    case T_CHAR: if (packed) { v.i = *(unsigned short *)p; break; }
    case T_SHORT: if (packed) { v.i = *(short *)p; break; }
    case T_INT: v.i = *(int32_t *)p; break;
    case T_FLOAT: v.f = *(float *)p; break;
    case T_LONG: v.l = GET_INT64(tmp, p); break;
    case T_DOUBLE: v.d = GET_DOUBLE(tmp, p); break;
    default: v.h = *(HObject **)p;
    }
    return v;
}
static void reflect_write(void *p, int type, int packed, ReflectValue v)
{
    Java8 tmp;
    switch (type) {
    case T_BOOLEAN: case T_BYTE: if (packed) { *(signed char *)p = (signed char)v.i; break; }
    case T_CHAR: if (packed) { *(unsigned short *)p = (unsigned short)v.i; break; }
    case T_SHORT: if (packed) { *(short *)p = (short)v.i; break; }
    case T_INT: *(int32_t *)p = v.i; break;
    case T_FLOAT: *(float *)p = v.f; break;
    case T_LONG: SET_INT64(tmp, p, v.l); break;
    case T_DOUBLE: SET_DOUBLE(tmp, p, v.d); break;
    default: *(HObject **)p = v.h;
    }
}

static char *reflect_wrapper(int type)
{
    switch (type) {
    case T_BOOLEAN: return JAVAPKG "Boolean";
    case T_BYTE: return JAVAPKG "Byte";
    case T_CHAR: return JAVAPKG "Character";
    case T_SHORT: return JAVAPKG "Short";
    case T_INT: return JAVAPKG "Integer";
    case T_LONG: return JAVAPKG "Long";
    case T_FLOAT: return JAVAPKG "Float";
    case T_DOUBLE: return JAVAPKG "Double";
    }
    return NULL;
}
static HObject *reflect_box(ReflectValue v, int type)
{
    ClassClass *cb;
    HObject *object; int i;
    struct fieldblock *fb;
    if (type == T_VOID) return NULL;
    if (type == T_CLASS) return v.h;
    cb = FindClass(0, reflect_wrapper(type), TRUE);
    if (!cb) return NULL;
    object = ObjAlloc(cb, 0);
    if (!object) return reflect_error(JAVAPKG "OutOfMemoryError", NULL);
    for (i = 0, fb = cbFields(cb); i < cbFieldsCount(cb); i++, fb++)
        if (!strcmp(fb->name, "value")) {
            reflect_write((char *)unhand(object) + fb->u.offset, type, FALSE, v);
            return object;
        }
    return reflect_error(JAVAPKG "InternalError", "primitive wrapper value");
}
static int reflect_unbox(HObject *object, ReflectValue *value)
{
    ClassClass *cb; int type, i; char *name;
    struct fieldblock *fb;
    if (!object || obj_flags(object) != T_NORMAL_OBJECT) return 0;
    cb = obj_classblock(object);
    for (type = T_BOOLEAN; type <= T_LONG; type++) {
        name = reflect_wrapper(type);
        if (name && !strcmp(cbName(cb), name)) {
            for (i = 0, fb = cbFields(cb); i < cbFieldsCount(cb); i++, fb++)
                if (!strcmp(fb->name, "value")) {
                    *value = reflect_read((char *)unhand(object) + fb->u.offset, type, FALSE);
                    return type;
                }
        }
    }
    return 0;
}

static struct fieldblock *reflect_field_address(HObject *self, HObject *target, void **address, ClassClass **type)
{
    ReflectField *field = (ReflectField *)unhand(self);
    ClassClass *owner = field->clazz, *targetclass;
    struct fieldblock *fb;
    if (field->slot < 0 || field->slot >= cbFieldsCount(owner)) return NULL;
    fb = &cbFields(owner)[field->slot]; *type = field->type;
    if (fb->access & ACC_STATIC) {
        *address = CCIs(*type, Primitive) && cbSlotSize(*type) == 8
            ? twoword_static_address(fb) : normal_static_address(fb);
        targetclass = owner;
    } else {
        if (!target) { reflect_error(JAVAPKG "NullPointerException", NULL); return NULL; }
        targetclass = obj_flags(target) ? classJavaLangObject : obj_classblock(target);
        if (targetclass != owner && !is_instance_of(target, owner, EE())) {
            reflect_error(JAVAPKG "IllegalArgumentException", "object is not an instance of declaring class"); return NULL;
        }
        *address = (char *)unhand(target) + fb->u.offset;
    }
    if ((!(cbAccess(owner) & ACC_PUBLIC) || !(fb->access & ACC_PUBLIC))
        && !reflect_check_access(EE(), owner, fb->access, targetclass)) return NULL;
    return fb;
}

static ReflectValue reflect_field_get(HObject *self, HObject *target, int requested)
{
    ReflectValue v; void *address; ClassClass *type; int code;
    memset(&v, 0, sizeof(v));
    if (!reflect_field_address(self, target, &address, &type)) return v;
    code = CCIs(type, Primitive) ? cbTypeCode(type) : T_CLASS;
    v = reflect_read(address, code, FALSE);
    if (requested && !reflect_widen(&v, code, requested)) {
        reflect_error(JAVAPKG "IllegalArgumentException", "field type mismatch");
        memset(&v, 0, sizeof(v));
    }
    return v;
}
HObject *java_lang_reflect_Field_get(HObject *self, HObject *target)
{
    ReflectField *field = (ReflectField *)unhand(self);
    ClassClass *type = field->type;
    ReflectValue v = reflect_field_get(self, target, 0);
    if (exceptionOccurred(EE())) return NULL;
    return reflect_box(v, CCIs(type, Primitive) ? cbTypeCode(type) : T_CLASS);
}
static void reflect_field_set(HObject *self, HObject *target, ReflectValue v, int from)
{
    void *address; ClassClass *type; int to;
    struct fieldblock *fb = reflect_field_address(self, target, &address, &type);
    if (!fb) return;
    if (fb->access & ACC_FINAL) { reflect_error(JAVAPKG "IllegalAccessException", "field is final"); return; }
    to = CCIs(type, Primitive) ? cbTypeCode(type) : T_CLASS;
    if (from == T_CLASS && to != T_CLASS) from = reflect_unbox(v.h, &v);
    if (from == T_CLASS && to == T_CLASS) {
        if (v.h && !is_instance_of(v.h, type, EE())) {
            reflect_error(JAVAPKG "IllegalArgumentException", "field type mismatch"); return;
        }
    } else if (!from || !reflect_widen(&v, from, to)) {
        reflect_error(JAVAPKG "IllegalArgumentException", "field type mismatch"); return;
    }
    reflect_write(address, to, FALSE, v);
}
void java_lang_reflect_Field_set(HObject *self, HObject *target, HObject *value)
{ ReflectValue v; v.h = value; reflect_field_set(self, target, v, T_CLASS); }

#define FIELD_ACCESSORS(Name, CType, member, code) \
CType java_lang_reflect_Field_get##Name(HObject *self, HObject *target) \
{ ReflectValue v = reflect_field_get(self, target, code); return v.member; } \
void java_lang_reflect_Field_set##Name(HObject *self, HObject *target, CType value) \
{ ReflectValue v; v.member = value; reflect_field_set(self, target, v, code); }
FIELD_ACCESSORS(Boolean, long, i, T_BOOLEAN)
FIELD_ACCESSORS(Byte, long, i, T_BYTE)
FIELD_ACCESSORS(Char, long, i, T_CHAR)
FIELD_ACCESSORS(Short, long, i, T_SHORT)
FIELD_ACCESSORS(Int, long, i, T_INT)
FIELD_ACCESSORS(Long, int64_t, l, T_LONG)
FIELD_ACCESSORS(Float, float, f, T_FLOAT)
FIELD_ACCESSORS(Double, double, d, T_DOUBLE)

static void *reflect_array_address(HObject *array, long index, int *type)
{
    int size;
    if (!array) { reflect_error(JAVAPKG "NullPointerException", NULL); return NULL; }
    *type = obj_flags(array);
    if (*type == T_NORMAL_OBJECT) {
        reflect_error(JAVAPKG "IllegalArgumentException", "argument is not an array"); return NULL;
    }
    if (index < 0 || index >= (long)obj_length(array)) {
        reflect_error(JAVAPKG "ArrayIndexOutOfBoundsException", NULL); return NULL;
    }
    size = *type == T_CLASS ? sizeof(HObject *) : 1 << (*type & 3);
    return (char *)unhand(array) + index * size;
}
long java_lang_reflect_Array_getLength(HObject *unused, HObject *array)
{
    if (!array) { reflect_error(JAVAPKG "NullPointerException", NULL); return 0; }
    if (!obj_flags(array)) { reflect_error(JAVAPKG "IllegalArgumentException", "argument is not an array"); return 0; }
    return obj_length(array);
}
static ReflectValue reflect_array_get(HObject *array, long index, int requested)
{
    ReflectValue v; void *address; int type;
    memset(&v, 0, sizeof(v));
    address = reflect_array_address(array, index, &type);
    if (!address) return v;
    v = reflect_read(address, type, TRUE);
    if (requested && !reflect_widen(&v, type, requested)) {
        reflect_error(JAVAPKG "IllegalArgumentException", "array element type mismatch");
        memset(&v, 0, sizeof(v));
    }
    return v;
}
HObject *java_lang_reflect_Array_get(HObject *unused, HObject *array, long index)
{
    ReflectValue v = reflect_array_get(array, index, 0);
    if (exceptionOccurred(EE())) return NULL;
    return reflect_box(v, obj_flags(array));
}
static void reflect_array_set(HObject *array, long index, ReflectValue v, int from)
{
    void *address; int to; ClassClass *component;
    address = reflect_array_address(array, index, &to);
    if (!address) return;
    if (from == T_CLASS && to != T_CLASS) from = reflect_unbox(v.h, &v);
    if (from == T_CLASS && to == T_CLASS) {
        component = ((ClassClass **)unhand((HArrayOfObject *)array)->body)[obj_length(array)];
        if (v.h && !is_instance_of(v.h, component, EE())) {
            reflect_error(JAVAPKG "IllegalArgumentException", "array element type mismatch"); return;
        }
    } else if (!from || !reflect_widen(&v, from, to)) {
        reflect_error(JAVAPKG "IllegalArgumentException", "array element type mismatch"); return;
    }
    reflect_write(address, to, TRUE, v);
}
void java_lang_reflect_Array_set(HObject *unused, HObject *array, long index, HObject *value)
{ ReflectValue v; v.h = value; reflect_array_set(array, index, v, T_CLASS); }
#define ARRAY_ACCESSORS(Name, CType, member, code) \
CType java_lang_reflect_Array_get##Name(HObject *unused, HObject *array, long index) \
{ ReflectValue v = reflect_array_get(array, index, code); return v.member; } \
void java_lang_reflect_Array_set##Name(HObject *unused, HObject *array, long index, CType value) \
{ ReflectValue v; v.member = value; reflect_array_set(array, index, v, code); }
ARRAY_ACCESSORS(Boolean, long, i, T_BOOLEAN)
ARRAY_ACCESSORS(Byte, long, i, T_BYTE)
ARRAY_ACCESSORS(Char, long, i, T_CHAR)
ARRAY_ACCESSORS(Short, long, i, T_SHORT)
ARRAY_ACCESSORS(Int, long, i, T_INT)
ARRAY_ACCESSORS(Long, int64_t, l, T_LONG)
ARRAY_ACCESSORS(Float, float, f, T_FLOAT)
ARRAY_ACCESSORS(Double, double, d, T_DOUBLE)

HObject *java_lang_reflect_Array_newArray(HObject *unused, ClassClass *component, long length)
{
    if (!component) return reflect_error(JAVAPKG "NullPointerException", NULL);
    if (length < 0) return reflect_error(JAVAPKG "NegativeArraySizeException", NULL);
    return reflect_new_array(component, length);
}
HObject *java_lang_reflect_Array_multiNewArray(HObject *unused, ClassClass *component, HArrayOfInt *dimensions)
{
    char signature[268], *p = signature, *name;
    stack_item sizes[255]; int count, i;
    ClassClass *cb, *arrayclass;
    if (!component || !dimensions) return reflect_error(JAVAPKG "NullPointerException", NULL);
    count = obj_length(dimensions);
    if (!count) return reflect_error(JAVAPKG "IllegalArgumentException", "zero dimensions");
    if (count >= 255) return reflect_error(JAVAPKG "IllegalArgumentException", "too many dimensions");
    for (i = 0; i < count; i++) {
        sizes[i].i = unhand(dimensions)->body[i];
        if (sizes[i].i < 0) return reflect_error(JAVAPKG "NegativeArraySizeException", NULL);
        *p++ = '[';
    }
    cb = component;
    if (CCIs(cb, Primitive)) *p++ = cbTypeSig(cb);
    else {
        name = cbName(cb);
        if (strlen(name) + count >= 260) return reflect_error(JAVAPKG "InternalError", "signature overflow");
        if (*name != '[') *p++ = 'L';
        strcpy(p, name); p += strlen(name);
        if (*name != '[') *p++ = ';';
    }
    *p = 0;
    arrayclass = FindClassFromClass(0, signature, TRUE, cb);
    if (!arrayclass) return NULL;
    return MultiArrayAlloc(count, arrayclass, sizes);
}

/* 100345C3: prepare a pseudo-frame in the VM's Java stack, so its parameters
 * remain visible to GC. This maps Sun's frame fields to the current layout. */
static JavaFrame *reflect_frame(ExecEnv *ee, int nargs)
{
    JavaFrame *previous = ee->current_frame, *frame;
    JavaStack *stack;
    if (nargs < 2) nargs = 2;
    if (!previous) {
        stack = ee->initial_stack; frame = (JavaFrame *)stack->data;
    } else {
        stack = previous->javastack;
        frame = previous->current_method
            ? (JavaFrame *)&previous->ostack[previous->current_method->maxstack]
            : (JavaFrame *)(previous->optop + 3);
        if (frame->ostack + nargs >= stack->end_data) {
            stack = stack->next ? stack->next : CreateNewJavaStack(ee, stack);
            if (!stack) { reflect_error(JAVAPKG "OutOfMemoryError", NULL); return NULL; }
            frame = (JavaFrame *)stack->data;
        }
    }
    frame->prev = previous; frame->javastack = stack; frame->optop = frame->ostack;
    frame->vars = NULL; frame->monitor = 0; frame->current_method = NULL;
    ee->current_frame = frame;
    return frame;
}

static HObject *reflect_invoke(HObject *target, struct methodblock *mb,
    HArrayOfObject *parameters, HArrayOfObject *args, ClassClass *returntype)
{
    ExecEnv *ee = EE();
    JavaFrame *previous = ee->current_frame, *frame;
    int i, count = obj_length(parameters), from, to, isstatic = mb->fb.access & ACC_STATIC;
    ReflectValue value;
    ClassClass *type;
    HObject *argument, *result = NULL, *thrown;
    Java8 tmp;
    cp_item_type pool[2]; unsigned char tags[2], code[4];
    if (count != (args ? (int)obj_length(args) : 0))
        return reflect_error(JAVAPKG "IllegalArgumentException", "wrong number of arguments");
    frame = reflect_frame(ee, mb->args_size);
    if (!frame) return NULL;
    if (!isstatic) (frame->optop++)->h = target;
    for (i = 0; i < count; i++) {
        type = (ClassClass *)unhand(parameters)->body[i];
        argument = unhand(args)->body[i];
        if (CCIs(type, Primitive)) {
            to = cbTypeCode(type); from = reflect_unbox(argument, &value);
            if (!from || !reflect_widen(&value, from, to)) {
                reflect_error(JAVAPKG "IllegalArgumentException", "argument type mismatch"); goto done;
            }
            reflect_write(frame->optop, to, FALSE, value);
            frame->optop += cbSlotSize(type) / 4;
        } else {
            if (argument && !is_instance_of(argument, type, ee)) {
                reflect_error(JAVAPKG "IllegalArgumentException", "argument type mismatch"); goto done;
            }
            (frame->optop++)->h = argument;
        }
    }
    pool[0].p = tags; tags[0] = CONSTANT_POOL_ENTRY_RESOLVED;
    pool[1].p = mb; tags[1] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Methodref;
    code[0] = isstatic ? opc_invokestatic_quick : opc_invokenonvirtual_quick;
    code[1] = 0; code[2] = 1; code[3] = opc_return;
    frame->constant_pool = pool;
    ee->exceptionKind = EXCKIND_NONE;
    if (ExecuteJava(code, ee)) {
        to = CCIs(returntype, Primitive) ? cbTypeCode(returntype) : T_CLASS;
        if (to != T_VOID) {
            value = reflect_read(frame->optop - (to == T_LONG || to == T_DOUBLE ? 2 : 1), to, FALSE);
            result = reflect_box(value, to);
        }
    } else {
        thrown = ee->exception.exc;
        ee->exceptionKind = EXCKIND_NONE;
        SignalError(ee, JAVAPKG "reflect/InvocationTargetException", NULL);
        if (ee->exception.exc) {
            /* deviation: locate the Java target field instead of assuming Sun's
             * Throwable layout has placed it at byte offset eight. */
            ClassClass *errorclass = obj_classblock(ee->exception.exc);
            for (i = 0; i < cbFieldsCount(errorclass); i++)
                if (!strcmp(cbFields(errorclass)[i].name, "target"))
                    *(HObject **)((char *)unhand(ee->exception.exc) + cbFields(errorclass)[i].u.offset) = thrown;
        }
    }
done:
    ee->current_frame = previous;
    return result;
}

HObject *java_lang_reflect_Method_invoke(HObject *self, HObject *target, HArrayOfObject *args)
{
    ReflectMethod *method = (ReflectMethod *)unhand(self);
    struct methodblock *mb = reflect_method_block(self), *selected;
    ClassClass *owner = method->clazz, *targetclass;
    int i;
    if (!mb) return reflect_error(JAVAPKG "InternalError", "invoke");
    selected = mb;
    if (mb->fb.access & ACC_STATIC) targetclass = owner;
    else {
        if (!target) return reflect_error(JAVAPKG "NullPointerException", NULL);
        targetclass = obj_flags(target) ? classJavaLangObject : obj_classblock(target);
        if (targetclass != owner && !is_instance_of(target, owner, EE()))
            return reflect_error(JAVAPKG "IllegalArgumentException", "object is not an instance of declaring class");
        if (!(mb->fb.access & ACC_PRIVATE)) {
            for (i = 1; i < cbMethodTableSize(targetclass); i++) {
                struct methodblock *candidate = cbMethodTable(targetclass)->methods[i];
                if (candidate && !strcmp(candidate->fb.name, mb->fb.name)
                    && !strcmp(candidate->fb.signature, mb->fb.signature)) { selected = candidate; break; }
            }
        }
    }
    if ((!(cbAccess(owner) & ACC_PUBLIC) || !(mb->fb.access & ACC_PUBLIC))
        && !reflect_check_access(EE(), owner, mb->fb.access, targetclass)) return NULL;
    return reflect_invoke(target, selected, method->parameters, args, method->type);
}

HObject *java_lang_reflect_Constructor_newInstance(HObject *self, HArrayOfObject *args)
{
    ReflectConstructor *ctor = (ReflectConstructor *)unhand(self);
    struct methodblock *mb = reflect_method_block(self);
    ClassClass *owner = ctor->clazz;
    HObject *object;
    if (!mb) return reflect_error(JAVAPKG "InternalError", "newInstance");
    if (cbAccess(owner) & (ACC_INTERFACE | ACC_ABSTRACT))
        return reflect_error(JAVAPKG "InstantiationException", cbName(owner));
    if ((!(cbAccess(owner) & ACC_PUBLIC) || !(mb->fb.access & ACC_PUBLIC))
        && !reflect_check_access(EE(), owner, mb->fb.access, owner)) return NULL;
    object = newobject(owner, NULL, EE());
    if (!object) return NULL;
    reflect_invoke(object, mb, ctor->parameters, args, FindPrimitiveClass("void"));
    return exceptionOccurred(EE()) ? NULL : object;
}
