# Phase 2 completion evidence

Completed on 2026-11-06 on build/jdk115-support-libraries, following the approved
support-library roadmap and its explicit AWT dependency ruling.

## Delivered

All six native libraries are built and installed: zip, net, math, sysresource,
jpeg and mmedia. Windows export lists match Sun exactly: 21, 29, 12, 3, 1 and
3 entries respectively. The javai export checks remain unchanged and pass.
Original zlib 1.0.4, IJG release 6 and user-authorized bnlib 1.1 dependency
sources retain their provenance/licenses; installed images include notices.

Console tools include native2ascii, serialver, javadoc, javakey, rmic,
rmiregistry and java-rmi.cgi alongside existing jar/javap/javah/javac.
Tests cover archive interoperability, generated documentation, isolated
identity persistence and DSA key generation, CGI behavior, generated RMI
stubs/skeletons and real loopback registry calls. I18n formatting, parsing,
collation and boundaries match the original class-library oracle.

## Final gates

| Configuration | Result |
|---|---|
| Windows x86 Release | 106 passed, zero failed |
| Windows x86 Debug | 106 passed, zero failed |
| Linux i386 Release | 94 passed, one device skip, zero failed |
| Linux i386 Debug | 94 passed, one device skip, zero failed |

Complete CTest suites ran without test exclusions. Logs are in ignored out/
as phase2-final-{win,linux}-{release,debug}-{build,tests}.log. All new Java
fixtures are class-file version 45.3. The existing self-hosting javac suite
passes in every configuration.

Windows and Linux Release installations pass tests/tools/phase2-image115.cmake
using installed executables, class libraries, properties and all six native
libraries. JPEG's test-only bridge is staged separately so its decoder
dependency resolves from the installed image. Logs are
out/phase2-final-image-{win,linux}-tests.log. License files are present in both
images under legal/.

Independent reviews covered native bridges, launchers, CMake, tests, math/JPEG
ABI and original-binary evidence. Findings were fixed and re-reviewed: tests
now honor JDK115_DIR; UDP receive/peek retain their descriptor and reject close
before publishing a sender; POSIX timeout waits use poll for high descriptors;
and missing Sun network error messages are restored. Regression cases failed
before each fix and pass afterward. No review blockers remain.

## Boundaries retained

- WSL has no /dev/dsp. Linux OSS code builds and deterministic audio tests pass,
  but device playback is unverified and the device test explicitly skips.
  Windows WinMM accepts repeated silence streams; audible output is not claimed.
- The five original i18n GUI demos require AWT. They remain unchanged and their
  GUI execution/visual checks belong to the later AWT milestones. Headless
  JPEG callback tests do not claim AWT rendering coverage.
- Javaverify, agent/jdb, JdbcOdbc, AWT and Rhapsody execution remain outside this
  Phase 2 milestone. Their overall reconstruction scope is unchanged.
- Historical behavior remains documented in subsystem evidence, including jar's
  unsupported update mode, STORED ZIP CRC behavior and bnlib's original assert.

The only staged whitespace-check diagnostic is a retained trailing space in
the original Sun JPEG decoder comment; upstream source was not reformatted.
