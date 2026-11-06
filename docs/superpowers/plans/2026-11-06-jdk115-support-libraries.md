# JDK 1.1.5 Support Libraries (Phase 2) Roadmap

**Spec:** ../specs/2026-11-06-jdk115-reconstruction-design.md

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
   jar create/update/extract interoperability with Sun. Keep bootstrap ZIP
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
  Serialver is next; its distinct launcher behavior has been identified.
- All six support libraries and the remaining console tools are still open.
