/* DR2 serialize.m, donor 0x53d0bb58-0x53d0c39f. */
#import "coding.h"
#import "bridge-exceptions.h"
#include "invocation.h"
#include "constructor.h"

static ClassClass *coderOutputClass, *coderInputClass, *outputClass, *inputClass;
static struct methodblock *writeObjectMethod, *readObjectMethod;
static NSMapTable *coderToJava, *javaToCoder;

stack_item *Java_com_apple_yellow_foundation_0005fprivate_NSCoderOutputStream_writeByte_stub(stack_item *args, ExecEnv *env)
{
    NSCoder *coder = *(id *)unhand(args[0].h);
    unsigned char byte = args[1].i;
    [coder encodeValueOfObjCType:"C" at:&byte];
    return args;
}
stack_item *Java_com_apple_yellow_foundation_0005fprivate_NSCoderOutputStream_write_stub(stack_item *args, ExecEnv *env)
{
    NSCoder *coder = *(id *)unhand(args[0].h);
    unsigned char *bytes = (unsigned char *)unhand(args[1].h) + args[2].i;
    long count = args[3].i;
    void (*encode)(id, SEL, const char *, void *) = (void *)[coder methodForSelector:@selector(encodeValueOfObjCType:at:)];
    while (count-- > 0) {
        encode(coder, @selector(encodeValueOfObjCType:at:), "C", bytes++);
    }
    return args;
}
stack_item *Java_com_apple_yellow_foundation_0005fprivate_NSCoderInputStream_readByte_stub(stack_item *args, ExecEnv *env)
{
    NSCoder *coder = *(id *)unhand(args[0].h);
    unsigned char byte;
    [coder decodeValueOfObjCType:"C" at:&byte];
    args[0].i = byte;
    return args + 1;
}
stack_item *Java_com_apple_yellow_foundation_0005fprivate_NSCoderInputStream_read_stub(stack_item *args, ExecEnv *env)
{
    NSCoder *coder = *(id *)unhand(args[0].h);
    unsigned char *bytes = (unsigned char *)unhand(args[1].h) + args[2].i;
    long count = args[3].i;
    void (*decode)(id, SEL, const char *, void *) = (void *)[coder methodForSelector:@selector(decodeValueOfObjCType:at:)];
    while (count-- > 0) {
        decode(coder, @selector(decodeValueOfObjCType:at:), "C", bytes++);
    }
    args[0].i = args[3].i;
    return args + 1;
}
static void bindNative(ClassClass *clazz, char *name, char *signature, void *code)
{
    struct methodblock *method = _JAVAMethodLookup(EE(), clazz, name, signature, FALSE, FALSE);
    if (!method) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: can't find %s on %s", name, cbName(clazz)];
    method->code = code;
    method->invoker = invokeNativeMethod;
}
static ClassClass *codingClass(char *name, NSString *description)
{
    ClassClass *clazz = FindClass(EE(), name, TRUE);
    if (!clazz) {
        NSLog(description);
        [NSException raise:NSInternalInconsistencyException format:description];
    }
    return clazz;
}
void SerializeInitialization(void)
{
    outputClass = codingClass("java/io/ObjectOutputStream", @"ObjCJava: can't find java.io.ObjectOutputStream");
    writeObjectMethod = _JAVAMethodLookup(EE(), outputClass, "writeObject", NULL, TRUE, FALSE);
    if (!writeObjectMethod) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: can't find writeObject() method on java.io.ObjectOutputStream"];
    inputClass = codingClass("java/io/ObjectInputStream", @"ObjCJava: can't find java.io.ObjectInputStream");
    readObjectMethod = _JAVAMethodLookup(EE(), inputClass, "readObject", NULL, TRUE, FALSE);
    if (!readObjectMethod) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: can't find readObject() method on java.io.ObjectInputStream"];
    coderToJava = NSCreateMapTable(NSNonRetainedObjectMapKeyCallBacks, NSNonOwnedPointerMapValueCallBacks, 0);
    javaToCoder = NSCreateMapTable(NSNonOwnedPointerMapKeyCallBacks, NSNonRetainedObjectMapValueCallBacks, 0);
    coderOutputClass = codingClass("com/apple/yellow/foundation/_private/NSCoderOutputStream", @"ObjCJava: can't find com.apple.yellow.foundation._private.NSCoderOutputStream");
    coderInputClass = codingClass("com/apple/yellow/foundation/_private/NSCoderInputStream", @"ObjCJava: can't find com.apple.yellow.foundation._private.NSCoderInputStream");
    bindNative(coderOutputClass, "writeByte", "(I)V", Java_com_apple_yellow_foundation_0005fprivate_NSCoderOutputStream_writeByte_stub);
    bindNative(coderOutputClass, "write", "([BII)V", Java_com_apple_yellow_foundation_0005fprivate_NSCoderOutputStream_write_stub);
    bindNative(coderInputClass, "readByte", "()I", Java_com_apple_yellow_foundation_0005fprivate_NSCoderInputStream_readByte_stub);
    bindNative(coderInputClass, "read", "([BII)I", Java_com_apple_yellow_foundation_0005fprivate_NSCoderInputStream_read_stub);
}
static HObject *streamForCoder(NSCoder *coder, BOOL output)
{
    HObject *stream = NSMapGet(coderToJava, coder), *nativeStream;
    if (!stream) {
        nativeStream = bridgeNewJavaObject(PRIVILEGED_EE, output ? coderOutputClass : coderInputClass, "(I)", FALSE, (va_list)&coder);
        bridgeRaisePendingJavaException(EE());
        if (!nativeStream) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: can't instantiate com.apple.yellow.foundation._private.NSCoder%@Stream for coder 0x%x", output ? @"Output" : @"Input", coder];
        stream = JAVANewObject(EE(), output ? outputClass : inputClass,
            output ? "(Ljava/io/OutputStream;)" : "(Ljava/io/InputStream;)", FALSE, (va_list)&nativeStream);
        if (!stream) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: can't instantiate java.io.Object%@Stream for coder 0x%x", output ? @"Output" : @"Input", coder];
        NSRetainJavaObject(stream);
        NSMapInsertKnownAbsent(coderToJava, coder, stream);
        NSMapInsertKnownAbsent(javaToCoder, stream, coder);
    }
    return stream;
}
HObject *ObjectOutputStreamForCoder(NSCoder *coder) { return streamForCoder(coder, YES); }
HObject *ObjectInputStreamForCoder(NSCoder *coder) { return streamForCoder(coder, NO); }
void NSArchiveJavaObject(HObject *object, NSCoder *coder)
{
    HObject *stream = ObjectOutputStreamForCoder(coder);
    __JAVAMethodInvoke(EE(), stream, writeObjectMethod, FALSE, FALSE, (va_list)&object, NULL);
}
HObject *NSUnarchiveJavaObject(NSCoder *coder)
{
    HObject *stream = ObjectInputStreamForCoder(coder);
    return (HObject *)__JAVAMethodInvoke(EE(), stream, readObjectMethod, FALSE, FALSE, NULL, NULL);
}
NSCoder *CoderForObjectOutputStream(HObject *stream)
{
    NSCoder *coder = NSMapGet(javaToCoder, stream);
    if (!coder) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: sorry, serializing ObjC objects from Java is not supported, yet."];
    return coder;
}
NSCoder *CoderForObjectInputStream(HObject *stream)
{
    NSCoder *coder = NSMapGet(javaToCoder, stream);
    if (!coder) [NSException raise:NSInternalInconsistencyException format:@"ObjCJava: sorry, deserializing ObjC objects from Java is not supported, yet."];
    return coder;
}
void NSSerializeObjCObject(id object, HObject *stream) { [CoderForObjectOutputStream(stream) encodeObject:object]; }
id NSDeserializeObjCObject(HObject *stream) { return [CoderForObjectInputStream(stream) decodeObject]; }


