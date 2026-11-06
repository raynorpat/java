# HPI header and thread/directory ABI alignment

`sys_api.h`, Windows `io_md.h`, and Windows `dirent.h` are byte-identical to
`vendor/jdk1.1.5/include` originals. Unix directory calls now use the host
`DIR`/`struct dirent` ABI. The old `sys_dir_t` names remain aliases in the
platform compatibility headers for existing consumers. Retained GC register,
thread-cookie setter and monitor cleanup declarations were moved out of the
original shared header into explicitly marked platform declarations.

Primary evidence: javai_g.dll, SHA256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.

- `sysThreadAlloc` 0x1004C770 allocates/zeros 84 bytes, assigns supplied cookie
  and stack base, creates a manual-reset event, duplicates the current thread
  handle, installs TLS and registers the thread. Allocation failure is -5.
  The port retains appended GC bookkeeping and checks event/handle failures.
- `sysThreadSetStackBase` 0x1004CA6B stores offset 12.
- `sysThreadFree` 0x1004CBC5 clears Java PrivateInfo/eetop, notifies an existing
  in-use monitor, unregisters, deletes the saved execution environment, clears
  TLS and closes both handles. Its int declaration does not correspond to an
  explicit status assignment: disassembly returns EAX left by CRT free.
  This implementation explicitly returns SYS_OK, a documented deviation.
- `sysAttachThreadLock` 0x10002925 lazily initializes and enters a critical
  section; unlock 0x10002962 leaves it. Unix uses a pthread mutex.
- Shared lifecycle recovery (implemented in the invocation tranche): defaults
  0x100024EA; initialize 0x100025E3 returns 0 on success/-1 on failure; finalize
  0x1000290B waits for user threads and returns -1.
- `opendir` 0x100155B0 uses the 588-byte original DIR layout, path allocation
  of strlen+5, slash conversion 0x10015814, GetFileAttributes and FindFirstFile.
  Pattern strings are `\\*.*` at 0x1006D968 and `*.*` at 0x1006D970. The primary
  root test treats any leading slash as the current drive root; preserved.
- `readdir` 0x10015860 copies into the inline 260-byte dirent name, advances
  FindNextFile and closes the exhausted handle. `closedir` 0x10015908 frees
  path and DIR after closing the live handle. `rewinddir` has no linked body
  located in the primary: implementation is explicitly header-contract based.

JNI host attachment wrappers currently delegate to these HPI functions. The
private cookie setter remains necessary while JNI constructs the Java object
*after* attaching the host thread; it is not represented as an original API.
The shared threadCreate redundant setter was removed because sysThreadCreate
already receives the Java cookie.

Verification: isolated Windows and Linux Release hpi115-test pass, including
stream readiness plus directory exhaustion/rewind and Windows layout offsets
(path260, handle264, find_data268, sizeof588). MSVC x86 compilation of changed
thread_md.c and shared threads.c passes against the new original headers;
Linux x86 C89 compilation of thread_md.c passes. Full VM/JNI/thread gates are
pending the concurrent oobj/interpreter/collector integration. These isolated
checks do not establish whole-VM ABI fidelity.
