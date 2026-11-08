#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/retention-classes" "$build/retention-broken"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/retention-classes" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/retention-broken" tests/framework-dr2/retention-broken/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -DDONOR tests/framework-dr2/retention.m -framework Foundation -o "$build/retention-donor"
"$build/retention-donor" "$build/retention-classes:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/retention-donor.txt"
diff tests/framework-dr2/retention.expected "$build/retention-donor.txt"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c src/rhapsody/java/bridge/retention.m -o "$build/retention-adapter.o"
cc $flags tests/framework-dr2/retention.m "$build/retention-adapter.o" src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c -Lout/dr2/lib -ljavai -framework Foundation -o "$build/retention"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/retention" "$build/retention-classes:vendor/jdk1.1.5/lib/classes.zip" > "$build/retention.txt"
diff "$build/retention-donor.txt" "$build/retention.txt"
"$build/retention-donor" "$build/retention-broken:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" broken > "$build/retention-broken-donor.txt"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/retention" "$build/retention-broken:vendor/jdk1.1.5/lib/classes.zip" broken > "$build/retention-broken.txt"
grep '^missing-methods=NSInternalInconsistencyException:' "$build/retention-broken-donor.txt"
diff "$build/retention-broken-donor.txt" "$build/retention-broken.txt"
cc $flags -Isrc/rhapsody/java/bridge tests/framework-dr2/retention-errors.m "$build/retention-adapter.o" src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c -Lout/dr2/lib -ljavai -framework Foundation -o "$build/retention-errors"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/retention-errors" "$build/retention-classes:vendor/jdk1.1.5/lib/classes.zip"
echo 'PASS bridge retention adapter'
