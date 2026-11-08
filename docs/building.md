# Building and testing

The VM requires a 32-bit compiler. Reference distributions stay in ignored
`vendor/`; only our Java fixtures and their captured output are tracked.
Python 3 is required for the deliberately patched circularity and coverage test
fixtures. Normal builds and test compilation use the reconstructed javac; Sun
executables are needed only to recapture reference results.

## Stage reference material (Windows)

```powershell
cmake -DFROM=C:/Users/raynorpat/Downloads/test/Sun-JDK-1.1.5-Windows/jdk1.1.5 -P tools/vendor.cmake
cmake -DFROM=C:/Users/raynorpat/Downloads/test/DR2-Java-Kit -DTO=rhapsody-dr2 -P tools/vendor.cmake
```

An alternate reference tree can be selected with `-DJDK115_DIR=<directory>`.
It must contain `lib/classes.zip`.

## Windows

```powershell
cmake -S . -B out -G "Visual Studio 18 2026" -A Win32
cmake --build out --config Release
ctest --test-dir out -C Release --output-on-failure
cmake --build out --config Debug
ctest --test-dir out -C Debug --output-on-failure
```

Run each configuration's build and tests together: the legacy native fixture DLL
is shared between configurations. The complete suite uses the 1.1.5 class library;
no progress tests are excluded or inverted. Windows export checks require exactly
286 Release names and 287 Debug names, matching the reference DLLs.

## Linux

Install CMake, make, GCC and the distribution's 32-bit development libraries
(on Ubuntu: `gcc-multilib` and `libc6-dev-i386`). Stage the same vendor tree.

```sh
CC='gcc -m32' cmake -S . -B out/linux115 -DCMAKE_BUILD_TYPE=Release
cmake --build out/linux115
ctest --test-dir out/linux115 --output-on-failure
```

Use a separate `out/linux115-debug` tree with `-DCMAKE_BUILD_TYPE=Debug` for
Debug verification.

The Phase 1 C89 gate additionally sets `-DCMAKE_C_EXTENSIONS=OFF` and appends
`-pedantic` to `CMAKE_C_FLAGS`. GCC calls the resulting language mode `c90`,
equivalent to `c89`. Original 64-bit types and native function-pointer adapters
still produce pedantic warnings; compilation does not require C99 syntax.

Missing `crti.o` or `-lgcc` at link time means the 32-bit development libraries
are missing.

On this WSL host, `gcc-multilib` conflicts with the installed PowerPC cross
compiler. Keep `gcc-15-multilib` and `libc6-dev-i386` explicitly installed instead;
the PowerPC compiler can then remain installed. Configure with the x86 system
header directory explicitly (the generic metapackage normally supplies the
`asm` header link):

```sh
CC='gcc -m32' cmake -S . -B out/linux115 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS=-isystem/usr/include/x86_64-linux-gnu
```

## Linux Motif AWT

Add the extracted Solaris JDK 1.1.8/16 directory as source material. It must
contain `SUNWjvrt/reloc/usr/java1.1/lib/classes.zip`; both original class archives
remain unchanged. On Ubuntu the native build needs `libmotif-dev:i386`,
`libx11-dev:i386` and `libxt-dev:i386`. Tests also need `libxtst-dev:i386`,
`xvfb`, `xauth`, `openbox`, core X11 fonts and Ghostscript. Demo capture additionally
uses `xdotool`, `x11-utils` and ImageMagick. Use an EWMH window manager for decorated
windows; see [the host compatibility boundary](../tests/awt118/compatibility.md).

```sh
cmake -S . -B out/motif-debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 \
  -DSOLARIS118_DIR=/path/to/solaris-jdk-1.1.8
cmake --build out/motif-debug --parallel
ctest --test-dir out/motif-debug --output-on-failure
cmake --install out/motif-debug --prefix "$HOME/jdk115-motif"
```

The installed launcher automatically uses `lib/motif-peers.zip`, reconstructed
`lib/libawt.so`, and the retained font/print resources and notices. Do not copy
Solaris native libraries into the image. Use a separate Release build directory.
Do not rebuild/relink a build tree while its tests are running.

For the unchanged demo gate, generate the existing shared inventory on Windows:

```powershell
cmake -DROOT=D:/raynorpat/java -DOUT=D:/raynorpat/java/out/demo-inventory115 -P tests/tools/winawt-demos115.cmake
```

Then run the Linux capture helper against the original demo tree (or a verified
byte-for-byte copy on WSL's Linux filesystem):

```sh
xvfb-run -a -s '-screen 0 1280x1024x24 -nolisten tcp' \
  sh tests/awt118/with-wm.sh python3 tests/awt118/demos.py \
  out/demo-inventory115/manifest.json vendor/jdk1.1.5/demo \
  "$HOME/jdk115-motif" out/motif-demo-captures \
  --probe out/motif-debug/lib/libmotif118xprobe.so
```

`--select` accepts a case-ID regular expression. Each case records interaction,
process/close behavior, controls and screenshots of every client window. Review
the rendering as well as `executionPassed`; screenshots alone are not a visual
parity assertion. Eight original Frame-only mains intentionally require bounded
termination, matching the Windows demo policy. The observer and viewer entry
helper are separate test classes; the test-only native probe identifies each
Window precisely even when several applets overlap. No demo class or HTML is
changed. Toolkit initialization precedes loading the probe, so the installed
runtime and AWT library supply execution.

## Oracle fixtures

Compile from the fixture directory: Sun's compiler rejects forward-slash source
paths on this Windows host.

```powershell
Push-Location tests/vm
& ../../vendor/jdk1.1.5/bin/javac.exe -classpath ../../vendor/jdk1.1.5/lib/classes.zip Hello115.java
Pop-Location
cmake -DCLASS=Hello115 -P tools/capture-expected.cmake
```

Capture fails on nonzero exit or timeout and writes LF-normalized combined
stdout/stderr. The runner compares that output exactly and requires exit zero.
`java_test` accepts `VM_ARGS` before the harness classpath and class name, and
`ARGS` after the class name. Platform-dependent fixtures need separate expected
outputs before they can be used on both systems.

## Reconstruction gates

The suite includes original structure layouts, JNI and invocation, native-thread
attachment, class unloading, allocation caches, monitor reclamation, finalizer
ordering, arithmetic boundaries, resources, serialization, reflection and process
behavior. `javac-selfhost` recompiles the Java fixtures and reruns their captured
oracles. `vm-JCov115` checks Debug coverage generation and merging; Release accepts
and ignores the coverage option, as Sun did. Native writer tests cover compiler
seeds and test bitmaps. The audit ledger and binary-evidence documents are under
`docs/superpowers/plans/`.

## Rhapsody DR2 Intel

The native DR2 build uses Apple cc and GNU Make rather than CMake. On the host,
package the audited original AppKit classes separately from the baseline archive:

```sh
python tools/rhapsody115-peers.py vendor/jdk1.1.5/lib/classes.zip vendor/rhapsody-dr2 out/dr2-peers
```

Stage repository text with LF endings and preserve binary vendor/archive bytes.
Inside the private Intel guest, from the source root:

```sh
/bin/gnumake -f build/rhapsody.mk image
/bin/gnumake -f build/rhapsody.mk check-types check-core check-libraries
/bin/gnumake -f build/rhapsody.mk check-install INSTALL_DIR=/private/tmp/jdk115-installed
/bin/gnumake -f build/rhapsody.mk check-installed-awt INSTALL_DIR=/private/tmp/jdk115-installed DESKTOP_PID=<Workspace-PID>
```

Use `DEBUG=1 BUILD=out/dr2-debug` for a separate Debug build. The image contains
reconstructed native libraries and the original 1.1.5 classes.zip supplemented
by rhapsody-peers.zip. Launchers select RToolkit and private libraries without
changing the guest's JavaVM.framework. GUI tests launched over SSH use the
logged-in Workspace bootstrap namespace through the test-only desktop-launch
helper. See [guest setup and acceptance](../tests/awt-dr2/guest-build.md).

Build the demo observer outside the installed image:

```sh
/bin/sh tests/awt-dr2/build-demo-helpers.sh /private/tmp/jdk115-installed out/dr2-demo-helpers
```

The host-side `tests/awt-dr2/demos.py --help` describes SSH, QMP, manifest, helper
and output arguments. Use the shared 81-case demo inventory described above,
original demo files, and a private QEMU guest. Inspect each client capture as well
as the recorded interactions and cleanup. The SoundKit device is unavailable in
the current test guest; controlled conversion/retry tests verify native audio
behavior, but physical playback is not established.
