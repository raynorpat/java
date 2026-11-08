#!/bin/sh
set -eu
b=out/dr2-values-abi
mkdir -p "$b/runtime" "$b/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/runtime" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/classes" tests/framework-dr2/ClassAbiFixture.java
cc -traditional-cpp -DDONOR tests/framework-dr2/values-abi.m -framework Foundation -o "$b/donor"
NEXT_ROOT=/private/bridge-test-root "$b/donor" "$b/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$b/donor.txt" 2> "$b/donor.err"
flags='-traditional-cpp -fno-common -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags -dynamiclib src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c -Lout/dr2/lib -ljavai -framework Foundation -o "$b/libObjCJava.dylib"
cc -traditional-cpp tests/framework-dr2/values-abi.m -L"$b" -lObjCJava -framework Foundation -o "$b/client"
NEXT_ROOT=/private/bridge-test-root DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib:`pwd`/$b" "$b/client" "$b/runtime:$b/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/client.txt" 2> "$b/client.err"
diff "$b/donor.txt" "$b/client.txt"
sed 's/^.*] //' "$b/donor.err" > "$b/donor.diagnostics"
sed 's/^.*] //' "$b/client.err" > "$b/client.diagnostics"
diff "$b/donor.diagnostics" "$b/client.diagnostics"
echo 'PASS class and legacy type mapping ABI'
