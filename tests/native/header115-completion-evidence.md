# Remaining small-header adoption audit

The 41 original shared headers were compared byte-for-byte against the source
include directory. This tranche adopts original jmath.h, decode.h, reflect.h,
selftest.h and wrap.h. The last three are restored declarations/macros, not a
claim that every declared function has been implemented. Existing reflection
helpers have compatible prototypes; reflect.c is unchanged. No placeholder
PrintPublicOnly/PrintInternalSigs or java_wrap/java_unwrap bodies were added.

The original jmath.h guard compiles with current MSVC and Linux glibc math.h.
The shared jmath_md.h and timeval_md.h fallbacks formerly shadowed platform
include directories. They now live under Unix, preserving the existing Unix
implementation. The Windows include directory contains the original timeval_md.h
(two long fields); no current runtime code uses timeval_t arithmetic. Declared
timeradd/timersub have no current callers and have not been fabricated.

Windows mutex_md.h was already byte-identical and remains unchanged. Windows
jmath_md.h uses original text with a marked two-macro adaptation: modern CRT
fmod/remainder preserve the existing port's operations. Original drem/ieeerem
wrapper bodies are not claimed recovered. byteorder_md.h uses the original
macros with a marked x86 selection adaptation for _M_IX86/_X86_; the legacy
build used x86. path_md.h restores the original '/' logical separator and
retains marked path-list/launcher adapters (Windows native separator '\\').

Isolated x86 MSVC and Linux C89 header probes compile and execute successfully:
byte swaps, JavaValue size8, fmod(-7,4)=-3, remainder(-7,4)=1, rint ties-to-even;
Windows timeval size8 and member access. MSVC compilation of actual math.c,
thread_md.c and threads.c succeeds. These are header/translation-unit checks;
the concurrent core class-layout migration still needs integrated VM gates.

At this audit, 36 of 41 shared headers are byte-identical. Remaining five:

- breakpoints.h is absent; breakpoints115.h supplies the active reconstructed
  debugger contract. Original header uses BREAKPTS and bool returns whereas
  release binary adapters currently use void. Migrate with debugger exports
  and DEBUG/BREAKPTS definitions together, preserving JNI setter decoration.
- debug.h retains marked JNI diagnostic routing and exported visibility.
- finalize.h, monitor.h and monitor_cache.h remain coupled to pending runtime
  migrations owned by the coordinating tasks.

Windows headers: seven exact (dirent, io_md, jni_md, monitor_md, mutex_md,
oobj_md, timeval_md), six adapted (byteorder_md, jmath_md, path_md, sysmacros_md,
threads_md, typedefs_md), gc_md absent in the Windows directory (current shared
collector header is owned by the collector migration).

## Design for remaining broad platform headers (not applied)

1. typedefs_md: restore original long/unsigned-long 32-bit typedef spelling and
   original long arithmetic helpers together with actual float2l/double2l,
   float2ll/double2ll and ll_shl bodies recovered from the binary. Current casts
   differ for NaN/overflow; replacing declarations alone would break linkage.
   Verify numeric boundary gates and pointer-type compile diagnostics. Retain
   host-required sys/stat includes, extra narrow integer types and callback/
   JAVA_INTERNAL_BUILD visibility declarations in an explicitly marked adapter.
   Never reintroduce blanket DLL exports: strict Sun export manifests remain
   authoritative. The original MSVC #@ operator and malformed NAN_CHECK macro
   require a documented modern-compiler adaptation when actually used.
2. sysmacros_md: original cache_lock_t/_moncache_lock and owner/cache macros
   must migrate with monitor-cache layout, locking and thread cache consumers.
   Do not replace the current lock macros in isolation. Preserve native stream
   adapters until original sysOpen/sysAccess/sysMkdir/sysUnlink wrappers exist;
   original header does not map those calls directly to CRT. Preserve current
   internal cleanup/collector declarations separately from exact sys_api.h.
   Original debug assertions call DumpThreads/panic rather than CRT assert;
   migrate with diagnostic dependencies and check Debug behavior.
3. threads_md: exact Sun prefix already present; appended register/stack/priority
   bookkeeping can be removed only after collector and thread-priority clients
   no longer use it. This is a real layout deviation, not formatting drift.
