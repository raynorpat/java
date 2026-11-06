/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Reconstructed from javai_g.dll sysInvokeNative, 0x10053218.
 * Both supported targets use the 32-bit x86 stack argument ABI. */
#include "jni_impl.h"

/* Saving ESP accommodates Win32 stdcall and Unix cdecl native methods. */
#ifdef _MSC_VER
__declspec(naked) static void nativeCall(void *function, stack_item *args,
                                        int count, void *env, jobject clazz,
                                        int resultType, stack_item *result)
{
    __asm {
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        mov esi, [ebp+12]
        mov ecx, [ebp+16]
    copyargs:
        test ecx, ecx
        jz prefix
        dec ecx
        push dword ptr [esi+ecx*4]
        jmp copyargs
    prefix:
        cmp dword ptr [ebp+24], 0
        je environment
        push dword ptr [ebp+24]
    environment:
        push dword ptr [ebp+20]
        call dword ptr [ebp+8]
        lea esp, [ebp-12]
        mov edi, [ebp+32]
        mov ecx, [ebp+28]
        cmp ecx, 70
        je floatresult
        cmp ecx, 68
        je doubleresult
        mov [edi], eax
        cmp ecx, 74
        jne done
        mov [edi+4], edx
        jmp done
    floatresult:
        fstp dword ptr [edi]
        jmp done
    doubleresult:
        fstp qword ptr [edi]
    done:
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
#else
extern void jniNativeCall(void *, stack_item *, int, void *, jobject, int, stack_item *);
__asm__(
".text\n"
".type jniNativeCall,@function\n"
"jniNativeCall:\n"
"pushl %ebp\n movl %esp,%ebp\n pushl %ebx\n pushl %esi\n pushl %edi\n"
/* Contemporary i386 C compilers require 16-byte alignment before CALL. */
"movl 16(%ebp),%eax\n incl %eax\n cmpl $0,24(%ebp)\n je 7f\n incl %eax\n"
"7: shll $2,%eax\n movl %esp,%edx\n subl %eax,%edx\n andl $15,%edx\n subl %edx,%esp\n"
"movl 12(%ebp),%esi\n movl 16(%ebp),%ecx\n"
"1: testl %ecx,%ecx\n jz 2f\n decl %ecx\n pushl (%esi,%ecx,4)\n jmp 1b\n"
"2: cmpl $0,24(%ebp)\n je 3f\n pushl 24(%ebp)\n"
"3: pushl 20(%ebp)\n call *8(%ebp)\n leal -12(%ebp),%esp\n"
"movl 32(%ebp),%edi\n movl 28(%ebp),%ecx\n"
"cmpl $70,%ecx\n je 4f\n cmpl $68,%ecx\n je 5f\n"
"movl %eax,(%edi)\n cmpl $74,%ecx\n jne 6f\n movl %edx,4(%edi)\n jmp 6f\n"
"4: fstps (%edi)\n jmp 6f\n"
"5: fstpl (%edi)\n"
"6: popl %edi\n popl %esi\n popl %ebx\n popl %ebp\n ret\n"
".size jniNativeCall,.-jniNativeCall\n");
#define nativeCall jniNativeCall
#endif

long *sysInvokeNative(void *env, void *function, long *args,
                      char *resultType, int count, void *clazz)
{
    nativeCall(function, (stack_item *)args, count, env, clazz, *resultType, (stack_item *)args);
    return args + (*resultType == 'V' ? 0 : (*resultType == 'J' || *resultType == 'D') ? 2 : 1);
}
