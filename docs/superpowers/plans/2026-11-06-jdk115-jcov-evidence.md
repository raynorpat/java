# JCOV recovery and verification

Primary authority is Sun Win32 javai_g.dll. Release javai.dll establishes
the disabled configuration. Rhapsody `_java_main` at 5C14149C recognizes
and skips `-prof=jcov` and nonempty `-prof=jcov:` arguments, corroborating
the Release disabled path. The strings are at 5C14A89E/5C14A8A9; decompiling
the actual parser established behavior despite missing direct string xrefs.

## Options and observed behavior

Debug `java_main` 100012D5 recognizes `-prof=jcov` (default java.jcov) and
`-prof=jcov:<nonempty file>`, disables class GC, and registers exit callback
100021D4. The callback calls writer 1003A02F, retries return value 1 up to
five attempts, then reports that the temporary file is in use. Other return
values stop retries. `-prof:jcov` is ordinary method profiling to file jcov.

Actual Sun oracle probe: javac `-prof=jcov JCovProbe115.java` instruments
the class; `-Xjcov` is rejected. Debug execution with `-prof=jcov:debug-first.jcov`
returned zero, printed 20, and generated the coverage file. Running it again
returned zero and doubled counters by merging. Release execution with
`-prof=jcov:release-first.jcov` also returned zero and printed 20, but created
no coverage file. Release java_main explicitly recognizes and ignores these
forms, confirmed in decompilation, rather than rejecting them as unknown.
Initial probe artifacts are in out/oracle-debug115.

Sun compiler `javac -prof=jcov:<file>` also emits an initial coverage file
with the same class/source/timestamp sections but only flag/line/position
columns. This is the writer's format-1 input, whereas completed reports
contain counts (format 2) or counts/testcounts/scales (format 3).

## Metadata and execution

Debug Code-attribute parser 1000F5AA recognizes CoverageTable only when
cov_file (IDA FileName) is non-NULL. Reader 1000F9C9 reads a u2 record count,
then records {u2 pc, u2 type, u4 where}; count initially zero. Allocation is
16 bytes per record plus 4 bytes per bytecode byte, containing a trailing
PC-to-last-record-index table (one-based, zero for no records). It temporarily
clears the loader's clinit-allocation flag so coverage survives initializer
code reclamation. A wrong attribute length clears coverage_table. A faithful
adaptation must bounds-check pc before indexing this native table.

Interpreter 10053290 updates all records sharing the current PC, walking
backwards from its indexed last record, incrementing types below 5. A local
guard avoids counting redispatch of the same instruction twice. Branch helper
10039D90 updates one of the final pair at a conditional PC. Lookup-switch
10039E32 and table-switch 10039F05 update case/no-default records; their exact
index conventions must be preserved together with the interpreter callers.
Original vendor jcov.h supplies the eight CT_* values and helper signatures.

## Output and merge

Writer 1003A02F is a substantial merge algorithm, not merely a fresh report
formatter. It checks/creates a directory (1003BE13), exclusively creates a
`.temp` file (1003BBE7 through 1003BCA2), reads an existing coverage file
(1003BAED), merges matching class/source/timestamp sections and test bitmaps,
appends uncovered loaded classes (1003BFFC), copies the temporary stream into
the final file (1003BF8F), and removes the temporary file (1003B992).
More precisely, 1003BCA2 exclusively creates `<output>.temp.lock` with
O_CREAT|O_EXCL (mode 0600), then opens `<output>.temp` as a binary update
stream. Existing lock returns 1, which triggers the exit wrapper's bounded
retries. Cleanup 1003BA54 removes the temporary data, closes the lock descriptor,
and removes the lock. The temporary data file itself is not opened O_EXCL.

The header is `JCOV-DATA-FILE-VERSION: 1.0`. Class sections contain CLASS,
SRCFILE, TIMESTAMP, then flag/line/position/count columns, with optional test
count and bitmap columns. `where >> 18` is line, `where & 0x3ffff` is position.
Native methods are skipped; CT_CASE records with where zero are omitted.
Existing files match by class name, absolute source name, and timestamp.
The implementation preserves all three input formats, accumulated counts,
test count/bitmap columns, unmatched class sections, and original diagnostic
source-line numbers. Verification is described below; full VM verification
now includes the shared Windows Debug VM and self-compiled coverage fixture.

## Deterministic fixture

`tests/vm/JCov115.java` covers both branch directions, dense and sparse
switches, a switch without default, and a class initializer. Compile using
Sun javac `-prof=jcov`, then run `patch-jcov115.py`. The patch updates the
class's AbsoluteSourcePath constant to JCov115.java and TimeStamp to 1;
it does not rewrite oracle output or instruction/coverage records. Sun then
produces deterministic metadata itself.

Sun Release captured JCov115.expected (34, exit zero). Two successive Sun
Debug invocations on the patched class exited zero, matched that stdout,
and produced the retained JCov115-first.jcov.expected and
JCov115-second.jcov.expected files. Their records include initialized clinit
counters even though initializer bytecode has been reclaimed. The second file
contains additive merged counters. Implementation began after ABI checkpoint
d067dd3 passed its four configuration gates.

## Implementation and isolated verification

`jcov115.c` supplies counters, writer, and the five-attempt exit callback.
The loader allocates CoverageTable data and its trailing PC index in the main
metadata slab. Interpreter dispatch counts an instruction only once across
opcode rewriting and resets the guard when advancing or changing frames;
branch and switch hooks follow the primary call conventions (table-switch key
is already adjusted by low; lookup-switch passes the remaining pair count).
Debug enables these paths. The standalone header generator explicitly leaves
cov_file NULL. Launcher option integration is in javai.c.

Explicit deviations: malformed coverage PCs and counter indexes are bounded
before native memory access; diagnostic bitmap input has a bounded scanner;
temporary allocations and files are cleaned up on error; report formatting
does not mutate live class names from slashes to dots. Platform directory
separators are used for Linux output paths. These do not change the verified
valid-report bytes.

`tests/native/jcov115-probe.c` calls the actual Sun Debug writer at RVA 3A02F
with original-layout synthetic class metadata, setting its registry at RVAs
83550/8351C and testname at 71870. The normal build links our writer with
standalone CRT file adapters. Fresh reports, compiler seed rows (format 1),
preexisting counts (format 2), named tests/bitmap merging (format 3), and an
unloaded class section all matched byte-for-byte, with both executables exiting
zero. The comparison exposed and corrected the required `#` bitmap prefix.
Retained `.seed` inputs and `.expected` outputs feed `jcov115-writer` CTest;
the same native regression passed isolated Windows32 and Linux32 builds.

`tests/vm/jcov115.cmake` passes against Sun Debug and Release. It checks fresh
and merged reports, default filename, exclusive-lock retry exhaustion,
header-only malformed input preservation, cleanup, and Release's accepted but
inactive options. The current pre-change reconstructed Debug executable fails
this script by rejecting the coverage option, establishing the runtime red
test. MSVC32 and GCC32 isolated syntax checks accept the implementation.
The shared Windows Debug VM passed the entire script after exit-callback
registration was restored. The strengthened gate supplies JAVAC, compiles
JCov115.java with the reconstructed compiler's `-prof=jcov`, normalizes that
new class's source/timestamp metadata, and puts the new output first on the
classpath. That full self-compiled gate passed on the shared Windows Debug
runtime. Omitting JAVAC retains the original Sun-oracle replay mode.

## Exit callback dependency

The first shared coverage test ran the program correctly but produced no
report: both platform sysAtexit adapters were no-ops. Primary sysAtexit
1004BFFA allocates an 8-byte function/next node and prepends it; allocation
failure returns 1, success 0. sysExit 1004BF67 runs exit finalizers first,
drains callbacks in LIFO order, then invokes the JNI exit hook or CRT exit.
Rhapsody 5C135ED4/5C135E64 confirms the same order and representation.

Both platform adapters now implement this mechanism. Existing finalizer and
JNI hook calls remain in their original positions. Explicit safety deviation:
each callback is unlinked before invocation, so reentrant exit or a returning
JNI hook cannot revisit freed nodes (the primary leaves a stale global head).
sysAbort and its separate hook remain unchanged.

`tests/native/exitcallbacks115.c` calls primary Debug sysAtexit/sysExit through
their recovered RVAs and installs a returning exit hook at RVA 71868. Actual
Sun stdout was callback=2, callback=1, exit=7, returned; exit status was zero.
The reconstructed Windows standalone adapter produced identical stdout bytes
and exit status. The Linux32 adapter passed the same retained oracle. CTest
`exitcallbacks115` preserves this ordering regression independently of JCOV.
