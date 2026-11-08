#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/coding-classes" "$build/coding-retention"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/coding-retention" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/coding-classes" tests/framework-dr2/CodingFixture.java
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c tests/framework-dr2/coding-bind.c -o "$build/coding-bind.o"
cc -traditional-cpp -DDONOR tests/framework-dr2/coding.m "$build/coding-bind.o" -framework Foundation -o "$build/coding-donor"
"$build/coding-donor" "$build/coding-retention:$build/coding-classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/coding-donor.txt"
cat "$build/coding-donor.txt"
diff tests/framework-dr2/coding.expected "$build/coding-donor.txt"
cc $flags -c src/rhapsody/java/bridge/coding.m -o "$build/coding-adapter.o"
cc $flags tests/framework-dr2/coding.m "$build/coding-adapter.o" src/rhapsody/java/bridge/exceptions.m src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/coding"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/coding" "$build/coding-retention:$build/coding-classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$build/coding.txt"
diff "$build/coding-donor.txt" "$build/coding.txt"
cmp "$build/coding-donor.archive" "$build/coding.archive"
echo 'PASS bridge coding'

