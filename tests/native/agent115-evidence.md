# JDK 1.1.5 debugger agent reconstruction

## Binary evidence and ABI

The native surface is the 47 exports in `tools/exports-agent115.txt` from
`vendor/jdk1.1.5/bin/agent.dll` and `agent_g.dll`. Headers and complete stubs are
regenerated with the reconstructed `javah` from the unchanged `classes.zip`.
The implementation is a separate `agent` shared library; vendor native binaries
are only oracle inputs, never installed into the reconstructed image.

Debug IDA database: `b95a32e49c13`; Release: `48c41bc19a95`. The working decompile
capture is `out/agent115-recovery/agent-debug.txt` (not a source input).

| Debug address | Recovered behavior |
| --- | --- |
| 1000106D | agent initialization, exit callback, debugger hook registration |
| 100010A6–10001408 | exclude registered threads, find sticky BreakpointHandler, queue monitor handshake, exception catch PC, report application exit |
| 1000144E | invoke target `main(String[])` |
| 10001475–10001918 | stack frame metadata, skip foreign frames, local variable live ranges |
| 10001988–1000200B | typed instance/static slots, signature types, validation exceptions |
| 10002133–100024C5 | methods, fields (including slot gaps), loaded classes |
| 1000256C–100025E2 | VM opcode lookup and breakpoint insertion/removal |
| 10002631–10002862 | line/PC mapping, excluding class initializers |
| 1000296B–10002B94 | object/array/boolean/integer/long/float/double stack locals |
| 10002BD8 | formatted exception stack trace |
| 10002DB3–10002DE6 | single stepping and source name |
| 10002E2E–100031A1 | typed field and stack setters |
| 100031DE–10003576 | sorted line table and first method line |
| 10003650–10003849 | suspension, system-thread registry, daemon exclusion and thread status |
| 10003950–1000433A | generated legacy stack-item wrappers |

The old `runtime/agent.c` supplies the retained Sun notice and compatible source
structure for metadata operations. It is not used as the 1.1.5 ABI definition.
Recovered differences include:

- A Class handle is used directly, never cast from its body.
- `makeslottable` returns an error code (`-5` for allocation failure), not a table.
- Stack frames add method signature and absolute PC. Local live ranges exclude
  their upper endpoint, and foreign frames are skipped without consuming indices.
- Float stack reads consume a float slot, not a double.
- Breakpoint natives return void and delegate to the VM's opcode preservation
  table. The old direct byte overwrite and old-opcode argument are not used.
- Single-step changes delegate to `set_single_step_thread`.
- `lineno2pc`, `pc2lineno`, and line enumeration exclude `<clinit>`.
- Line enumeration suppresses adjacent repeats before sorting, not all duplicates.
- Release initialization raises `IllegalAccessError: not a debugging VM`, as
  observed in the original Release launch/attach session; it imports no hooks.

## Platform adaptations

All VM structure access uses this tree's declarations. Sun's Thread daemon field
is at byte 28; this VM's generated Thread layout has it at byte 24. The registry
oracle explicitly uses the appropriate layout. Both single-step fields are at 20.

Windows status uses the recovered system-thread state values. The pthread port
maps its suspension stop bits and waiting monitor to the corresponding debugger
states. Unix symbol visibility is limited to the same native export manifest.

The local-variable allocator checks the returned array handle, correcting the old
source's test of the output parameter itself. `getMethodLinenumber` rejects a
negative slot instead of dereferencing before the method table.

## Verification fixtures

- `agent115-bindings`: generated wrapper names match all 47 original exports.
- `agent115-exports` (Windows): actual DLL exports exactly match that manifest.
- `agent115-registry` / `agent115-registry-sun`: registry insertion, duplicates,
  removal order, daemon exclusion, handle identity, and step queries through
  stack-item wrappers. Its five small wrappers live in tests, not production.
- `agent115-stack`: primitive/object/array locals through real VM structures,
  foreign-frame skipping, absent frame, dead thread, and unavailable slot errors.
- `agent115-metadata`: initialized VM and Java fixture, all typed/static slots,
  field/method/class enumeration, signatures/source, sorted lines and PC mapping.
- `agent115-jni` / `agent115-jni-sun`: the identical JNI-only probe against either
  runtime and agent, with no native layout assumptions. Covers primitive/static
  slots, object/array slots, enumeration, source/lines, invalid slot/type errors,
  and exception stack trace. A Java wrapper establishes the genuine debugger
  package caller for package-private metadata constructors.
- `tests/debugger/session115.py`: separately maintained loopback integration
  exercises actual debugger lifecycle, breakpoints, stacks, locals, stepping,
  continue, termination and original Release rejection.

TDD observations: the initial registry fixture failed to link its five missing
natives before implementation; the stack fixture failed with eleven missing
operations; the metadata fixture failed with eight missing operations after its
VM invocation linkage was corrected; the full debugger session failed because
no agent library existed before the shared target was implemented.

Windows Debug/Release and Linux validation results are recorded after completing
those runs; passing an isolated native group is not evidence that a full debugger
session or the entire Phase 5 suite passes.

## Observed results (2026-11-07)

Windows: `ctest --test-dir out/phase5-agent -C Debug -R '^agent115-'`
and the corresponding Release command each passed all ten agent tests,
including both original-binary oracle tests and the exact DLL export check.
Debug build log: `out/phase5-agent/probes-debug-final.log`; Release:
`out/phase5-agent/probes-release-final.log`.

Full protocol sessions and broad VM regression results are maintained by the
Phase 5 integration work; this native evidence does not substitute for them.

The stack probe also accepts an absolute agent DLL path and drives all eleven
stack getter/setter wrappers with legacy stack-item arguments. The same probe
passed against original Release/Debug and reconstructed Release/Debug libraries,
including 64-bit values, float width, object/array handles, foreign-frame skip,
returned stack pointers and an untouched sentinel. These runs are registered as
`agent115-stack-shared` and `agent115-stack-sun` on Windows.

Linux: final native suites passed all six tests in both `out/linux115` (Release)
and `out/linux115-debug` (Debug), including real metadata, JNI operations and
exact dynamic exports. Logs: `out/phase5-agent/linux-release.log` and
`out/phase5-agent/linux-debug.log`. The java/java_g (Debug), javac, jdb and net
prerequisites were rebuilt for the separate end-to-end session gate.

Windows import inspection confirmed that only Debug imports `set_debugger_hooks`;
Release does not. No vendor native library is an install source.
