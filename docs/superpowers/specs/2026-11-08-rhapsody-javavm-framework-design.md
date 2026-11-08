# Complete Rhapsody DR2 JavaVM.framework

Status: approved by the user on 2026-11-08; implementation and native DR2 acceptance complete. See `tests/framework-dr2/intake.md` for evidence and the external native-wrapper boundary.

## Requested result

Keep `build/rhapsody.mk` as the standalone Rhapsody JDK build. Add
`build/rhapsody-framework.mk` to build a complete, linkable JavaVM.framework,
including Objective-C embedding. The user explicitly selected the complete
framework rather than packaging the current runtime alone.

Target the existing DR2 Intel toolchain and runtime. Reuse the reconstructed
interpreter, native libraries, tools and audited class archives. Build and test
in a private destination without replacing the guest's system framework.

## Evidence and implications

The donor `Versions/A/JavaVM` contains NSJavaVirtualMachine,
NSJavaConfiguration, NSObject's InstantiatingJavaObjects category, and
NSFramework_JavaVM. Its resource plist names NSJavaVirtualMachine as the
principal class. IDA identifies 41 functions including import stubs.

The framework delegates VM creation and Java class bridging to
`/usr/lib/java/libObjCJava.dylib`. At 0x5cd01168, initWithClassPath: loads that
library and resolves NSLoadVirtualMachine. At 0x5cd01278, findClass: calls
NSClassFromString. The framework's NSObject category rejects non-Java classes;
the bridge must provide real Java object construction and dispatch.

The extracted donor tree lacks libObjCJava, but the running guest has
`/usr/lib/java/libObjCJava.A.dylib` (110280 bytes) and its unversioned symlink.
Developer headers provide NSJavaObject, java-objc, java-vm, vm-interface and
vm-apple contracts. Recover this binary and its dependency inventory before
implementing the bridge; do not substitute guessed behavior.

The guest confirms top-level Classes, Commands, Home and Libraries directories,
with Headers, JavaVM and Resources links through Versions/Current. Match this
DR2 layout rather than importing modern macOS framework conventions.

## Build design

The separate framework makefile will reuse the generic build's compilation
rules, with separate output directories and explicit framework targets. Keep
any necessary shared-rule extraction minimal. Existing generic commands and
install layout remain supported.

Build the JavaVM framework executable from reconstructed Objective-C sources.
Reconstruct the required ObjCJava bridge against the existing VM, using donor
behavior and the developer headers to establish its ABI. Package bridge and
runtime libraries privately, with one physical copy of each loaded library;
compatibility names should be links. DR2's flat dyld namespace makes duplicate
library loading a correctness issue.

Supply public headers, Info-nextstep.plist, classes, command launchers, Home
compatibility paths and native libraries. Resolve private dependencies and
Java home from the staged framework location. Preserve donor API semantics;
the intentional path adaptation makes the private framework relocatable.
Do not silently fall back to Apple's JavaVM or ObjCJava binaries.

## Behavior to reconstruct

- NSJavaConfiguration vendor selection, lookup precedence, defaults domains,
  system configuration and environment-variable expansion.
- NSJavaVirtualMachine singleton behavior, class path selection, VM creation,
  findClass: and donor defineClass:withName: behavior.
- Java class registration with the Objective-C runtime, signature-based object
  construction, method dispatch and primitive/reference conversion.
- Object identity and lifetime across Java GC and Objective-C retain/release,
  exception propagation, thread entry and the bridge's supported object APIs.
- Required bridge dependencies identified from the donor. Foundation/AppKit
  operating-system libraries remain platform dependencies. Inventory generated
  Java wrapper libraries separately; any needed to satisfy the embedding
  contract must be accounted for before declaring completion.

Do not infer that unrelated Enterprise Objects, database-adaptor or system
administration products belong to this framework simply because their bridge
libraries reside in the same /usr/lib/java directory.

## Execution and verification

1. Preserve donor hashes, symbols, dependency inventory and native oracle
   fixtures for configuration and embedding. Record unsupported donor cases.
2. Implement configuration and the framework entry points, checking observable
   results against the donor in separate processes.
3. Implement the bridge in tested increments: VM startup, class lookup,
   constructors, method calls, conversions, ownership and exception paths.
4. Add the dedicated makefile and package layout; compile an Objective-C client
   using the staged headers and `-framework JavaVM`.
5. Validate Release and Debug framework images in the native guest, relocate
   to a path containing spaces, and run console, JNI, Objective-C embedding,
   AWT and audio acceptance checks appropriate to the shared runtime.
6. Recheck generic build/install behavior. Document both entry points and
   commit completed increments with their verification evidence.

Acceptance requires functional Java construction and calls from Objective-C,
configuration/error-path parity, and evidence that private clients load the
reconstructed Java components. Inspect both linked and dynamically loaded
dependencies: link-command success alone cannot establish this. Framework
packaging or passing console tests alone does not constitute completion.

## Remaining investigation

The full ObjCJava implementation and generated-wrapper dependencies have not
yet been mapped. The initial reconstruction tasks must resolve those details
before their source interfaces are finalized. The existing Phase 7 results
cover the generic runtime and AWT implementation, not this embedding bridge.
