# DR2 ObjCJava export audit

`core-exports.tsv` inventories the **155 public symbols** in the donor
`libObjCJava.A.dylib`: 118 ordinary functions, 30 native entry points, and seven
data symbols. Addresses are donor virtual addresses. Names retain the Mach-O
leading underscore; C source identifiers omit that one prefix.

The inventory was recovered from IDA's public-name records, independently of
the reconstruction's source list. It excludes local `j_` import/jump veneers,
CRT imports, and Objective-C method implementation labels. Objective-C class
methods remain a separate behavioral contract, tested by the proxy, forwarding,
hybrid, and bootstrap fixtures. FoundationJava and AppKitJava are separate
donor libraries and are outside this native core inventory.

The source column identifies implementation definitions, including the
`NATIVE(name)` expansion in `native-object.m` and `KEY_NATIVE` expansion in
`key-value-natives.m`. All 155 identifiers now have source definitions.
A source match establishes
implementation coverage, not proof that every input has been tested. `MISSING`
means the exact exported identifier has no definition in the audited sources;
similar internal helpers and comments do not count as implementations.

## Verification relevant to this audit

- `core-exports-check.sh` builds and loads the reconstructed bridge with the
  private VM core, binds all 155 exact manifest symbols, rejects duplicate or
  missing entries, and verifies each resolved address lies in a load-command
  segment of the intended ObjCJava image. It rejects loaded donor images and
  confirms the intended private core image is present. This checks function and
  data export ownership without calling uninitialized functions. The client
  links the private bridge normally; `NSAddLibrary` on this DR2 build instead
  forces duplicate CRT `__objcInit` definitions to bind before the check runs.
- `forward-check.sh` compares original and reconstructed proxy forwarding,
  explicit invocation exports, selector-name exports, mapped return callbacks,
  EBX structure-return addressing, native response fallback, validation, and
  coder hooks. It checks generated returns of zero, four, eight, and twelve-byte
  mapped types and both exact mapping-error diagnostics.
- `selector-native-check.sh` runs the real Java `ObjectiveCSelector` APIs under
  full bootstrap. It compares scalar/object/void returns, numeric/object/string/
  selector arguments, unsupported encodings, missing numeric accessors,
  parameter counts, native exceptions, and `implementedByClass`.
- `coding-check.sh` compares cyclic/shared-reference Java serialization,
  nil/error behavior, and archive bytes. `native-object-check.sh` checks wrapper
  lifecycle and native entry points. `nib-check.sh` checks native, proxy, and
  hybrid outlet dispatch. Full hybrid tests additionally cover real bootstrap
  and automatic native wrapping without converter/map seams.

All reconstructed integration clients assert that the original ObjCJava and
JavaVM framework images were not loaded where the fixture is intended to be
self-contained. The original Java wrapper classes remain runtime inputs.

## Recovered compatibility details

`_NSInvokeJavaMethod` and `_NSInvokeStaticJavaMethod` take four arguments: Java
target, Objective-C selector, argument-vector pointer, and aggregate destination.
The decompiler omits the fourth argument because its use occurs in a switch
branch classified as data; donor instructions read it at `ebp+0x14`.
`_BRIDGEStructReturnAddress` returns the incoming EBX value. Mapping callbacks for
aggregates larger than eight bytes receive their output location in EBX.

`_NSJavaClassRespondsToSelector` deliberately ignores its native-class argument.
The object counterpart first checks Java, then dispatches `respondsToSelector:`
starting at the first native ancestor without the generated-hybrid `0x200`
class flag. Both reject a nil selector.

The ObjectiveCSelector donor interprets numeric-accessor return words as a
double even for `intValue`, `longValue`, and `floatValue`. Tests preserve the
observable results: Integer 17 and Long 115 become zero; Float 1.5 becomes
0.125; Double 1.25 remains 1.25. It rejects Objective-C `i` encodings, rejects
structure arguments/returns, and its `selectorImplementedByClass` native always
returns false. These behaviors are recovered limitations, not placeholders.

Other donor limitations recorded by differential tests are truncation of the
forwarded Java long high word, omission of four/eight-byte structure copyback
by `forwardInvocation:`, and `StreamCorruptedException` in the full Foundation
proxy archive fixture. Raw Java coder serialization independently succeeds and
matches archive bytes. The donor proxy archive fixture explicitly binds the
donor's own private coder stream natives, which its original startup otherwise
fails to resolve; this is a fixture-only binding seam.


The public `_NSCreateJavaObjectV` path unconditionally calls
`MorphObjcArgumentsForJava` (donor addresses 0x53d049d0 and 0x53d060f4).
Its float traversal advances four bytes even when `shortFloats=NO` and C
varargs promote float to eight-byte double. A following reference can therefore
be misread or crash. This donor limitation is preserved, not repaired. Raw
constructor adapter tests cover both float layouts; they do not establish
correctness of this public mixed-float/reference path. This finding comes from
donor disassembly review, not an executed crashing differential fixture.
