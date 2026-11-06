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

## Loader migration implementation contract

The migrated loader uses `allocClassClass()` and the original six-argument
`createInternalClass(bytes, limit, cb, loader, expected_name, detail)`.
Its first pass measures aligned allocations through temporary nodes; its
second pass uses one main slab and one slab for `<clinit>()V` attributes.
The constant-pool allocation is first in the main slab. Both successful
parsing and synthetic-array creation register the class themselves; callers
must remove their former extra `AddBinClass` calls.

Primary Debug `1000EBE7` establishes the five-argument synthetic constructor:
name, base type, depth, component class, loader. It allocates four CP words
followed by four type bytes (20 bytes on x86), separately duplicates the name,
sets classfile version 45.3, and stores an already-resolved component handle
in CP slot 3 for reference arrays. Its access is PUBLIC inherited from the
component plus FINAL|ABSTRACT; primitive arrays are PUBLIC|FINAL|ABSTRACT.
There is no fifth class-name slot. The recovered FreeClass does not separately
free the synthetic name; that apparent original ownership gap is retained,
not hidden behind invented arena ownership.

Primary parser `1000E2A9` dispatches the configured CompiledCodeAttribute to
`ReadInCompiledCode` only for bootstrap-loaded classes and rejects argument
word counts above 255. These branches are preserved along with Exceptions
and Debug JCOV source/timestamp metadata. Standalone tools linking the loader
must provide the compiler hooks or their existing tool-only equivalents.

`FreeClass` releases native metadata only; GC owns body and handle reclamation.
The helper `freeClassInitializer`, recovered from `1000FE54`, frees the lower
code/Exceptions address when Exceptions exists, otherwise code; a valid
initializer has Code. It clears both pointers and leaves code_length intact.
This allows either classfile attribute order. Explicit transitional deviations: registry
operations use the existing class lock instead of Sun's separate registry
monitor; slab-size overflow is rejected; freed metadata pointers are cleared
because the retained legacy source-retry path may leave the managed body
reachable. Implementation is awaiting coordinated header/collector build and
runtime verification; this note does not assert passing tests.

Primary exported `NameAndTypeToHash` at `10012685` accepts only name and
signature. Under monitor `100835CC`, it calls Str2ID (`1004E541`) twice on
table `100835D4`, with NULL parameter output and copy enabled, returning
`(nameID << 16) + signatureID`; either zero raises OutOfMemoryError and returns
zero. There is no loader argument or loader-key construction. Str2ID allocates
2003-slot chained tables at 80 percent occupancy; allocator `1004E897`
creates a 20-byte header plus 8 bytes per slot. Hasher `1004E8F1` computes
37 times the prior hash plus each signed byte and uses `(hash & 7) + 1` as
the probe step. Existing compatibility reverse helpers must interpret the
low half as signature and high half as name after this migration.

`classhash115.c` implements the pair operation with a dedicated initialized
monitor and the shared Str2ID, whose capacity is now 2003. Its private
compatibility combined-string table does not alter member IDs. Initialize
through `initializeClassHash115()` after monitor bootstrap and before any
member resolution; the standalone tool requires the same call with its
existing no-op monitor adapters. Shared Str2ID still aborts on allocation
failure, unlike primary Str2ID's zero result; this existing limitation remains.

Verification: MSVC x86 `/Zs` Debug+JCOV accepted classloader.c and classhash115.c
without diagnostics against the migrated headers. The retained
`tests/native/classhash115-probe.c` ran against Sun Debug Str2ID at RVA
`0x4e541` using a fresh local table, then against classhash115 plus the real
updated util/arena. Both executions returned zero and stdout bytes matched
`classhash115-probe.expected`: 5000 names, several table expansions, repeated
pairs, distinct signatures, and signed UTF-8 bytes. The normal probe also
checks reverse compatibility strings. This isolated probe stubs monitors and
does not claim concurrent VM coverage. Build commands are retained in
`out/check-hash115.cmd`; it is not yet registered in CTest.

## Implemented managed allocation/cache slice (integration pending)

The shared collector now allocates Class bodies through `allocClassClass`,
using `sizeof(Classjava_lang_Class)` and the current Class methodtable, and
stores the returned managed handle in HandleToSelf. Platform `gc_md.c` files
no longer supply the arena-backed `AllocClass` implementation. The normal
marker traces object class handles, object-array element class handles and
typed Class metadata, propagating the primary soft/hard mark strength.
The depth-30/100-cell bounded worklist is retained for both object and class
metadata recursion. NULL methodtables during bootstrap take the class metadata
path explicitly; this defensive bootstrap handling is a port deviation.

Registry roots honor `classgc`/Sticky. With the world stopped, unmarked registry
entries move to the intrusive pending-free list via the dead finalizer field.
After world resume, under the existing heap lock, FreeClass releases metadata
and the collector returns the body and handle. Pending entries are rooted if
cleanup triggers another collection. The existing port class-load lock is not
acquired under heap lock; adding that inverse order could deadlock loaders.
This retains the port's stopped-world registry mapping while separate Sun
registry-monitor migration is pending.

Isolated GCC Linux32 Debug and MSVC Win32 Debug+JCOV compilation succeeded
against the adopted original headers. Runtime integration gates remain pending.

## Managed-class lifetime oracle

`tests/native/managed-class115.c` compiles against vendor headers and loads
primary `javai_g.dll` with SUN_ORACLE. It creates a real custom loader and
uses JNI DefineClass on `ManagedPayload115.class`. Both construction phases
run on attached native worker threads that detach and exit, eliminating stale
worker-stack roots before explicit GC. Registry inspection uses primary
binclasses RVA83550 and count RVA8351c; GC uses RVA2c9a9.

Fresh primary runs (exit0 each) establish:

- Global Class reference keeps the custom class and its loader registered.
- After deleting that Class reference, a global instance keeps its defining
  class registered.
- After deleting the instance, enableClassGC=1 unloads the class; disabling
  class GC retains it.

Outputs are respectively `managed-class=unloaded` and
`managed-class=retained`. CMake targets `managed-class115-1` and
`managed-class115-0` execute the same checks against the reconstruction.
Helper Java sources were compiled with original Sun 1.1.5 javac.
The class fixture also initializes a static byte-array marker before collection
and checks its two values afterward, proving typed reference-static tracing
rather than registry retention alone. Both primary modes still pass after this
addition.

Initial coordinated Linux Release runtime gates passed both managed-class
modes, allocation-cache115, JNI invocation and stress; broader reflection
migration failures are tracked separately by the coordinator.
