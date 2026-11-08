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
