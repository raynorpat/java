#!/bin/sh
set -eu
b=out/dr2-framework-tests
mkdir -p "$b/nib-classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/nib-classes" tests/framework-dr2/NibFixture.java tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc -traditional-cpp -DDONOR tests/framework-dr2/nib.m -framework Foundation -o "$b/nib-donor"
"$b/nib-donor" "$b/nib-classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$b/nib-donor.txt" 2> "$b/nib-donor.err"
cat "$b/nib-donor.txt"
diff tests/framework-dr2/nib.expected "$b/nib-donor.txt"
cc $flags -c src/rhapsody/java/bridge/nib.m -o "$b/nib-adapter.o"
cc $flags tests/framework-dr2/nib.m "$b/nib-adapter.o" src/rhapsody/java/bridge/key-value.m src/rhapsody/java/bridge/class-map.m src/rhapsody/java/bridge/exceptions.m src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/object-map.m -Lout/dr2/lib -ljavai -framework Foundation -o "$b/nib"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$b/nib" "$b/nib-classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/nib.txt" 2> "$b/nib.err"
diff "$b/nib-donor.txt" "$b/nib.txt"
sed 's/^.*] //' "$b/nib-donor.err" > "$b/nib-donor.diagnostics"
sed 's/^.*] //' "$b/nib.err" > "$b/nib.diagnostics"
diff "$b/nib-donor.diagnostics" "$b/nib.diagnostics"
echo 'PASS nib outlet bridge'
