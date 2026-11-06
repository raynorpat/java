# Class ABI allocation and ownership evidence

Read-only preparation for Task 3, 2026-11-06. Primary: shipped Win32
`javai.dll` / `javai_g.dll` and `vendor/jdk1.1.5/include/oobj.h`.
Secondary: Rhapsody DR2 `libjava.A.dylib`, explicitly opened in IDA.
Addresses below are Debug unless labeled otherwise. No production changes
are part of this investigation.

## Managed class allocation and build layouts

`allocClassClass` is `10027BF3` (Debug), `1001A570` (Release), and named
`_allocClassClass` at Rhapsody `5C13CE20`. All select the method table from
`unhand(classJavaLangClass)->methodtable`, or NULL before Class bootstrap;
allocate a managed body with a normal handle; and store the resulting handle
in body offset 20 (`HandleToSelf`). They return the handle, not the body.

Release requests **100 bytes**. Rhapsody disassembly at `5C13CE2B` pushes
`64h`, then calls its object allocator and stores `[handle] + 14h = handle`.
Debug requests **112 bytes**, confirmed by `10027BF3` calling `10027C61`
with size `70h`. This is JCOV, not an unexplained alignment discrepancy:
Debug parser `1000E2A9` recognizes AbsoluteSourcePath and TimeStamp attributes,
writing body+100 and the QWORD at body+104. The shipped header has those
fields under `#ifdef JCOV`. Release and Rhapsody lack this extra allocation.
Methodblocks likewise stride 100 in Debug `FreeClass`, versus 92 in
Rhapsody's `5C10BCF2`; preserve the shipped conditional coverage fields.

`10027C61` rounds body request plus allocation header to eight bytes,
allocates heap space, calls handle allocator `10027A03`, zeros the requested
body size, and returns the handle. It participates in normal GC/retry and
reserve policy. A class must not instead receive `calloc(sizeof body)` plus
an independently allocated synthetic handle.

Common 32-bit body offsets from the shipped header and primary uses:

| Offset | Field |
|---:|---|
| 0, 2 | major_version, minor_version |
| 4, 8, 12 | name, super_name, source_name |
| 16, 20, 24, 28 | superclass handle, HandleToSelf, loader, finalizer |
| 32, 36, 40, 44 | constantpool, methods, fields, implements |
| 48, 52, 56, 60 | methodtable, methodtable_mem, slottable, classname HString |
| 64 | hashinfo union (8 bytes; primitive typecode/typesig/slotsize/elementsize) |
| 72, 74, 76, 78 | constantpool/methods/fields/implements counts |
| 80, 82, 84, 86, 88 | methodtable_size, slottbl_size, instance_size, access, flags |
| 92, 96 | signers, imethodtable |
| 100, 104 | Debug JCOV absolute_source_name, timestamp |

The handle itself is two words, object pointer then methodtable pointer.
`ClassClass` is typedef `struct Hjava_lang_Class`, not the body type.
All class links in methodtables, fieldblocks, constant pools, arrays, and
the class registry must consequently contain handles.

## Metadata is two contiguous allocations, not a per-class arena

`createInternalClass` `1000E075` invokes parser `1000E2A9` twice under the
class-load lock. Context allocation helper `10010342` implements:

1. Pass 1: calloc temporary request+8, chain allocations for cleanup, round
   each request to an eight-byte total, and accumulate two separate sizes.
2. Free the pass-1 temporary chain (`100104DD`); reset parse cursor and
   temporary class CP/fields/method count; calloc the main aggregate and,
   if nonempty, the `<clinit>` aggregate.
3. Pass 2: return slices from the two aggregates, advancing each cursor by
   its eight-byte rounded request. Allocation selection follows the parser's
   in-clinit flag (set while parsing `<clinit>()V`, cleared after its attributes).
4. Register the successful class with `1000D670`. On parse longjmp,
   `100104DD` frees either the temporary chain or both aggregate allocations,
   according to the current pass.

`LoadConstantPool` `1000F30A` allocates CP first, then type bytes, then UTF8
strings through that helper. Thus `cbConstantPool` is also the **main slab
allocation base**. Interfaces, fields, methods, normal bytecode and method
attributes are slices; freeing them individually is invalid. Class names
normally point into CP UTF8 storage, not independent strdup allocations.
Primitive names are a separate sticky-class case (`1000ED18` uses strdup).

`FreeClass` `1000FEE3` / Rhapsody `_FreeClass` `5C10BC40`:

- Calls CompilerFreeClass (`10014A25` invokes configured compiler hook).
- For `<clinit>()V` with nonzero code_length, calls `1000FE54`, which frees
  the lower non-null address of code and declared-exceptions pointers and
  clears both. This is the separately allocated clinit slab, whose first
  attribute can be Code or Exceptions; do not assume code is its base.
- Frees constantpool, methodtable_mem, and slottable.
- Frees imethodtable only when implements_count != 0 or ACC_INTERFACE is set;
  otherwise this pointer may be shared/inherited.
- **Does not free the class handle or body.**

## Bootstrap and collector requirements

Initializer `1001082A` eventually sets `handle->methods` from the current
Class methodtable. Resolver `10010E9B`, when resolving Class itself, holds
the class-registry monitor and fixes every loaded class handle's methods
pointer to that table; then `10012851` initializes all nine primitive classes.
The current tree has analogous fixup logic, but the new allocator must allow
the temporarily NULL methods pointer without dereferencing it during GC.

Ordinary heap scanner `1002A7FF` first marks an object's class handle through
its methodtable. If that class is classJavaLangClass, it invokes special
class metadata scanner `1002ACFD`. The latter traces resolved CP **Class**
entries (type byte 0x87), reference statics, superclass, classname, loader,
and signers. It does not blindly treat metadata words as object references.
Resolved interned strings have their separate intern-table root path.
Class-specific recursive marker `1002AC4B` and ordinary-object marker
`1002B091` are distinct; both preserve mark strength.

Class roots `1002B143`: root every registry class if unloading is disabled;
otherwise root only Sticky (0x400) classes. This is materially different from
the current unconditional scanClasses roots. Replacing offsets alone cannot
implement unloading.

After marking, `1002B8D0` finds unmarked registry handles, marks them to
protect them during pending cleanup, links them through the now-dead body's
finalizer field (offset28), and removes each registry entry by moving the last
entry into its slot. `1002C738` drains this list under heap lock: FreeClass
first, then `1002B7F7` reclaims the body block and returns its handle to the
GC handle free list. No libc free is applied to either managed allocation.

## Coordinated migration recommendation (design, not implemented)

1. Adopt the shipped handle/body distinction and accessor macros together
   across VM, JNI, native adapters, verifier, debugger, and standalone tools.
   Remove raw-body `ClassClass*` assumptions and redundant unhand calls that
   currently convert superclass handles into the old typedef. Add 32-bit
   offsetof/sizeof checks for both JCOV configurations and methodblock strides.
2. Put allocClassClass beside the managed object allocator so it can request
   an explicit body size before Class is resolved. Preserve initialization
   fixups and root a handle across any allocating operation; re-fetch its body
   after such operations because compaction may move it. Do not append arena
   ownership or primitive bookkeeping to the externally specified body.
3. Replace loader ArenaAlloc calls with the recovered two-pass context
   allocator. Preserve the current Exceptions parser and malformed-input
   checks while moving all metadata ownership to the two slabs. Implement
   FreeClass before enabling unloading; keep methodtable_mem distinct from
   an adjusted/shared methodtable pointer. Synthetic array construction and
   compiler-generated clinit transformations need their own ownership audit.
4. Introduce the special Class tracing path and mark ordinary objects' class
   handles. Preserve explicit loader/name/signers/static roots while first
   validating managed bootstrap with unloading disabled; then implement
   sticky roots, registry removal, pending-unload queue, and managed reclamation
   as a coordinated collector change. Do not claim parity while retaining
   unconditional roots or leaking metadata slabs.
5. Verify bootstrap under small-heap collection, primitive identity,
   reflection/JNI class identity, signers/name/static references surviving GC,
   failed class loads repeatedly, then custom-loader unloading with live
   instances, class literals, reflection objects, and superclass references.
   Memory ownership tests must include `<clinit>` with both Code/Exceptions
   attribute orders and interface methodtable inheritance.

Remaining recovery before implementing the full switch: synthetic array
metadata allocation layout, methodtable allocation/adjustment, clinit early
free timing, and precise bootstrap-GC handling of NULL methodtables. The
addresses above establish the core allocation, metadata ownership, and
unload contract; these remaining paths must not be guessed.

## Captured header regression

`tests/native/class-layout115.c` contains compile-time sizeof/offsetof checks
for every field of Classjava_lang_Class, methodblock, JavaFrame, and ExecEnv,
plus ClassClass/Hjava_lang_Class handle sizes and a type-identity helper whose
conflicting declarations fail if ClassClass names the body. Expected constants
were generated from the isolated vendor-header probe's actual stdout, retained
as `class-layout115-release.layout` and `class-layout115-debug.layout`.
MSVC x86 compiled and executed the regression against the original vendor
headers in both configurations with exit 0. The commands used `/MD /DWIN32
/D_WINDOWS /DXP_PC /DXP_WIN32`, only the vendor include and vendor include/win32
directories, adding `/DDEBUG /DJCOV` for the second configuration. No packing
override was used. Captured JavaFrame is 44 bytes and ExecEnv is 76 bytes in
both configurations, with JNI nativeInterface at20, seenclasses at52 and
allocation cache at60. These are original layouts, not the transitional tree's.
The regression is intentionally not wired into CTest until the ABI migration.
