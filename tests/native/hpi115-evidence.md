# JDK 1.1 host API audit

Primary reference is `vendor/jdk1.1.5/bin/javai_g.dll`, inspected in IDA,
and the shipped `include/sys_api.h` and `include/win32/sysmacros_md.h`.

* `sysAvailable`: `100307D4` calls CRT fstat, then selects readiness for
  character/FIFO descriptors or seek-to-end with position restoration for files.
  It returns the negative fstat result unchanged. `100308D8` uses
  `_get_osfhandle` and PeekNamedPipe, treating ERROR_BROKEN_PIPE as successful
  zero available bytes. Descriptor zero uses `1003094E`: GetStdHandle and a
  zero-time WaitForSingleObject; signaled means one byte, timeout/abandoned
  means zero, other failure means failure. `10030ACB` unwraps FileDescriptor's
  biased descriptor and delegates. The existing permission checks in
  sysAvailableFD remain, with the actual readiness work now delegated.
* `sysIsMP`: `1004D4B1` gets SYSTEM_INFO and returns processor count > 1.
  Unix maps this to sysconf(_SC_NPROCESSORS_ONLN).
* MemoryFlush is explicitly `lock xor DWORD PTR 0[esp], 0` in the shipped
  Win32 header. StoreBarrier is explicitly a no-op for Intel there. Unix
  uses compiler full/release fences as a documented platform mapping.
* Rename and Rmdir are CRT macros in the shipped Win32 header. FStat maps
  to the same CRT fstat used at `100307D4`. Unix uses its POSIX counterparts.
* CanonicalPath `100394BA` has path, destination, and destination-size
  arguments. The existing recovered corepath115 algorithm is reused through
  a bounded adapter; no duplicate path algorithm is introduced. The adapter
  rejects insufficient destination capacity without copying a partial result.
* Directory open/read already had platform implementations. The shipped
  CloseDir signature returns int. `10015908` returns -1 with EBADF if
  FindClose fails, retaining the allocation, otherwise frees and returns 0.
  Current Win32 CloseDir now follows that success/failure contract; Unix
  forwards closedir's status. The existing different directory iterator
  layout and deferred handle close remain platform implementation mappings.

`sysAllocBlock` / `sysFreeBlock` declarations are guarded by PAGED_HEAPS in
the shipped sys_api.h. They are not unconditional missing APIs. Their block
ownership/alignment contract has not been recovered from this non-paged
Win32 binary; no speculative implementations or unconditional declarations
were added. Existing map/commit/unmap APIs are separate and unchanged.

Native regression `tests/native/hpi115.c` checks file available bytes and
unchanged position, FStat, repeated non-consuming pipe readiness, buffered
pipe data after writer close, empty closed pipe, directory open/read/close,
MP boolean, and callable barriers. MSVC x86 standalone build and execution
completed with exit 0 on 2026-11-06. Linux GCC `-std=gnu89` standalone build
and execution also completed with exit 0. These are contract assertions rather than
a claim of a standalone Sun native harness comparison. Canonical behavior
remains covered by the existing Sun-captured Canonical115 fixture.
