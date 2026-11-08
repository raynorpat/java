#!/bin/sh
set -eu
b=out/dr2-wrapper-dependencies
mkdir -p "$b/classes" "$b/runtime"
out/dr2/bin/javac -classpath /System/Library/Java:vendor/jdk1.1.5/lib/classes.zip -d "$b/classes" tests/framework-dr2/WrapperDependencyFixture.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$b/runtime" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -DDONOR tests/framework-dr2/wrapper-dependencies.m -framework Foundation -o "$b/donor"
flags='-traditional-cpp -fno-common -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags -dynamiclib src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c -Lout/dr2/lib -ljavai -framework Foundation -o "$b/libObjCJava.dylib"
cc -traditional-cpp tests/framework-dr2/wrapper-dependencies.m -L"$b" -lObjCJava -framework Foundation -o "$b/client"
for which in 0 1; do
 "$b/donor" "$b/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" "$which" > "$b/donor-$which.txt" 2> "$b/donor-$which.err"
 DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib:`pwd`/$b" "$b/client" "$b/runtime:$b/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" "$which" > "$b/client-$which.txt" 2> "$b/client-$which.err"
 grep '^result=0$' "$b/donor-$which.txt"
 grep '^result=java.lang.UnsatisfiedLinkError:FoundationJava.B$' "$b/client-$which.txt"
done
echo 'PASS separate FoundationJava dependency boundary'
