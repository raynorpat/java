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
