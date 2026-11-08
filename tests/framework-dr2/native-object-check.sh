#!/bin/sh
set -eu
b=out/dr2-framework-tests
mkdir -p "$b/native-object-classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/native-object-classes" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc -traditional-cpp -DDONOR tests/framework-dr2/native-object.m -framework Foundation -o "$b/native-object-donor"
"$b/native-object-donor" "$b/native-object-classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$b/native-object-donor.txt"
cat "$b/native-object-donor.txt"
diff tests/framework-dr2/native-object.expected "$b/native-object-donor.txt"
cc $flags -c src/rhapsody/java/bridge/native-object.m -o "$b/native-object-adapter.o"
cc $flags tests/framework-dr2/native-object.m "$b/native-object-adapter.o" src/rhapsody/java/bridge/exceptions.m src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/object-map.m src/rhapsody/java/bridge/coding.m -Lout/dr2/lib -ljavai -framework Foundation -o "$b/native-object"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$b/native-object" "$b/native-object-classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/native-object.txt"
diff "$b/native-object-donor.txt" "$b/native-object.txt"
echo 'PASS native object wrapping and lifetime'
