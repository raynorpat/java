# JDK 1.1.5 native2ascii Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan. Steps use checkbox syntax for tracking.

**Goal:** Build and install Sun-compatible native2ascii on Windows and Linux.

**Status:** complete. Windows Release/Debug 84/84 each; Linux Release/Debug
79/79 each. Installed-image oracle harness passes on both platforms. See
2026-11-06-jdk115-native2ascii-evidence.md for binary and review evidence.

**Architecture:** Reuse the Phase 1 simple tool wrapper, calling java_main with
java, -ms8m, sun.tools.native2ascii.Main and unchanged user arguments. The Java
converter is the approved vendor classes.zip; no replacement Java library.

**Tech Stack:** C89, MSVC Win32/GCC -m32, CMake 3.0...3.10 and CTest.

**Spec:** ../specs/2026-11-06-jdk115-reconstruction-design.md, Phase 2 console tools.

## Global Constraints

- Keep 32-bit Windows/Linux Release and Debug green; retain original VM exports.
- Primary Windows binaries determine behavior; no guessed launcher options.
- Vendor binaries stay ignored. Commit only source, our fixtures and evidence.
- Use short subsystem-prefixed commits, without metadata.

## Review Focus

UTF-8 non-ASCII input, reverse Unicode escapes, stdin conversion, paths with
spaces, and option-like filenames/-encoding errors are covered in the one
oracle harness below. Normalize CRLF only, retaining all other output bytes.

### Task 1: native2ascii launcher and oracle gate

**Files:** Modify CMakeLists.txt; reuse src/share/java/javai/tool_main.c unchanged;
create tests/tools/native2ascii115.cmake, native2ascii115.in,
native2ascii115.expected and docs/superpowers/plans/2026-11-06-jdk115-native2ascii-evidence.md.

**Interfaces:** Existing wrapper consumes TOOL_CLASS, TOOL_SIMPLE and default
TOOL_HEAP=-ms8m. Produce native2ascii executable/install target and
tools-native2ascii115 CTest. Harness consumes TOOL, ROOT and OUT paths.

- [x] Capture Sun's UTF8 forward/reverse/stdin bytes and error status/output
  for -J-ms16m and a missing -encoding argument; record primary entry at 401000.
- [x] Run the retained harness against an absent reconstructed native2ascii;
  require failure naming the missing launcher.
- [x] Add native2ascii to the existing tool loop and image installation;
  define TOOL_SIMPLE and TOOL_CLASS="sun.tools.native2ascii.Main".
- [x] Run the same harness against Sun and reconstructed tools. Build/test all
  four configurations, including the complete inherited CTest suite.
- [x] Review the focused branch diff, record results, and commit
  `tools: add the JDK 1.1.5 native2ascii launcher`.

This initial Phase 2 deliverable does not complete the support-library roadmap.
