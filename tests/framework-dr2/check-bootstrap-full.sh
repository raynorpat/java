#!/bin/sh
set -eu
b=out/dr2-framework-tests/bootstrap-full
mkdir -p "$b/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/classes" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java tests/framework-dr2/ProxyFixture.java
flags='-traditional-cpp -fno-common -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags -dynamiclib src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c -Lout/dr2/lib -ljavai -framework Foundation -o "$b/libObjCJava.dylib"
cc -traditional-cpp tests/framework-dr2/bootstrap-full.m -L"$b" -lObjCJava -framework Foundation -o "$b/client"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib:`pwd`/$b" "$b/client" "$b/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$b/output.txt"
cat "$b/output.txt"
grep '^startup=12$' "$b/output.txt"
grep '^string=Hello private framework$' "$b/output.txt"
grep '^ownership-gc-thread=1$' "$b/output.txt"
echo 'PASS complete bridge bootstrap'
