# JDK 1.1.5 Support Libraries (Phase 2) Roadmap

**Spec:** ../specs/2026-11-06-jdk115-reconstruction-design.md

**Status:** Complete within the documented Phase 2 headless scope.
Final evidence: 2026-11-06-jdk115-phase2-completion.md.

Phase 1 merged into master at 4996b33; bdedb1a fixes checkout-dependent coverage
fixture positions. Phase 2 starts from that verified VM on
build/jdk115-support-libraries, in the existing checkout and build directories.
The approved design governs scope, primary-binary fidelity, C89, 32-bit builds,
source-only commits and Windows/Linux parity. Each independently reviewable
library gets a focused implementation plan after its binary inventory.

## Order and acceptance

1. Console launchers: native2ascii first, then serialver, javadoc, javakey,
   rmic/rmiregistry and java-rmi.cgi. Recover each executable's argument vector;
   do not assume every launcher handles -J, JAVA_HOME or CLASSPATH identically.
   Verify real outputs, failures, installed-image discovery and tool-specific
   workloads. Network-dependent RMI execution closes with net.
2. zip: reconstruct the library API around the already retained zlib 1.0.4
   inflater. Add compression/CRC/streaming tests, malformed archive cases and
   jar create/list/extract interoperability with Sun and its rejection of the
   unsupported update option. Keep bootstrap ZIP
   loading green while separating library exports.
3. net: recover socket, address and datagram interfaces; test loopback TCP/UDP,
   timeouts, close/unblock and failed connects on both platforms. Avoid public
   network dependencies. Gate RMI registry/stub behavior here.
4. math and sysresource: inventory original exports and recover numeric and
   resource behavior, including exceptional numeric values and absent resources.
5. jpeg: stage unmodified IJG release 6 source with provenance, reconstruct
   Sun decoder glue, and compare pixels and truncated-input behavior. Source
   inspection determines the boundary that can run before AWT is available.
6. mmedia: recover Sun audio formats/Win32 playback and implement the approved
   real Linux backend. Separate deterministic decoding from device-dependent
   playback checks; an unavailable device is not evidence of playback success.
7. Integration: complete console-tool and i18n library tests, image installation,
   exact library export checks, all four CTest configurations, and fresh review.

Ruling: the five shipped i18n demos derive from AWT applets/frames. Phase 2 will
exercise their underlying formatting/collation/boundary APIs and preserve the
unmodified demo sources. The design's unchanged GUI demo milestone remains
dependent on Windows/Motif AWT; console coverage will not be reported as GUI
demo completion. No demo edits or fake AWT implementation are authorized here.

## Execution ledger

- Began primary analysis of native2ascii.exe and serialver.exe. Native2ascii
  forwards every argument unchanged after -ms8m and its Java class; serialver
  instead processes JAVA_HOME, CLASSPATH and -J, so it needs a separate slice.
- First implementation plan: 2026-11-06-jdk115-native2ascii.md.
- Native2ascii is implemented, installed and verified against Sun. Full suites
  pass Windows Release/Debug 84/84 each and Linux Release/Debug 79/79 each.
  Serialver's distinct launcher behavior guided its subsequent implementation.
- Console wrappers and CGI are implemented with oracle tests. Javadoc generates
  documentation; javakey identity persistence and real RMI calls are exercised.
- ZIP, net and sysresource are integrated. The initial complete Windows suites
  passed Release/Debug 97/97 each. A subsequent Linux UDP close race was traced
  to rereading the Java fd field after close clears it; native code now retains
  the original descriptor handle while blocked. Final matrix passes.
- Mmedia is integrated with exact decoder tests and real WinMM/OSS backends.
  Windows submission checks pass; WSL has no OSS device, so Linux playback is
  explicitly unverified. JPEG decoding and callbacks match Sun's oracle.
- Math uses the original bnlib 1.1 dependency, explicitly authorized by the user
  on 2026-11-06. Original GPL license and provenance are retained. Math and JPEG
  are integrated, and their independent Windows/Linux Release/Debug oracle
  comparisons pass. Final full-suite and installed-image gates pass as recorded
  in the completion evidence: Windows 106/106 per configuration; Linux 94 pass
  plus one explicit hardware skip per configuration; zero failures.
- I18n API oracle coverage is added. GUI demo execution remains deferred to AWT
  as ruled above. Javaverify is overall scope but outside the listed Phase 2
  milestone; its native entry point has only been inventoried.
