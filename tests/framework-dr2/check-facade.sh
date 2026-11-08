#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp tests/framework-dr2/embedding.m -framework Foundation -framework JavaVM -o "$build/embedding-donor"
CLASSPATH=fixture-path "$build/embedding-donor" > "$build/embedding-donor.txt"
diff tests/framework-dr2/embedding.expected "$build/embedding-donor.txt"
unset CLASSPATH || :
"$build/embedding-donor" startup > "$build/embedding-startup-donor.txt"
diff tests/framework-dr2/embedding-startup.expected "$build/embedding-startup-donor.txt"
if test "${1-}" = donor; then
    echo 'PASS donor embedding'
    exit 0
fi
cc -traditional-cpp -DRECONSTRUCTED -Isrc/rhapsody/java/framework tests/framework-dr2/embedding.m src/rhapsody/java/framework/NSJavaConfiguration.m src/rhapsody/java/framework/NSJavaVirtualMachine.m -framework Foundation -o "$build/embedding"
CLASSPATH=fixture-path "$build/embedding" > "$build/embedding.txt"
diff "$build/embedding-donor.txt" "$build/embedding.txt"
if "$build/embedding" missing-bridge > "$build/embedding-missing-bridge.txt" 2> "$build/embedding-missing-bridge.err"; then
    echo 'FAIL: missing private bridge unexpectedly loaded'
    exit 1
fi
# DR2 dyld terminates the process before NSAddLibrary can return on this error.
grep 'out/dr2-framework-tests/Libraries/libObjCJava.dylib' "$build/embedding-missing-bridge.err" > /dev/null
grep 'No such file or directory' "$build/embedding-missing-bridge.err" > /dev/null
echo 'PASS framework facade (startup requires reconstructed bridge)'
