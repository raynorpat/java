# Rhapsody JavaVM.framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Status:** Complete; native DR2 verification and final review fixes are recorded in `tests/framework-dr2/intake.md`. Integration remains a separate decision.

**Goal:** Build the complete DR2 JavaVM.framework while preserving the generic JDK makefile.

**Architecture:** Reuse the reconstructed runtime and add the Objective-C framework facade and required ObjCJava bridge. A separate makefile produces a private versioned framework and native acceptance clients.

**Tech Stack:** DR2 Intel, Apple cc 771.4, GNU make 3.74, C and Objective-C, Foundation, existing Java 1.1.5 runtime.

**Spec:** `docs/superpowers/specs/2026-11-08-rhapsody-javavm-framework-design.md` (approved).

## Global Constraints

- Keep `build/rhapsody.mk` as the standalone Rhapsody JDK build.
- Add `build/rhapsody-framework.mk` to build a complete, linkable JavaVM.framework, including Objective-C embedding.
- Target the existing DR2 Intel toolchain and runtime.
- Build and test in a private destination without replacing the guest's system framework.
- Do not silently fall back to Apple's JavaVM or ObjCJava binaries.
- Preserve donor API semantics; adapt paths for private relocation.
- No placeholders count as completed bridge functionality.

## Review Focus

- Undefined and braced environment variables: preserve observed donor expansion (Task 2).
- Missing classes and non-Java receivers: match donor exceptions (Tasks 3–4).
- Primitive/64-bit/floating arguments and return values: preserve the i386 ABI (Task 4).
- Java GC while Objective-C retains a proxy: preserve identity and lifetime (Task 5).
- Spaces in relocated paths and duplicate library identities: load only the intended private libraries (Task 6).

## File and interface map

- `tests/framework-dr2/`: donor inventory, native differential clients, evidence and acceptance instructions.
- `src/rhapsody/java/framework/`: NSJavaConfiguration, NSJavaVirtualMachine, umbrella/public headers and framework resources.
- `src/rhapsody/java/bridge/`: ObjCJava implementation; private interfaces established from donor exports and developer headers before implementation.
- `build/rhapsody-framework.mk`: framework compilation, packaging and test targets; reuse generic rules.
- `docs/building.md`: both supported entry points.

### Task 1: Donor inventory and executable oracle

**Files:** `tests/framework-dr2/intake.md`, `donor-manifest.json`, `configuration.m`, `check-configuration.sh`.
**Interfaces:** Consume the guest JavaVM.framework and `/usr/lib/java/libObjCJava.A.dylib`; produce hashed binary inventory and deterministic native configuration observations for Task 2.

- [x] Recover and hash the missing bridge; enumerate functions/imports and dependencies with IDA.
- [x] Compile the configuration client against the original JavaVM framework. Assert vendor-specific lookup, application-over-domain precedence, nil missing values and observed expansion behavior.
- [x] Run the oracle in isolated defaults domains without modifying persistent user/system settings; record exact output and compiler invocation.
- [x] Commit the inventory and passing oracle.

### Task 2: Configuration implementation

**Files:** `src/rhapsody/java/framework/NSJavaConfiguration.{h,m}`, Task 1 test files.
**Interfaces:** Export `NSDefaultJavaVendorKey`, `NSDefaultJavaVMKey`, `NSDefaultJavaCompilerKey`, `NSDefaultJavaClassPathKey`; implement all methods declared by the donor NSJavaConfiguration header, including `-valueForKey:` and `+vendorNames`.

- [x] Compile the same client against reconstructed sources; observe failure before sources exist.
- [x] Implement the three lookup layers, vendor selection, memory ownership and donor environment expansion.
- [x] Compare reconstructed and donor output for ordinary, missing, empty, braced and adjacent environment variables. Check vendorNames duplicates/top-level keys against the donor rather than assuming a filtered set.
- [x] Run `sh tests/framework-dr2/check-configuration.sh` in the guest; expect identical transcripts and `PASS framework configuration`.
- [x] Commit the verified implementation and evidence.

### Task 3: Framework facade and bridge startup

**Files:** `src/rhapsody/java/framework/NSJavaVirtualMachine.{h,m}`, `JavaVM.h`, `Info-nextstep.plist`, `tests/framework-dr2/embedding.m`, initial bridge sources.
**Interfaces:** Public donor NSJavaVirtualMachine and NSObject category signatures; bridge entry points `NSLoadVirtualMachine` and `NSAddClassFile`, with exact argument/return ABI recovered before coding.

- [x] Add donor fixtures for singleton creation, default classpath, non-Java construction errors, class lookup and defineClass:withName:.
- [x] Record donor startup dependencies and VM adapter requirements; pin exact bridge interfaces in evidence before implementing them.
- [x] Observe the reconstructed embedding fixture fail; implement real startup against the reconstructed VM and facade behavior.
- [x] Pass the fixture without donor JavaVM/ObjCJava loading; commit.

### Task 4: Java classes, constructors and dispatch

**Files:** `src/rhapsody/java/bridge/`, `tests/framework-dr2/BridgeFixture.java`, `embedding.m`.
**Interfaces:** NSJavaObject developer-header API, Objective-C runtime class registration, JVM method signatures and donor i386 bridge calling convention.

- [x] Add donor/reconstructed fixtures covering class lookup, inheritance, overloaded constructors, static/instance calls, all primitive types, references, arrays and invalid signatures.
- [x] Run reconstructed fixtures to establish failures; implement class registration, construction and method dispatch using recovered bridge interfaces.
- [x] Verify exact values (including long/double and negative values), error behavior and dynamic dependency provenance; commit.

### Task 5: Ownership, exceptions and thread entry

**Files:** bridge sources; `tests/framework-dr2/ownership.m`, `BridgeFixture.java`.
**Interfaces:** Existing proxy handle/class API, VM GC roots, thread attachment and NSJavaObject coding/key-value behavior recovered from the donor.

- [x] Add differential fixtures for repeated wrapping, retain/release around Java GC, exception propagation, supported field/key-value access, coding and calls from an additional native thread.
- [x] Observe failures; implement the corresponding donor lifetime, conversion and thread behavior.
- [x] Run fixtures repeatedly in fresh processes; document donor limitations and account for required generated wrappers; commit.

### Task 6: Complete framework build and acceptance

**Files:** `build/rhapsody-framework.mk`, framework resources/launcher, `tests/framework-dr2/check-framework.sh`, `docs/building.md`.
**Interfaces:** Generic build's VM/libraries/classes and Tasks 2–5 embedding implementation; versioned framework layout and private install destination.

- [x] Add a failing client build using staged `-F` headers and `-framework JavaVM`, plus layout and loaded-library assertions.
- [x] Implement the separate build entry point and package versioned framework/headers/resources/Commands/Classes/Libraries/Home with canonical library copies.
- [x] Verify Release and Debug console/JNI/embedding/AWT/audio checks and relocation to a path containing spaces; reject donor Java components in dynamic loads.
- [x] Recheck generic install/launch behavior and document both build commands.
- [x] Commit acceptance evidence, obtain whole-branch review, fix material findings with regression tests and leave integration explicit.

The donor investigation in Tasks 1 and 3 determines private bridge signatures. Record those discoveries before coding dependent tasks; do not guess an interface to make the plan look complete.
