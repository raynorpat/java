# Platform header and arithmetic reconstruction

The active shared vendor headers now match the originals except for two marked
retained adaptations: debug.h routes diagnostic stdio through JNI hooks, and
oobj.h retains relative bytecode-switch alignment. Win32 typedefs_md.h and
sysmacros_md.h now use Sun's original definitions plus explicitly marked host
and portable-source declarations. Byteorder recognizes current compiler macros;
math maps the original remainder operations onto the modern CRT; thread extra
fields retain native-register scanning and platform lifecycle bookkeeping.

## Numeric helpers

Rhapsody named double2l (5c137efc) and double2ll (5c137f88) implement NaN-to-zero,
saturating overflow, and truncation. Win32's original typedefs_md.h declares the
four conversion helpers and ll_shl, and INT_OP handles MIN_VALUE divided by -1.
NumericConversions115 uses runtime operands (not constant-folded casts) and
captures actual Sun 1.1.5 stdout for NaN, both infinities, signed boundaries,
values immediately inside long range, fractions, negative zero, division and
remainder. The original reconstruction printed minimum integers for NaN/positive
overflow and exited abnormally at integer division overflow on Linux. The new
shared helpers and interpreter overflow cases match Sun; the focused Linux and
Windows Release gates pass. ll_shl preserves the original helper interface while
using unsigned shifting to avoid signed-shift undefined behavior.

## Win32 files and native paths

Primary javai_g.dll functions:

- 100304E0 sysOpen: reject paths longer than 259 bytes with errno38, normalize a
  copied path, force binary mode, then open.
- 10030562 sysAccess and 100305DB sysStat: same bounded-copy normalization.
- 10030654 sysUnlink: normalize, clear attributes, then DeleteFileA.
- 100306EF sysMkdir and 10030763 sysOpenDir: bounded-copy normalization.
- 100392D7 sysNativePath: normalize separators, leading separators before a
  drive letter, repeated separators, and trailing separators except roots.

file-hpi115.c calls those primary private addresses in SUN_ORACLE mode. It
checks nine path forms, both long-path errors, and read-only deletion. Baseline
normalization preserved repeated/trailing separators, returned errno2 for long
paths, and failed read-only deletion. The reconstructed wrapper probe matches
actual Sun output byte-for-byte (exit0). Its CTest retains the captured output.
The current CRT stat representation remains the host API representation.

The classpath integration test caught an old caller passing the literal "." to
the restored in-place normalizer. Primary sysGetClassPath (10039140) does not
normalize entries in place; the Win32 parser now likewise leaves them untouched,
and file wrappers normalize owned copies. The existing empty/classpath-entry
matrix and native file probe pass after this correction.
