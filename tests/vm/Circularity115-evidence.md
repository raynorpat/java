# Class circularity recovery

Primary Debug `javai_g.dll` `Locked_InitializeClass` at `1001082A` checks
whether a class whose superclass is still unset occurs in the current EE's
seen list. It sets the Error flag and returns ClassCircularityError with the
class name as detail when found. Otherwise it pushes a native stack node,
loads/initializes the superclass, and pops the node before processing the
result. Failed superclass loading also sets Error; later resolution produces
NoClassDefFoundError.

Helpers `10010DEE` (push), `10010E17` (pop with corrupt-stack panic), and
`10010E4D` (handle-identity membership) operate on EE offset 56, the `next`
field of the original embedded seenclass sentinel. Nodes contain a class
handle and next pointer. Class bootstrap saves/clears this head while loading
sticky String, ThreadDeath, Throwable, Exception, Error, RuntimeException,
Cloneable, and java/io/Serializable, then restores it on success. The recovered
bootstrap's early missing-dependency returns precede restoration; this change
does not introduce different bootstrap error handling.

No GC production edits accompany this change. The collector review confirmed
the primary relies on conservative native-stack rooting for these stack nodes;
it does not add a separate EE seen-list root walk.

Compile `Circularity115.java` using Sun javac in tests/vm, then run
`python patch-circularity115.py`. The patch replaces precisely one superclass
UTF8 constant in Cycle115A (Base to B) and Cycle115Self (Base to Self), retaining
all constant-pool indexes. Source alone is intentionally acyclic; checked-in
class files contain the two cycles. Capture using
`cmake -DCLASS=Circularity115 -P tools/capture-expected.cmake` from the root.

The actual Sun process exited zero and produced the retained expected file:
self-cycle and first A/B cycle loading throw ClassCircularityError; subsequent
B/A loading throws NoClassDefFoundError; an unrelated base class still loads.
Before the resolver change, the current Linux Release VM exited 1 with no
output on this fixture, while Hello115 immediately passed on the same binary
and classpath. Windows' older pre-ABI binary also failed with stack overflow,
but that is not used as the current-tree red result.

MSVC x86 and GCC Linux32 syntax checks accepted the new resolver code. The
existing loader-pointer type warnings remain outside this bounded change.
Runtime green verification is pending the coordinated rebuild.

The first full Release gate exposed exception replacement: both first loads
incorrectly surfaced NoClassDefFoundError. Primary initialization/resolution
wrapper `100122A1` calls SignalError only if EE.exceptionKind is zero. Both
FindLoadedClass error paths now preserve a pending ClassCircularityError.
The self-host fixture runner now invokes the same patch script on its freshly
compiled output directory; its original unpatched classes naturally contain
no cycle. Copies of the actual self-hosted output, patched through this new
directory argument, were run on Sun and matched the oracle with exit zero.
The self-host fixture mutation requires Python 3, discovered by CMake.
