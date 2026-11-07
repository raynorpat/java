# Phase 5: debugger and JDBC-ODBC reconstruction

**Goal:** Restore the shipped jdb/agent debugger on Windows and Linux and the
JDBC-ODBC bridge on Windows, using the unchanged JDK 1.1.5 Java classes.

**Architecture:** Recover the original native interfaces from Sun binaries;
reuse the existing VM services and launcher. Keep the agent a separate native
library. Use the Windows ODBC driver manager for the bridge.

**Tech stack:** C89, 32-bit MSVC/GCC, CMake/CTest, IDA, original classes.zip.

**Spec:** ../specs/2026-11-06-jdk115-reconstruction-design.md, Phase 5.

**Global constraints:** No placeholder exports or copied vendor native DLLs in
the reconstructed image. Preserve source notices. Do not modify vendor inputs,
global DSNs, firewall settings, or unrelated VM behavior. Platform adaptations
must be explicit. Debug/Release and Linux verification remain separate gates.

**Review focus:** Legacy stack/handle ABI, debugger thread/monitor ordering,
breakpoint restoration, ODBC buffer ownership, SQL NULL and diagnostic handling.

## Task 1: establish interfaces and restore jdb launcher

- [x] Recover agent/JdbcOdbc export manifests from both binary configurations.
- [x] Record jdb entry-point behavior from IDA; capture original help, version,
  and rejected -J argument output/status in tests/tools/jdb-* fixtures.
- [x] Add tests/tools/jdb115.cmake; observe failure without the launcher.
- [x] Add jdb to CMakeLists.txt using tool_main.c with TOOL_SIMPLE, empty heap
  option, and sun.tools.ttydebug.TTY. Include it in installation.
- [x] Verify launcher fixtures on Windows Debug/Release and Linux; commit.

Interfaces: tools/exports-agent115.txt (47 exports),
tools/exports-JdbcOdbc115.txt (69 exports), native argv to java_main.
This task establishes launch/argument behavior, not a working debug session.

## Task 2: reconstruct agent native operations

- [x] Generate Agent/BreakpointHandler headers and stubs from original classes;
  compare with manifests and the legacy BREAKPTS code in runtime/agent.c.
- [x] Recover the 1.1.5 implementations into a separate shared agent source
  target, with exact exports. Audit existing VM debug hooks before changes.
- [x] Add original-versus-rebuilt fixtures for class/field/method enumeration,
  slots, stack locals, line tables, object IDs, and exception stack traces.
- [x] Implement and verify each operation group before committing that group.

Interfaces: original stack_item native stubs, VM debug.h services; proposed
cmake/agent115.cmake and src/share/java/debug/agent115.c. Determine exact class
layouts and callbacks from IDA/generated headers before implementation.

## Task 3: debugger lifecycle and end-to-end sessions

- [x] Recover agent initialization, thread exclusion/suspension, callbacks,
  breakpoint insertion/removal, and single-step behavior.
- [x] Add a bounded loopback harness and Java debuggee with known source lines;
  verify launch/attach, break, stack/local inspection, step, continue, and exit.
- [x] Repeat Windows Debug/Release and Linux; exercise disconnect/exit while
  stopped and repeated sessions. Commit only demonstrated behavior.

Recover configuration differences explicitly: Release agent.dll does not import
debugger-hook registration, while Debug does. Require full sessions on the
debug-capable runtime and original-equivalent behavior in other configurations.

Interfaces: unchanged sun.tools.debug wire protocol/classes, VM debug hooks,
jdb launcher and reconstructed agent. Every process must be owned and bounded.

## Task 4: reconstruct Windows JDBC-ODBC bridge

- [x] Generate native signatures from sun.jdbc.odbc.JdbcOdbc and recover all
  69 wrappers, including parameter buffer lifetime and status arrays.
- [x] Add cmake/odbc115.cmake and src/win32/sun/jdbc/odbc115.c, linking odbc32.
- [x] Test environment/connection/statement lifecycle and invalid-DSN diagnostics
  against Sun first, then prepared parameters, result conversion, metadata,
  transactions, SQL NULL, truncation, and cleanup with a local driver fixture.
- [x] Inventory installed 32-bit drivers before selecting a temporary database
  fixture. Record any unavailable coverage; do not call the phase complete with
  driver-dependent behavior untested. Verify exact exports; commit by group.

## Task 5: integrate and close Phase 5

- [x] Verify installed images contain reconstructed agent/jdb on both platforms
  and JdbcOdbc on Windows; run real debug sessions and bridge tests from images.
- [ ] Run relevant VM regressions and complete platform suites in both configs.
- [ ] Audit all exports and evidence, obtain branch review, resolve findings,
  and mark completion only when all Phase 5 gates pass.

Status: Tasks 1-4 complete. ODBC implementation and installed-image evidence
are committed as f0ff16e and 2a47ef6; native agent recovery is 0662a2c and JNI
exception callbacks are a3b8839. Debugger sessions and installed-image checks
pass on Windows and Linux. Final complete regression gates remain in progress.
Native recovery evidence is in tests/native/agent115-evidence.md,
tests/native/debugger115-evidence.md, and tests/odbc/PROVENANCE.md. Phase 5 began
from merged Phase 4 commit 0d97dea.
