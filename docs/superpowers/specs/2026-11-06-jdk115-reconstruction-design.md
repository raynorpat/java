# JDK 1.1.5 reconstruction — design

## Intent

The tree currently holds Sun's JDK 1.0.2 VM (from Netscape's 3.0 sources), cleaned of
Netscape code and ported to Win32 and POSIX. The goal is to carry it forward to **JDK 1.1.5**,
reconstructing Sun's 1.1.5 native code by decompiling the shipped binaries, so the tree builds
a complete 1.1.5 JDK — VM, every `bin/` tool, and AWT — on Windows, Linux/Unix, and finally
Apple Rhapsody DR2.

**Success:** every demo shipped with the Windows JDK 1.1.5 runs on our build **with no edits
to the demo**: the 20 applets in `demo/` (via `appletviewer`), the three `demo/awt-1.1` demos,
and the five `demo/i18n` demos.

## Decisions made by the owner

- 1.1.5 **replaces** the 1.0.2 VM; 1.0.2 is the starting point, not kept in parallel.
- Reconstruction is **as faithful as possible** to Sun's 1.1.5 source: Sun's file names,
  function names, structures and constants wherever the binaries/headers reveal them.
- Where reconstructed Sun code and our existing code disagree, **the reconstruction wins**,
  except bug fixes (e.g. the GC fixes in `runtime/gc.c`), which are kept and marked as
  deviations. Our POSIX pthreads thread layer is kept behind an `#ifdef` alongside the
  reconstructed Unix thread code.
- **All of the 1.1.5 `bin/`** is in scope: `java`/`javaw`/`jre`/`jrew`, `javac`, `javah`,
  `javap`, `javadoc`, `jar`, `jdb` (+ `agent`), `appletviewer`, `rmic`, `rmiregistry`,
  `java-rmi.cgi`, `javakey`, `native2ascii`, `serialver`, `javaverify`, and the `JdbcOdbc` bridge.
- **AWT on three platforms:**
  1. Windows: reconstruct `winawt.dll`.
  2. Linux/Unix: reconstruct and port **X11/Motif** AWT using the newly supplied
     Solaris JDK 1.1.8 release/debug binaries, with explicit 1.1.5 compatibility checks.
  3. Rhapsody: reconstruct Apple's **AppKit** AWT (`libawt.A.dylib` + `com.apple.rhapsody.awt`),
     built and tested on a real Rhapsody DR2 install (the owner's DR2 VM image). This comes
     **after** the main reconstruction is complete and ready for build testing.

## Goals, non-goals, anti-goals

**Goals**
- Reconstructed C source for every native library in the 1.1.5 JDK: `javai` (VM, incl. JNI,
  reflection, the 1.1 verifier and GC), `net`, `zip`, `math`, `mmedia`, `jpeg`, `sysresource`,
  `agent`, `JdbcOdbc`, `winawt`, and the tool launchers.
- One CMake build producing a JDK 1.1.5-shaped image (`bin/`, `lib/`, `include/`) on Windows and
  Linux, with `_g` debug variants where Sun shipped them (Debug config is acceptable).
- The CTest suite extended to cover the demos wherever they can be checked automatically.

**Non-goals**
- Rebuilding the Java class library from source. Sun's 1.1.5 `lib/classes.zip` is used as-is
  (it contains classes such as `sun.awt.windows.*` that have no source in `src/`).
  Phase 8 is an explicit later exception: reconstruct the Java changes needed to port
  audited 1.1.8 fixes and behavior, while preserving both original archives as inputs.
- 64-bit support (the VM stays 32-bit, as today).
- JIT compilers, Java 1.2+ features, Swing (beyond what a demo needs, which is none).

**Anti-goals** (failure even if it "works")
- Editing a demo to make it run.
- Code that reads as a rewrite when the binary shows what Sun wrote: names, structure and
  control flow should follow the disassembly, not our own taste.
- Presenting guessed code as Sun's: anything not recoverable from the binaries (local names,
  comments, ordering of statics) is plain code, and genuinely invented parts are marked
  `/* reconstructed */` (or a shortcut comment) so they're never mistaken for original.
- Losing the bug fixes we already found (GC fixes, etc.).

## Source material

All under `C:\Users\raynorpat\Downloads\test\`.

**Windows JDK 1.1.5** — `Sun-JDK-1.1.5-Windows\jdk1.1.5\`
| File | Size | Exports | Notes |
|---|---|---|---|
| `javai.dll` / `javai_g.dll` | 409 KB / 561 KB | 286 | VM. `_g` names 26 source files in assert strings: breakpoints, check_code, class, classinitialize, classloader, classresolver, classruntime, executeJava, field, finalize, gc, inline, interpreter, jni, linker_md, method, monitor, monitor_cache, monitor_md, reflect, selftest, thread, threads, threads_md, util, wrap |
| `winawt.dll` | 229 KB | 166 | **C++**: `awt_*.cpp` (Button, Canvas, Component, Frame, Graphics, Toolkit, …), `Hashtable.cpp`, `ObjectList.cpp`, `CmdIDList.cpp`, `image.cpp` |
| `net.dll`, `zip.dll`, `math.dll`, `mmedia.dll`, `jpeg.dll`, `sysresource.dll`, `agent.dll`, `JdbcOdbc.dll` | 6–71 KB | 1–69 | `jpeg.dll` is the IJG libjpeg plus Sun's decoder glue |
- `include/` — 1.1.5 VM headers incl. `jni.h`, `sys_api.h`, `include/win32/*_md.h`.
- `src/` — Java source for `java.*` and `sun.tools` (644 files); no `sun.awt` source.
- `lib/classes.zip` — 1656 entries incl. `sun/awt/windows`, `sun/awt/image`.
- `demo/jre/` — C source of the `jre` launcher (Sun original).
- `lib/*.properties` — `awt.properties`, `font.properties*`, etc. for the image.

**Rhapsody DR2 (Intel), JDK 1.1.5** — `DR2-Java-Kit\` (also universal i386/ppc copies in `DR2\`)
- `JavaVM.framework/Libraries/lib{java,awt,agent,debugit,jpeg,math,mmedia,net,sysresource,zip}.A.dylib`,
  Mach-O i386. `libjava` keeps **538 global symbols** and uses **Mach cthreads**
  (`cthread_fork`, `mutex_try_lock`, `sysThreadVMSuspend`, …).
- `Classes/classes.jar`, `Classes/awt.jar` (`com.apple.rhapsody.awt.R*Peer`), headers in
  `System/Developer/Java/Headers` (`vm-sun.h`, `java-vm.h`, …), tools in `usr/bin`.
- IDA databases already exist for several of these libraries.

**Mac OS X Server 1.2v3** (`Frameworks\JavaVM.framework`, PowerPC, JDK 1.1.6) — secondary cross-check only.

**Solaris JDK 1.1.8/16** — `C:\Users\raynorpat\Downloads\solaris-jdk-1.1.8`.
The `SUNWjvrt` and `SUNWjvdev` packages supply the runtime classes, release/debug
native libraries, tools and resources. Phase 6 uses the X11/Motif material first;
Phase 8 audits the full supplied distribution for changes applicable to this tree.
See the [Solaris intake](../plans/2026-11-06-solaris118-inventory.md) for inventory
and provenance. Original binaries and archives remain read-only, uncommitted inputs.

**Already in the tree:** Sun's **1.0.2 Windows AWT in C++** (`awt/windows/*.cpp`, unbuilt) is
the starting point for `winawt`, exactly as the 1.0.2 VM is for `javai`. `classsrc/` (1.0.2
classes and `genconst.c`) is retired once the 1.1.5 `classes.zip` replaces it; git history keeps it.

**My calls from the review:**
- **Tools follow the 1.1.5 binaries:** `javah` (62 KB) stays a native C program, upgraded to
  1.1.5; `javap`, `javac`, `jar`, `jdb`, `appletviewer` etc. (~5 KB each) are launchers around
  Java classes in `classes.zip`, so the 1.0.2 C `javap` is retired. Existing tests keep their
  test cases but run against the 1.1.5 tools, with classes recompiled where needed.
- **`jpeg`:** the DLL contains IJG libjpeg **release 6 (2-Aug-95)**. Use IJG's published v6
  source unmodified (it is what Sun compiled) and reconstruct only Sun's decoder glue.
- **Linux stays green from step 1:** the Unix platform code moves with each VM change rather
  than waiting for step 3; step 3 then covers only the reconstructed cthreads variant and
  Unix-only libraries.

**How the sources combine:** the Win32 binaries are the primary truth for 1.1.5 shared code;
Rhapsody's symbols name things the stripped Windows DLLs don't; the 1.0.2 tree supplies the
code Sun carried forward unchanged, so most work is "diff 1.0.2 against the 1.1.5 binary,
function by function"; `include/` gives the structure layouts.

## Constraints

- **C89** for all C code, compiled with MSVC (32-bit) and GCC (`-m32`, `-std=c89`-clean).
  **Exception (owner's decision):** `winawt` is C++, as Sun wrote it; the Rhapsody AWT is
  Objective-C, as Apple wrote it. Nothing else may use C++ or Objective-C.
- **Source only in git (owner's decision).** Sun's and Apple's binaries and data (1.1.5
  `classes.zip`, `lib/*.properties`, demos, original DLLs, Rhapsody libraries and
  jars) are copied into a folder inside the repo working tree that is **git-ignored**, never
  committed. Shipped *source* — Sun's 1.1.5 `include/` headers and `demo/jre/*.c` — is
  committed into the tree, since it is the reconstruction's own source. CMake and CTest read them from there. (My call: `vendor/jdk1.1.5/` and
  `vendor/rhapsody-dr2/`, with `vendor/` in `.gitignore`; the configure step fails with a clear
  message if they are missing.) The committed `classsrc/classes.zip` (1.0.2) goes away with the
  rest of `classsrc/`. IJG libjpeg v6 is source, so it may be committed.
- **Linux native libraries (owner accepted my recommendation):** `mmedia` plays real audio
  through a simple Linux backend (builder's call, e.g. OSS-compatible `/dev/dsp` or ALSA);
  `agent`/`jdb` works on Linux; `JdbcOdbc` is Windows-only.
- **Rhapsody VM (owner confirmed):** Rhapsody runs our reconstructed Sun VM with the cthreads
  platform code, plus only the extra exports Apple's `libawt` and `awt.jar` need. Apple's own
  VM changes and the `JavaVM.framework` layout are not reconstructed, at least at first.
  Build and test happen in the owner's **QEMU virtual machine running Rhapsody DR2**.
- **32-bit only.**
- **CMake** (`3.0...3.10`) and CTest, as today, on Windows and Linux. Sun's makefiles are not
  available and are not recreated.
- Keep the current portability work: Win32 and POSIX builds both stay green after every step.
- Test classes compiled with the 1.1.5 `javac` (class file 45.3). Once our own `javac` works,
  it should be usable for this too.
- Commit conventions per `AGENTS.md` (short, subsystem-prefixed, no metadata).

## Shape of the result (builder's latitude within these lines)

- **Source layout follows Sun's 1.1 tree**: platform-independent code under `src/share/...`
  (e.g. `src/share/java/runtime`, `src/share/java/include`, `src/share/sun/...`), platform
  code under `src/win32/...` and a Unix side, with the AppKit AWT under its own Rhapsody
  directory. The exact Unix and Rhapsody directory names are the builder's call (my call:
  `src/unix`, `src/rhapsody`). The current `runtime/`, `md/`, `md-include/` move into this
  layout as their files are reconstructed.
- **Unix thread model:** the reconstructed Unix platform code follows Rhapsody's (Mach
  cthreads) under one `#ifdef`; our existing pthreads layer under another, which is what
  Linux builds use.
- **Motif AWT:** needs both native code and Java peer classes (`sun.awt.motif.*`, toolkit
  `sun.awt.motif.MToolkit`, the name 1.0.2's properties already use). The owner has now
  supplied Solaris JDK 1.1.8/16 with native release/debug AWT libraries and Motif classes.
  Audit those peers against 1.1.5 before selecting a compatible peer-only archive or
  reconstructing Java 1.1 peers compiled by 1.1.5 `javac`. Keep both original class
  archives unchanged. Build against Motif and X11 on Linux. See the
  [Solaris intake](../plans/2026-11-06-solaris118-inventory.md) for provenance and gates.
  The owner chose X11 AWT first, followed by a separate full Solaris 1.1.8 reconstruction;
  the current Phase 3 work and the 1.1.5 image's version target remain unchanged.
- **Rhapsody AppKit AWT:** Java classes from Apple's `awt.jar` used as-is; the native
  `libawt` reconstructed. Rhapsody has no CMake, so it gets its own build files runnable with
  the DR2 developer tools (builder's call: a plain Makefile).

## Suggested order (my call; the plan may refine)

1. **VM core (`javai`) to 1.1.5 on Win32**: interpreter, class loading, verifier, GC, monitors,
   threads, JNI, reflection, with 1.1.5 `classes.zip`. Milestone: console programs and the
   1.1.5 `javac` run (compile the test classes with our own VM).
2. **Support libraries**: `zip`, `net`, `math`, `sysresource`, `mmedia`, `jpeg`. Milestone: the
   i18n demos and non-GUI tools (`jar`, `javadoc`, `javap`, `javah`, `native2ascii`, `serialver`,
   `javakey`, `rmic`/`rmiregistry`).
3. **Unix port of the 1.1.5 VM**: reconstructed Unix platform code (cthreads `#ifdef`) plus the
   pthreads layer for Linux; Linux runs everything from steps 1–2.
4. **`winawt`**: Windows AWT reconstruction. Milestone: all applet and awt-1.1 demos on Windows.
5. **`agent`/`jdb`, `JdbcOdbc`**.
6. **Motif AWT** for Linux. Milestone: all demos on Linux.
7. **Rhapsody**: build the tree on DR2, reconstruct AppKit `libawt`. Milestone: demos on DR2.
8. **Port Solaris JDK 1.1.8 changes and fixes into the reconstructed tree** (owner requested).
   Audit the supplied 1.1.8 binaries and classes against the reconstructed 1.1.5 baseline,
   then port applicable changes across the VM, native libraries, Java classes and tools.
   This follows the existing phases without renumbering them; Phase 6 remains focused
   on completing Motif AWT. Reconstructing a complete Solaris-targeted JDK remains a
   separate follow-up from integrating its shared changes into our portable tree.

### Phase 8 scope and completion gates

- Inventory every supplied native library, launcher, class and relevant resource.
  Compare methods, descriptors, fields, constants and bytecode as well as native
  control flow. Distinguish version changes from Solaris/Windows platform differences,
  compiler artifacts and changes already incorporated during Motif reconstruction.
- Maintain a delta ledger with source hashes, binary addresses or class/method references,
  observed behavior, affected tree locations and a disposition for each difference:
  ported, already present, platform-specific, or unresolved. Do not infer an upstream
  bug fix merely from different disassembly. Unresolved applicable changes block completion.
- Reconstruct and integrate applicable 1.1.8 changes in small subsystem commits,
  retaining the tree's existing correctness fixes and portability layers. Cover native
  and Java changes together where they alter layouts, native signatures or calling
  conventions. Do not limit the audit to AWT or to previously known bug fixes.
- For changed Java behavior, add the necessary reconstructed source and reproducible
  class-image build steps. Keep the original 1.1.5 and Solaris 1.1.8 archives unchanged;
  do not silently overlay classes with incompatible native or serialization contracts.
  Record the resulting image's version and compatibility contract explicitly; selected
  backports alone do not establish complete JDK 1.1.8 compatibility.
- Add a regression for each observable fix or behavioral change, establish the original
  behavior using the available 1.1.5 and 1.1.8 oracles, and verify the reconstructed result.
  Where executing Solaris is necessary, arrange an appropriate oracle environment;
  structural evidence alone does not close a runtime behavior gate.
- Finish with the unchanged demo matrix, tools, JNI, serialization and class-library
  checks, and full Windows/Linux Debug/Release suites. Recheck the Rhapsody build and
  demos for shared changes introduced after Phase 7. Preserve the source-only repository
  and ensure no vendor native libraries enter the reconstructed images.

**Milestone:** all inventoried applicable 1.1.8 changes are integrated and verified,
all platform-specific exclusions are explained, the resulting class/native boundary
is reproducible, and no unresolved applicable delta or regression remains.

## Verification

- Each reconstructed library is checked against the original by behaviour: the same tests run
  with Sun's `javai.dll`/DLLs and with ours (Windows) must give the same output. Where useful,
  exports must match the original's export list.
- Demos: an automated check that each demo starts and runs without an exception for a few
  seconds (applets via `appletviewer`, headless where possible: Xvfb on Linux); a manual visual
  pass for the GUI demos at each AWT milestone, compared side by side with Sun's 1.1.5 on Windows.
- Existing tests keep passing on Windows and Linux throughout.

## Left to the builder

- The function-by-function reconstruction order inside each library.
- Decompiler/tool choice (IDA databases already exist; Ghidra or IDA both fine).
- Exact test harness for the demos.
