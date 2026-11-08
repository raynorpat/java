#!/bin/sh
set -eu
build=out/dr2-kvc-tests
mkdir -p "$build/classes" "$build/retention"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/KVCFixture.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/retention" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -DDONOR tests/framework-dr2/kvc.m -framework Foundation -o "$build/donor"
"$build/donor" "$build/retention:$build/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/donor.txt" 2> "$build/donor.err"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c src/rhapsody/java/bridge/key-value.m -o "$build/key-value.o"
cc $flags -c src/rhapsody/java/bridge/exceptions.m -o "$build/exceptions.o"
cc $flags tests/framework-dr2/kvc.m "$build/key-value.o" "$build/exceptions.o" src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/object-map.m src/rhapsody/java/bridge/class-map.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/reconstructed"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/reconstructed" "$build/retention:$build/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$build/reconstructed.txt" 2> "$build/reconstructed.err"
diff "$build/donor.txt" "$build/reconstructed.txt"
sed 's/^.*] //' "$build/donor.err" > "$build/donor.diagnostics"
sed 's/^.*] //' "$build/reconstructed.err" > "$build/reconstructed.diagnostics"
diff "$build/donor.diagnostics" "$build/reconstructed.diagnostics"
echo 'PASS bridge key-value access'
