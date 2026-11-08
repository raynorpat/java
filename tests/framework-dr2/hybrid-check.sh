#!/bin/sh
set -eu
b=out/dr2-hybrid-tests
mkdir -p "$b/classes" "$b/retention"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/classes" tests/framework-dr2/HybridFixture.java tests/framework-dr2/HybridDescendant.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/retention" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -DDONOR tests/framework-dr2/hybrid.m -framework Foundation -o "$b/donor"
"$b/donor" "$b/retention:$b/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$b/donor.txt" 2> "$b/donor.err"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
objects=''
for source in src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c; do
 name=`basename "$source"`
 cc $flags -c "$source" -o "$b/$name.o"
 objects="$objects $b/$name.o"
done
cc $flags -c tests/framework-dr2/hybrid-adapter.m -o "$b/adapter.o"
cc -traditional-cpp tests/framework-dr2/hybrid.m "$b/adapter.o" $objects -Lout/dr2/lib -ljavai -framework Foundation -o "$b/reconstructed"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$b/reconstructed" "$b/retention:$b/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/reconstructed.txt" 2> "$b/reconstructed.err"
diff "$b/donor.txt" "$b/reconstructed.txt"
sed 's/^.*] //' "$b/donor.err" > "$b/donor.diagnostics"
sed 's/^.*] //' "$b/reconstructed.err" > "$b/reconstructed.diagnostics"
diff "$b/donor.diagnostics" "$b/reconstructed.diagnostics"
echo 'PASS hybrid generated class'
