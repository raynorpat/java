#!/bin/sh
set -eu
b=out/dr2-framework-tests/vm-helpers
mkdir -p "$b/runtime"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/runtime" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -Isrc/rhapsody/java/bridge -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/vm-helpers.m -framework Foundation -o "$b/client"
"$b/client" /usr/lib/java/libObjCJava.A.dylib /System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar > "$b/donor.txt" 2> "$b/donor.err"
flags='-traditional-cpp -fno-common -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags -dynamiclib src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c -Lout/dr2/lib -ljavai -framework Foundation -o "$b/libObjCJava.dylib"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$b/client" "`pwd`/$b/libObjCJava.dylib" "$b/runtime:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/reconstructed.txt" 2> "$b/reconstructed.err"
diff "$b/donor.txt" "$b/reconstructed.txt"
sed -e '/distributed notification server/d' -e 's/^.*] //' "$b/donor.err" > "$b/donor.diagnostics"
sed -e '/distributed notification server/d' -e 's/^.*] //' "$b/reconstructed.err" > "$b/reconstructed.diagnostics"
diff "$b/donor.diagnostics" "$b/reconstructed.diagnostics"
echo 'PASS exported VM, number and library helpers'
