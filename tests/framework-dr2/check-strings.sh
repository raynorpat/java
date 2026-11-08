#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp -DDONOR tests/framework-dr2/strings.m -framework Foundation -o "$build/strings-donor"
"$build/strings-donor" /System/Library/Frameworks/JavaVM.framework/Classes/classes.jar > "$build/strings-donor.txt"
cc -traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -c src/rhapsody/java/bridge/strings.m -o "$build/strings-adapter.o"
cc -traditional-cpp -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/strings.m src/rhapsody/java/bridge/vm.m "$build/strings-adapter.o" -Lout/dr2/lib -ljavai -framework Foundation -o "$build/strings"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/strings" vendor/jdk1.1.5/lib/classes.zip > "$build/strings.txt"
diff "$build/strings-donor.txt" "$build/strings.txt"
cat "$build/strings.txt"
echo 'PASS bridge strings'
