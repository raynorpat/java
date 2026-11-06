# JDK 1.1.5 math and sysresource

Approved parent: `../specs/2026-11-06-jdk115-reconstruction-design.md`.

1. Inventory Sun Release/Debug exports and Rhapsody names. Compare the
   discovered Colin Plumb bnlib implementation with original source before
   reconstructing Sun's byte-array wrappers.
2. Add Java 45.3 oracle cases for large signed arithmetic, quotient/remainder,
   modular powers/inverses, prime generation and resource URL access. Confirm
   missing libraries fail before adding implementation.
3. Implement C89 standalone libraries with exactly Sun's 12 math and three
   sysresource exports; retain existing javai ABI. Resource wrappers use the
   recovered 260-byte buffers and existing VM resource helpers.
4. Verify original Sun versus reconstructed output and exact exports, Release
   and Debug on Windows and Linux. Integrate through separate CMake fragments.

No Java class library changes. No new VM exports are expected. Local builds
use `out/math-agent-*`; parent owns common CMake integration and full suites.
