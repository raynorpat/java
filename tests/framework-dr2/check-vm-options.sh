#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp -DDONOR -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/vm-options.m -framework Foundation -o "$build/vm-options-donor"
"$build/vm-options-donor" > "$build/vm-options-donor.txt"
diff tests/framework-dr2/vm-options.expected "$build/vm-options-donor.txt"
cc -traditional-cpp -Isrc/share/java/include -Isrc/unix/java/include tests/framework-dr2/vm-options.m src/rhapsody/java/bridge/vm.m -framework Foundation -o "$build/vm-options"
"$build/vm-options" > "$build/vm-options.txt"
diff "$build/vm-options-donor.txt" "$build/vm-options.txt"
echo 'PASS bridge VM options'
