#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/ConstructorFixture.java
cc -traditional-cpp -DDONOR -Isrc/rhapsody/java/bridge -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/constructors.m -framework Foundation -o "$build/constructors-donor"
"$build/constructors-donor" "$build/classes:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/constructors-donor.txt"
diff tests/framework-dr2/constructors.expected "$build/constructors-donor.txt"
cc -traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -c src/rhapsody/java/bridge/constructor.c -o "$build/constructor-adapter.o"
cc -traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -c src/rhapsody/java/bridge/invocation.c -o "$build/invocation-adapter.o"
cc -traditional-cpp -Isrc/rhapsody/java/bridge -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/constructors.m src/rhapsody/java/bridge/vm.m "$build/constructor-adapter.o" "$build/invocation-adapter.o" -Lout/dr2/lib -ljavai -framework Foundation -o "$build/constructors"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/constructors" "$build/classes:vendor/jdk1.1.5/lib/classes.zip" > "$build/constructors.txt"
diff "$build/constructors-donor.txt" "$build/constructors.txt"
echo 'PASS bridge constructors'
