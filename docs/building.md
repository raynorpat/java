# Building and testing

The VM requires a 32-bit compiler. Reference distributions stay in ignored
`vendor/`; only our Java fixtures and their captured output are tracked.

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
ctest --test-dir out -C Release -LE jdk115 --output-on-failure
ctest --test-dir out -C Release -L jdk115 --output-on-failure
```

The `jdk115` tests currently fail: they measure reconstruction progress and are
not inverted with `WILL_FAIL`. The other 12 tests retain the 1.0.2 class library.
Repeat with `--config Debug` and `-C Debug` for debug verification.

## Linux

Install CMake, make, GCC and the distribution's 32-bit development libraries
(on Ubuntu: `gcc-multilib` and `libc6-dev-i386`). Stage the same vendor tree.

```sh
CC='gcc -m32' cmake -S . -B out/linux115 -DCMAKE_BUILD_TYPE=Release
cmake --build out/linux115
ctest --test-dir out/linux115 -LE jdk115 --output-on-failure
ctest --test-dir out/linux115 -L jdk115 --output-on-failure
```

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
