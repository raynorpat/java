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
