# Remaining core natives: evidence and scope

Primary binary: explicitly opened vendor/jdk1.1.5/bin/javai_g.dll, IDA instance
1493265a537f. All listed stubs and their native targets were decompiled.

| Native | Stub | Native target |
| --- | --- | --- |
| Class.getSigners | 10060B85 | 1000B933 |
| Class.setSigners | 10060BAE | 1000BA04 |
| Compiler.initialize | 10060D3D | 10014BF6 |
| SecurityManager.currentLoadedClass0 | 1006194C | 10040291 |
| FileDescriptor.sync | 10062286 | 1002F5AB, helper 10030B1E |
| File.canonPath | 1006294A | 10015FE0, helper 100394BA |
| Runtime.runFinalizersOnExit0 | 100616C7 | 1003FDF5 |

Class stores the supplied signer array directly at Sun ClassClass offset92.
getSigners allocates a new reference array and copies length+1 pointer slots,
including the component-class tail. This tree appends a signers pointer rather
than adopting Sun's full ClassClass layout; scanClasses explicitly roots it.
Core115 distinguishes caller-array aliasing from the defensive getter copy.

currentLoadedClass0 checks SecurityManager initialization, then returns the
first stack-frame declaring class with a nonnull loader. Compiled frame links
use the current tree's existing adapter. Compiler.initialize already exists in
compiler.c and already has an adapter; no duplicate was added. The no-JIT case
is tested. Sun additionally sets its initialized flag after the start callback;
the current compiler infrastructure has a different internal representation.

FileDescriptor.sync uses CRT descriptor minus one, _get_osfhandle, and
FlushFileBuffers. ERROR_ACCESS_DENIED is ignored; other errors become
SyncFailedException with FormatMessage text. The POSIX mapping uses fsync and
strerror, and is a platform adaptation rather than Win32 binary equivalence.

File.canonPath calls _fullpath into 260 bytes and recovers existing components'
on-disk spelling with FindFirstFileA. The binary calls FindClose even when the
find handle is invalid before consulting GetLastError; this observable behavior
is retained. A missing intermediate prefix fails with IOException("bad path
error") on the Sun oracle. A missing final basename is retained. Bounds checks
avoid the binary's unchecked strcat paths. UNC prefix handling is bounded but
not yet oracle-tested. POSIX mapping resolves existing prefixes through realpath
and normalizes dot components; missing-prefix semantics differ from Win32.

Core115 and Canonical115 were compiled with Sun javac and expectations captured
using tools/capture-expected.cmake. Core115 tests signers, forced GC, bootstrap
currentLoadedClass, sync, and no-JIT Compiler behavior. Canonical115 checks dots,
an existing filename, and absolute-path idempotence; it does not imply complete
UNC, symlink, permission-error, or missing-prefix conformance.

Runtime.runFinalizersOnExit0 sets the enabled flag. sysExit (1004BF67) invokes the all-object finalizer drain
(10016CCB) only for successful exit. That helper first collects, waits for pending
finalization (10016A10), and repeatedly transfers live finalizable objects. Sun's
drainer tracks its owner thread, waits on another owner, and notifies waiters.
The implementation now uses one owner-aware drainer for synchronous calls and
the daemon finalizer thread. It clears target exceptions (10016BDF), clears owner
and BeingFinalized under the queue lock, then frees the finalizer_t node. The
all-object drain reverses the live queue into the pending queue and repeats for
objects allocated by finalizers. Current finalizer_t nodes replace Sun's intrusive
object links. A runtime-installed callback lets sysExit trigger this without
forcing VM finalization symbols into javah, which also compiles system_md.c.
Normal launcher completion must call that callback before losing its ExecEnv.

FinalizeExit Sun oracle (bounded subprocess runs): default/zero and normal exit
print main, final=1, final=2 and exit zero; nonzero prints only main and exits7;
disabled prints only main and exits zero; recursive System.exit from finalizer1
prints main, final=1 and exits zero without hanging. Finalizers also recurse into
runFinalization and allocate another live finalizable object. The default output
is captured in FinalizeExit.expected; mode-specific comparisons include status.

Canonical115 exposed an older Windows File.isAbsolute defect: sysIsAbsolute's
macro recognizes only leading backslash, misclassifying drive-qualified paths.
Sun native100162CC instead recognizes leading slash/backslash or alphabetic
drive + colon + slash/backslash. filesystem.c now follows that check on Windows.
