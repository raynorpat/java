#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/AdapterFixture.java
cc -traditional-cpp -DDONOR -Isrc/share/java/include -Isrc/unix/java/include -Isrc/rhapsody/java/bridge tests/framework-dr2/vm-live.m -framework Foundation -o "$build/vm-live-donor"
"$build/vm-live-donor" /System/Library/Frameworks/JavaVM.framework/Classes/classes.jar "$build/classes/AdapterFixture.class" > "$build/vm-live-donor.txt"
cc -traditional-cpp -Isrc/share/java/include -Isrc/unix/java/include -Isrc/rhapsody/java/bridge tests/framework-dr2/vm-live.m src/rhapsody/java/bridge/vm.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/vm-live"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/vm-live" vendor/jdk1.1.5/lib/classes.zip "$build/classes/AdapterFixture.class" > "$build/vm-live.txt"
diff "$build/vm-live-donor.txt" "$build/vm-live.txt"
cat "$build/vm-live.txt"
echo 'PASS live bridge VM adapter'
