#!/bin/sh
set -eu
b=out/dr2-hybrid-tests
mkdir -p "$b/classes" "$b/retention"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/classes" tests/framework-dr2/AccessFixture.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/retention" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -Isrc/rhapsody/java/bridge -Isrc/share/java/include -Isrc/unix/java/include -DDONOR tests/framework-dr2/hybrid-access.m -framework Foundation -o "$b/access-donor"
"$b/access-donor" "$b/retention:$b/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$b/access-donor.txt" 2> "$b/access-donor.err"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
objects=''
for source in src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c; do
 name=`basename "$source"`
 cc $flags -c "$source" -o "$b/$name.o"
 objects="$objects $b/$name.o"
done
cc -traditional-cpp -Isrc/rhapsody/java/bridge -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/hybrid-access.m $objects -Lout/dr2/lib -ljavai -framework Foundation -o "$b/access"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$b/access" "$b/retention:$b/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/access.txt" 2> "$b/access.err"
diff "$b/access-donor.txt" "$b/access.txt"
sed 's/^.*] //' "$b/access-donor.err" > "$b/access-donor.diagnostics"
sed 's/^.*] //' "$b/access.err" > "$b/access.diagnostics"
diff "$b/access-donor.diagnostics" "$b/access.diagnostics"
echo 'PASS native caller constructor access'
