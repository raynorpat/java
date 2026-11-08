#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp -DDONOR tests/framework-dr2/object-map.m -framework Foundation -framework JavaVM -o "$build/object-map-donor"
"$build/object-map-donor" '/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar' > "$build/object-map-donor.txt"
diff tests/framework-dr2/object-map.expected "$build/object-map-donor.txt"
cc -traditional-cpp -c src/rhapsody/java/bridge/object-map.m -o "$build/object-map-adapter.o"
cc -traditional-cpp -Isrc/rhapsody/java/bridge tests/framework-dr2/object-map.m "$build/object-map-adapter.o" -framework Foundation -o "$build/object-map"
"$build/object-map" unused > "$build/object-map.txt"
diff "$build/object-map-donor.txt" "$build/object-map.txt"
echo 'PASS bridge object map'
