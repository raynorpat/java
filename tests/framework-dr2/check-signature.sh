#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp -DDONOR tests/framework-dr2/signature.m -framework Foundation -o "$build/signature-donor"
"$build/signature-donor" > "$build/signature-donor.txt"
diff tests/framework-dr2/signature.expected "$build/signature-donor.txt"
cc -traditional-cpp -c src/rhapsody/java/bridge/signature.c -o "$build/signature-adapter.o"
cc -traditional-cpp tests/framework-dr2/signature.m "$build/signature-adapter.o" -framework Foundation -o "$build/signature"
"$build/signature" > "$build/signature.txt"
diff "$build/signature-donor.txt" "$build/signature.txt"
cc -traditional-cpp -Isrc/rhapsody/java/bridge tests/framework-dr2/signature-bounds.c "$build/signature-adapter.o" -o "$build/signature-bounds"
"$build/signature-bounds"
echo 'PASS bridge signatures'
