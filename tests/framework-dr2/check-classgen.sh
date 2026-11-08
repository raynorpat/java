#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/ClassGenFixture.java
cc -traditional-cpp -DDONOR tests/framework-dr2/classgen.m -framework Foundation -framework JavaVM -o "$build/classgen-donor"
"$build/classgen-donor" "$build/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/classgen-donor.txt"
diff tests/framework-dr2/classgen.expected "$build/classgen-donor.txt"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c src/rhapsody/java/bridge/classgen.m -o "$build/classgen-adapter.o"
cc $flags -Isrc/rhapsody/java/bridge tests/framework-dr2/classgen.m "$build/classgen-adapter.o" src/rhapsody/java/bridge/class-map.m src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/selectors.m src/rhapsody/java/bridge/signature.c src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/invocation.c -Lout/dr2/lib -ljavai -framework Foundation -o "$build/classgen"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/classgen" "$build/classes:vendor/jdk1.1.5/lib/classes.zip" > "$build/classgen.txt"
diff "$build/classgen-donor.txt" "$build/classgen.txt"
echo 'PASS generated bridge class'
