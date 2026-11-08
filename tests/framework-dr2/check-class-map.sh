#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/ClassMapFixture.java
cc -traditional-cpp -DDONOR tests/framework-dr2/class-map.m -framework Foundation -framework JavaVM -o "$build/class-map-donor"
"$build/class-map-donor" "$build/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/class-map-donor.txt" 2> "$build/class-map-donor.err"
diff tests/framework-dr2/class-map.expected "$build/class-map-donor.txt"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c src/rhapsody/java/bridge/class-map.m -o "$build/class-map-adapter.o"
cc $flags -Isrc/rhapsody/java/bridge tests/framework-dr2/class-map.m src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/diagnostics.m "$build/class-map-adapter.o" -Lout/dr2/lib -ljavai -framework Foundation -o "$build/class-map"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/class-map" "$build/classes:vendor/jdk1.1.5/lib/classes.zip" > "$build/class-map.txt" 2> "$build/class-map.err"
diff "$build/class-map-donor.txt" "$build/class-map.txt"
sed 's/^.*] //' "$build/class-map-donor.err" | grep '^ObjCJava:' > "$build/class-map-donor.diagnostics"
sed 's/^.*] //' "$build/class-map.err" | grep '^ObjCJava:' > "$build/class-map.diagnostics"
diff "$build/class-map-donor.diagnostics" "$build/class-map.diagnostics"
echo 'PASS bridge class map'
