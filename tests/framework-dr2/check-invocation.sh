#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/InvocationFixture.java
cc -traditional-cpp -DDONOR tests/framework-dr2/invocation.m -framework Foundation -o "$build/invocation-donor"
"$build/invocation-donor" "$build/classes:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/invocation-donor.txt"
diff tests/framework-dr2/invocation.expected "$build/invocation-donor.txt"
cc -traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -c src/rhapsody/java/bridge/invocation.c -o "$build/invocation-adapter.o"
cc -traditional-cpp -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/invocation.m src/rhapsody/java/bridge/vm.m "$build/invocation-adapter.o" -Lout/dr2/lib -ljavai -framework Foundation -o "$build/invocation"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/invocation" "$build/classes:vendor/jdk1.1.5/lib/classes.zip" > "$build/invocation.txt"
diff "$build/invocation-donor.txt" "$build/invocation.txt"
echo 'PASS bridge invocation'
