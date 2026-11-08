#!/bin/sh
set -eu
build=out/dr2-framework-tests/bootstrap
mkdir -p "$build/classes" "$build/lib"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java tests/framework-dr2/BootstrapRuntime.java
cc $flags -c src/rhapsody/java/bridge/runtime.m -o "$build/runtime.o"
cc -traditional-cpp -dynamiclib "$build/runtime.o" src/rhapsody/java/bridge/diagnostics.m tests/framework-dr2/bootstrap-runtime-boundary.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/lib/libObjCJava.dylib"
cc -dynamiclib tests/framework-dr2/bootstrap-library.c -o "$build/lib/libBootstrapFixture.A.dylib"
cp "$build/lib/libBootstrapFixture.A.dylib" "$build/lib/libBootstrapFallback.A.dylib"
cp "$build/lib/libBootstrapFixture.A.dylib" "$build/lib/libBootstrapMissing.dylib"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" out/dr2/bin/java -Dcom.apple.yellow.foundation.ObjCJava="`pwd`/$build/lib/libObjCJava.dylib" -classpath "$build/classes:vendor/jdk1.1.5/lib/classes.zip" BootstrapRuntime "`pwd`/$build/lib" > "$build/runtime.txt" 2> "$build/runtime.err"
diff tests/framework-dr2/bootstrap-runtime.expected "$build/runtime.txt"
if out/dr2/bin/java -Dcom.apple.yellow.foundation.ObjCJava="`pwd`/$build/absent/libObjCJava.dylib" -classpath "$build/classes:vendor/jdk1.1.5/lib/classes.zip" BootstrapRuntime "`pwd`/$build/lib" > "$build/missing.txt" 2>&1; then
    echo 'FAIL missing private bridge accepted'
    exit 1
fi
grep 'UnsatisfiedLinkError' "$build/missing.txt" >/dev/null
echo 'PASS NSRuntime private loading and search paths'
