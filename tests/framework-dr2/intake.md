# DR2 JavaVM.framework reconstruction

Approved design: `docs/superpowers/specs/2026-11-08-rhapsody-javavm-framework-design.md`.
Branch starts at 25341e0. Generic Phase 7 runtime acceptance does not establish
Objective-C embedding support.

## Donors

The extracted JavaVM executable is at
`vendor/rhapsody-dr2/System/Library/Frameworks/JavaVM.framework/Versions/A/JavaVM`.
IDA instance bad854553352 identifies 41 functions including imported stubs.
The guest's `/usr/lib/java/libObjCJava.A.dylib` was copied read-only into ignored
`out/rhapsody115/libObjCJava.A.dylib`; its hash and size are in donor-manifest.json.
IDA instance 2c4a2c9a0829 identifies 411 functions including stubs. Its linked
Java dependency is the original framework's `Libraries/libjava.A.dylib`, plus
Foundation and System. That VM dependency must be replaced by the reconstructed
runtime; copying this donor into the final product is not implementation.

Useful bridge entry points:

| Address | Function |
| --- | --- |
| 0x53d06008 | NSLoadVirtualMachine |
| 0x53d06080 | NSAddClassFile |
| 0x53d0caf0 | JAVALoadVirtualMachine |
| 0x53d03a08 | CreateObjCClassForJavaClass |
| 0x53d06364 | _NSInvokeJavaMethod |
| 0x53d066b8 | _NSInvokeStaticJavaMethod |
| 0x53d05448 | NSRetainJavaObject |
| 0x53d0548c | NSReleaseJavaObject |
| 0x53d08614 | NSJavaMapObjects |

## Configuration oracle

Run from the guest source root:

```
sh tests/framework-dr2/check-configuration.sh donor
sh tests/framework-dr2/check-configuration.sh
```

The first command uses the original framework. The second compiles the same
client with reconstructed configuration sources and only Foundation, then
compares both transcripts. The fixture poses an in-memory NSUserDefaults subclass
so the persistent-domain lookup is tested without writing any user defaults.
The system configuration is still the donor file, read-only.

Framework disassembly anchors: expansion 0x5cd01388, dictionary search
0x5cd01644, three-source lookup 0x5cd016c4, vendorNames 0x5cd01800,
initWithVendor: 0x5cd019b0. Search is vendor dictionary then top-level key in
each source, in application/domain/system order. The header's example `default`
dictionary is not a special fallback in the executable. Missing and empty
values differ; an empty value stops the search.

Expansion removes leading whitespace and undefined variables. The braced form
includes the closing brace in the looked-up variable name, so a normal
`${FRAMEWORK_TEST_ROOT}` disappears even when FRAMEWORK_TEST_ROOT is set.
Preserve this observed behavior rather than substituting shell expansion.
vendorNames appends dictionary keys without deduplication or value-type filtering,
removing the Vendor key from each accumulated result.

The checked transcript was observed on native DR2 Intel using cc
`-traditional-cpp -framework Foundation -framework JavaVM`. Additional bridge
fixtures and generated-wrapper dependency closure remain to be established.

## Facade and startup checkpoint

`check-facade.sh donor` validates the original framework's singleton, class
lookup, missing-class result and second-VM exception. `check-facade.sh` also
builds the reconstructed facade against Foundation only and compares the
pre-startup transcript. It verifies that a missing private bridge is fatal;
DR2 dyld terminates NSAddLibrary on an absent library before its caller can
raise the framework exception. This fixture is explicitly not acceptance of
reconstructed VM startup. No donor ObjCJava binary is used by the reconstructed
facade test.

The facade uses its containing bundle's Libraries/libObjCJava.dylib, replacing
the donor's fixed /usr/lib/java location. Actual versioned-framework relocation
remains Task 6. The `defineClass:withName:` selector has BOOL return encoding
in the donor; the decompiler's Class return inference is incorrect.

Recovered startup ABI:

- `void *NSLoadVirtualMachine(NSString *classpath)` calls
  `JAVALoadVirtualMachine(classpath, [NSUserDefaults standardUserDefaults])`,
  then initializes `com/apple/yellow/foundation/NSRuntime` through FindClass.
- `BOOL NSAddClassFile(void *vmdata, NSString *name, NSData *data)` calls the
  JNI DefineClass entry through the environment held in vmdata.
- vmdata has two pointer-sized fields: JavaVM pointer then JNIEnv pointer.
- ClassgenInitialization (0x53d04344) installs FindObjcClassForJavaClass into
  Foundation's `_NSAltClassNameFromString` hook. Implementing findClass: alone
  does not supply Java class generation.
- ObjCJava_AInitialization initializes mapping, selectors, reflection,
  exceptions, GC ownership, serialization and built-in class conversions.

The guest's `/System/Library/Java/com` was collected read-only into ignored
`out/rhapsody115/apple-java-classes.tar` (573 files, per-file hashes in the
adjacent apple-java-classes-manifest.json). NSRuntime.class has two native
declarations: initialSearchPaths `()[Ljava/lang/String;` and
initializeObjcLibrary `(Ljava/lang/String;Ljava/lang/String;)V`. The second
resolves library initialization functions, including ObjCJava_AInitialization.
This donor class set is investigation material, not an approved packaging
closure or a substitute for reconstructed native code.

## VM adapter acceptance

`check-vm-options.sh` intercepts the donor's JNI initialization calls to capture
all option values and the two-pointer result layout, then compares reconstructed
output. Booleans compare exactly with `YES`; NSJavaVerifyMode is treated as a
boolean, defaults to zero, and enableClassGC is unconditionally zero. Sizes and
debug port use intValue. These are observed adapter semantics, not recommended
modern JNI settings.

`check-vm-live.sh` runs the actual donor and reconstructed adapters against their
respective runtimes. Both define AdapterFixture from bytes, call its method
(115118), reject invalid class bytes with a Java exception, and invoke the
defined class from an attached native thread. The reconstructed test inspects
dyld's loaded images and rejects JavaVM.framework and /usr/lib/java dependencies.
This establishes the low-level adapter, not NSRuntime initialization or class
bridging. The source's temporary classpath copy is freed after creation because
the reconstructed JNI runtime duplicates it; the donor leaks that allocation.

## Method signature encoding

`check-signature.sh` compares 15 native donor/reconstruction cases, then runs
long-input bounds checks. The conversion maps Java char, byte and boolean to
Objective-C `c`, long to `q`, and references/arrays to `@`. Encodings contain
zero offsets, as in the donor. An object-array parameter leaves its semicolon
to be parsed as an unknown type, inserting a NUL that truncates subsequent
parameter encodings; this observable donor limitation is retained.

The reconstructed allocation allows two output bytes per input byte instead of
the donor's insufficient strlen+8 allocation. Truncated object descriptors stop
at the terminator. `signature-bounds.c` passed native DR2 checks and a WSL build
with AddressSanitizer and UndefinedBehaviorSanitizer, including 128 parameters
and truncated object/object-array descriptors. These bounds cases are not run
through the donor's unsafe allocator/parser.

## String conversion

`check-strings.sh` compares native donor and reconstructed round trips through
raw Java string handles. Nil, empty, ASCII, embedded NUL, non-ASCII BMP values,
a surrogate pair and U+FFFF are preserved. The reconstruction uses the VM's
character-array String constructor and copies UTF-16 units back into an
autoreleased NSString. Native allocation/constructor failure conversion remains
part of the exception work in Task 5; these successful round trips do not test
out-of-memory behavior.

## Selector mapping and diagnostics

`check-selectors.sh` compares forward/reverse lookup, signature-specific
overrides, generic fallbacks, array parameter counting, inherited Objective-C
method reuse, nil inputs and replacement behavior. A reused inherited selector
matches its first name component even when the Java argument count differs.
Reverse lookup of an unknown selector caches its first component as a generic
mapping. Conflicting registrations log errors but still update their maps;
replacing one direction leaves the old entry in the other direction.

The same fixture compares normalized donor/reconstruction diagnostics for
duplicates and conflicts, custom banners, once-per-type suppression and reset.
Banner state and four complaint-type bytes are reconstructed from 0x53d043ac
and 0x53d04408. Table setup is separate from the later Java selector class/type
initialization; this test does not claim those Java wrapper classes work yet.

## Raw method lookup and invocation

`check-invocation.sh` compares 17 native cases against the donor. It exercises
instance/static and inherited lookup, declared-only filtering, optional
signatures, integers, long and double return words, both float argument layouts,
object identity and a thrown Java exception followed by a successful call.
The raw invocation helper returns the Java exception handle separately and
clears the VM's pending exception; Objective-C exception conversion is a later
layer and is not claimed by this fixture.

For long/double values, the helper's return is the first VM word and its
`highBits` output is the second. On i386 the donor's words are not ordered as
the parameter name suggests: the long fixture returns 1336557c, then 9fbad9f8;
the double fixture returns c0040000, then 00000000. Callers must reconstruct
values according to these observed semantics.

The donor's constructor-vararg entry takes a shortFloats flag, whereas the
reconstructed common VM entry has five arguments and always promotes float
arguments. The bridge-local constructor adapter described below accounts for
that ABI difference without changing the generic VM entry point.

## Constructor argument adapter

`check-constructors.sh` compares seven native cases: default construction,
promoted and short-float mixed primitive arguments, reference/array/float
arguments, a throwing constructor, missing-constructor lookup with an empty
argument area, and successful construction after clearing the exception.
`bridgeNewJavaObject` promotes four-byte floats into the double-width argument
layout accepted by the existing VM. It resolves the constructor before reading
the argument area, so missing-constructor errors do not dereference nonexistent
arguments. Java exceptions remain pending for the future Objective-C exception
conversion layer. This is raw constructor acceptance, not acceptance of
`+[NSJavaObject newWithSignature:]` or full framework startup.

## Class registry and generated classes

`check-class-map.sh` compares 24 donor transcript lines and normalized duplicate
registration diagnostics. Exact and inherited lookup, lazy Java class caching,
constructor selector/signature registration and default construction match.
The 32-byte JavaInfo layout and metaclass ivar cache follow the donor; root
classes use a separate registry. Records use ordinary allocation rather than
the donor's allocation pool.

Conversion registrations match the donor's callback slot order, duplicate and
unknown-class rejection, overload replacement/cache invalidation, and value-type
size registration. Type mappings reject overloads. These tests verify callback
registration, not invocation or object ownership. The donor overload entry
mutates an existing record's Java name without reinserting the hash key; that
behavior is retained rather than silently repairing it.

`check-classgen.sh` compares 12 donor transcript lines for the pure Java class
builder: runtime registration, class/metaclass versions, inherited instance
layout, method encodings, private method inclusion and dollar-name exclusion.
Generated instance, static integer and static float methods execute real Java
code in the reconstructed VM. The fixture supplies the dispatch implementations
and a Java-backed test superclass; production NSJavaObject dispatch and ownership
remain pending. Hybrid Objective-C/Java subclasses are outside this builder.
Neither fixture is full framework startup or packaging acceptance.

## Object identity registry

`check-object-map.sh` compares 11 donor transcript lines for the two non-owning
pointer maps. It checks both lookup directions, explicit retain/release flags,
replacement without inverse-entry cleanup, one-sided removal, missing and nil
lookups, and two native threads performing 2,000 map/lookup/remove cycles each.
Table operations share a cthreads mutex; retain/release messages occur after
unlocking, as in the donor. Initialization is separated from VM bootstrap and
is idempotent so later startup can initialize both registries safely.

The fixture uses opaque handle tokens: this proves registry semantics and
Objective-C retain/release behavior, not Java GC rooting or proxy lifetime.
Those remain part of the production NSJavaObject integration.

Dispatch investigation: `MorphObjcArgumentsForJava` (0x53d060f4) rejects array
arguments with NSInvalidArgumentException. Object arguments call
`_BRIDGEConvertObjcArgToJava` (0x53d08a14); type-only mappings receive the input
stack address and an output consumed-byte count, while ordinary references
use NSIdToJavaHandle and consume four bytes. Primitive slots consume four bytes,
except long/double at eight. This conversion layer is not yet implemented.

## Java retention adapter

`check-retention.sh` compares the donor's JavaGCInitialization,
NSRetainJavaObject and NSReleaseJavaObject with the reconstructed initializer
and raw retention adapters. Both run against an isolated test NSRuntime class
whose two private retention methods match the donor class's Hashtable bytecode.
The fixture is kept in its own class directory and is not a shipped replacement
for NSRuntime. Its count/value probes expose the table's actual contents.

Nine transcript lines verify returned handle identity, idempotent duplicate
retention, removal by one release, absent/nil release, and 100 retained values
remaining readable after System.gc(). The test does not prove prompt collection
after release or absence of conservative native-stack roots. A broken fixture
without the private methods produces the same initialization exception and
reason on both implementations. A reconstructed-only raw API check confirms
that retaining nil leaves NullPointerException pending and that release works
after clearing it. Public Objective-C exception conversion remains pending.

The donor NSRuntime static initializer hardcodes the system ObjCJava path. Its
retention logic can be tested independently, but production bootstrap must
adapt that class's loading path before it is safe to use with the private
framework. No donor bridge is loaded by the reconstructed retention tests.

## Exception conversion and checked calls

`check-exceptions.sh` compares 18 donor transcript lines plus normalized
secondary-exception diagnostics. NSJavaException preserves the raw Java handle,
roots it until deallocation, and archives its Foundation exception fields
without archiving the handle. The fixture verifies ordinary Java exceptions,
native exception names and mapped userInfo, null and `*nil*` messages, name
fallback, stack traces/descriptions, Java handle identity on round trip, and
suppression of an exception thrown by getMessage during conversion.

The checked JAVANewObject, __JAVAMethodInvoke and NSRetainJavaObject wrappers
now turn pending Java failures into Objective-C exceptions. The fixture checks
throwing methods/constructors, nil retention and successful subsequent calls.
The reverse raise helper also sets the expected pending Java exception, checked
through the recovered ExecEnv layout on both VMs.

Tests reuse the donor NativeException, NSException and NSMemoryOutputStream
Java classes; these dependencies contain no native library initialization.
The separate retention NSRuntime fixture still isolates bootstrap. Test-only
NSIdToJavaHandle/NSJavaHandleToId functions accept explicitly registered
identities and abort on an unmapped non-null object. General object wrapping
remains unimplemented and is not implied by the mapped userInfo checks.
Reconstructed loaded-image checks reject the donor JavaVM and ObjCJava images.

## Key-value access and object coding

`kvc-check.sh` compares donor and reconstructed accessors, inherited/private
fields, primitive boxing, numeric string conversion, assignment compatibility,
array/missing-key behavior and thrown exceptions. The getter method is the key
itself; setters use `set` plus the capitalized key. Component conversions are
limited to explicit string handling; general proxies are checked separately.

`coding-check.sh` round-trips cyclic Serializable objects, shared references
and nil through NSCoder and Java object streams. Donor and reconstructed
archives compare byte-for-byte. Nonserializable Java objects and unsupported
Objective-C serialization match the donor errors. Private NSCoder stream
natives are bound explicitly during SerializeInitialization. Persistent coder
maps and retained Java streams follow the donor's lack of teardown.

## Hybrid class generation and native constructor access

`hybrid-check.sh` compares two generated levels above an Objective-C native
superclass. Both class/metaclass use flag 0x200 and preserve inherited instance
size. Only the first generated level owns copy, retain, release, copyWithZone:,
valueForKey: and takeValue:forKey:. Copy dispatches Java clone; retain roots the
mapped Java object; release unroots at native retain count two, before calling
the first native superclass implementation. The fixture compares actual Java
dispatch, clone side effects, primitive KVC, native fallback and root counts.
The original pure-class builder and `check-classgen.sh` remain independently
linkable without the hybrid lifetime modules.

The package-private constructor case exposed a donor/runtime difference.
Donor libjava.A.dylib `execute_java_constructor_vararg` at 0x5c114584 performs
ordinary access checks; it is not universally privileged. Its
`VerifyFieldAccess` at 0x5c109ae4 returns true when current_class is null.
The shared reconstruction's classinitialize.c instead returns IsPublic(access)
for that case. The checked bridge `JAVANewObject` therefore passes PRIVILEGED_EE
only when there is no current Java method. It retains the original environment
for exception conversion and leaves shared VM access policy unchanged.

`hybrid-access-check.sh` preserves donor oracles for private/package/protected
constructors, a public constructor in a package-private class, private packed
float constructors, constructor exceptions, abstract classes and missing
constructors. Registered JNI callbacks reached through ordinary Java methods
also check that an unrelated Java caller cannot construct the private class,
while a same-class Java caller can. Thus native entry privilege does not bypass
access checks when a Java caller frame is present.

`hybrid-full-check.sh` adds end-to-end hybrid acceptance with the complete
production NSLoadVirtualMachine bootstrap and reconstructed NSRuntime. It uses
actual Java subclasses of the donor NSObject wrapper, not manual object maps
or replacement callbacks. Fourteen donor/reconstructed transcript lines cover
Objective-C init, C argument constructors, Java factory constructors, identity,
native hash/equals/description/KVC, a public native-superclass mapping, real
Java/native copying and independent state, and wrapping an existing native
instance. Explicit Java finalization followed by the production safe-release
queue drain verifies mapping removal, release of the Java-owned native retain,
and three native deallocations. This tests finalizer behavior without claiming
deterministic garbage-collector timing. Reconstruction rejects loaded donor
VM/bridge images, and stdout plus normalized stderr are compared freshly.

### Generated wrapper dependency boundary

`generated-wrapper-dependencies.json` inventories the retained 573 Apple Java
classes and their literal native-library loads. There are 408 native-declaring
classes with 8,813 native declarations. FoundationJava, AppKitJava, EO families,
printing, keyboard, networking/admin, and adaptor libraries are separate generated
OS wrapper libraries; retaining their bytecode does not reconstruct their natives.
No NSValue class is present in this archive. NSPoint/NSRange/NSSize/NSRect value
classes are pure Java; mutable wrapper classes must be assessed separately.

`wrapper-dependencies-check.sh` runs fresh full-bootstrap donor/reconstructed
processes for NSArray and NSData. The donor returns zero count/length; this
reconstruction reports `java.lang.UnsatisfiedLinkError:FoundationJava.B` and rejects
any loaded donor JavaVM or `/usr/lib/java` image. These generated wrapper libraries
are outside the reconstructed core embedding contract and are not silently loaded.

`core-key-check.sh` tests all nine alpha.core.keyValue native exports through Java
fixture native aliases, preserving object identity and donor errors. The retained
keyValue initializer itself requests `NativeObject.loadLibrary("ObjCJava")`; the
donor fails this request because it lacks `ObjCJavaInitialization`. Native aliasing
in the test isolates the exported native ABI from that obsolete loader defect.

### Core native and class ABI audit

The donor exports exactly 30 `Java_*_stub` entry points: NativeObject (12),
keyValue (9), NSRuntime (3), ObjectiveCSelector (2), and NSCoder streams (4).
The third NSRuntime entry, `nextRootPath`, is a legacy native absent from the
retained NSRuntime class; its exported ABI still returns NEXT_ROOT or an empty
Java string. NativeObject's 12 exports likewise include legacy methods absent
from the retained class.

`values-abi-check.sh` compares deprecated type-registration diagnostics, public
and private class conversions, constructor registration, reflection metadata,
explicitly named class creation, and legacy NEXT_ROOT against fresh donor output.
The donor's primitive field-signature buffers are not NUL-terminated; the fixture
compares descriptor-length bytes, and reconstructed buffers safely include a
terminator. Reflection flags exclude VM-private bits (mask 0xfff). This is a
memory-safety repair, not an additional reflection feature.

## Exported VM helpers and public data

`check-vm-helpers.sh` compares the full donor/reconstructed bootstrap for
`JAVAFieldLookup`, scalar/object array allocation, UTF-16 string construction
(including embedded NUL), subclass checks, the four typed NSNumber callbacks,
flat-namespace function lookup, and the empty donor runtime-registration hook.
The initial reconstructed run reported all eleven missing functions. The final
run matches the donor transcript and normalized diagnostics, including the
legacy signature-buffer API and all five additional exported data records.
`MapLock` is the actual identity-map mutex; the class caches are initialized by
MappingInitialization. Version strings retain the donor's 73.dev target metadata.

The deprecated `_NSJavaSignatureToObjc` preserves caller-buffer selection and its
exact complaint, but requires enough space for the actual encoding as well as
the donor's strlen(signature)+8 threshold. This retains the existing bounds
correction for signatures with many primitive arguments. `JAVANewString` returns
NULL on allocation failure instead of dereferencing a failed allocation.

Full production bootstrap also passes explicit Java GC while an Objective-C
proxy is retained, invocation/GC from an attached native worker, identity checks,
and final release/unmapping (`check-bootstrap-full.sh`). These runs use the real
reconstructed NSRuntime, with no retention substitute.

The packaged Release facade passes construction, strings, identity, KVC,
exception recovery, singleton behavior and defineClass through staged public
headers and `-framework JavaVM`, without DYLD_LIBRARY_PATH. Runtime acceptance
passes JNI callbacks, math/ZIP/network, AWT window creation/disposal and audio,
with loaded-image assertions. Relocation and final Debug acceptance are recorded
separately after the completed package checks.


## Final native acceptance and review

Tasks 3 through 6 are complete on the DR2 Intel guest. The final
`check-components.sh` run passed all 33 component and integration groups.
`core-exports-check.sh` bound all 155 donor public symbols within the
reconstructed ObjCJava image; this establishes export ownership, not exhaustive
input coverage. Individual component logs are kept separately because shared
redirected suite output exhibited NUL holes on this guest; explicit exit statuses
and the fresh isolated-log suite passed.

Both Release and Debug packages built successfully, including unchanged repeat
builds without recompilation. Both passed layout, private native imports,
Objective-C embedding, export ownership, JNI, math/ZIP/network, actual AWT
window creation/disposal, audio, and loaded-image provenance checks. Both also
passed relocation under `Relocated Frameworks With Spaces` with the original
framework temporarily hidden; restoration was verified afterward. Long paths
required a framework-only PATH_MAX native-library filename buffer. The generic
runtime retains its existing behavior.

A fresh whole-branch review of candidate tree
`19f38273c724fa0f0d87bb16d96e266aa43a46bd` against base `25341e0` found two
material build/test issues, both fixed with failing-then-passing native checks:

- `framework-build-dependencies.sh`: touching a VM header or the framework
  makefile now invalidates the custom runtime and bridge objects.
- `framework-fresh-check.sh`: runtime acceptance compiles its own desktop helper
  and passes for both Release and Debug from a context without prebuilt Release
  outputs. The complete acceptance driver uses this check.

After the final runtime change, generic `check-core check-libraries check-install`
passed again: console, numeric conversions, reflection, serialization, monitors,
interrupts, ZIP, math, resources, JPEG, JNI headers, debugger thread states,
donor-path exclusion, and privately installed launchers. The known ALIGN macro
redefinition warning remains non-fatal.

The core JavaVM/ObjCJava implementation is complete within the approved boundary.
FoundationJava/AppKitJava and EO/admin generated native wrapper libraries remain
separate unreconstructed dependencies; their absence and explicit failure are
tested, with no donor fallback. Completion does not imply these libraries work.
Donor defects remain compatibility behavior; portability beyond DR2 Intel was
not assessed. The public constructor float-varargs limitation is recorded below
and in `core-exports.md`.


The public `_NSCreateJavaObjectV` path unconditionally calls
`MorphObjcArgumentsForJava` (donor addresses 0x53d049d0 and 0x53d060f4).
Its float traversal advances four bytes even when `shortFloats=NO` and C
varargs promote float to eight-byte double. A following reference can therefore
be misread or crash. This donor limitation is preserved, not repaired. Raw
constructor adapter tests cover both float layouts; they do not establish
correctness of this public mixed-float/reference path. This finding comes from
donor disassembly review, not an executed crashing differential fixture.
