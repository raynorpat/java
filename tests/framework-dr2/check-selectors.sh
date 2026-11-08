#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
cc -traditional-cpp -DDONOR tests/framework-dr2/selectors.m -framework Foundation -framework JavaVM -o "$build/selectors-donor"
"$build/selectors-donor" > "$build/selectors-donor.txt" 2> "$build/selectors-donor.err"
diff tests/framework-dr2/selectors.expected "$build/selectors-donor.txt"
cc -traditional-cpp -c src/rhapsody/java/bridge/selectors.m -o "$build/selectors-adapter.o"
cc -traditional-cpp -Isrc/rhapsody/java/bridge tests/framework-dr2/selectors.m "$build/selectors-adapter.o" src/rhapsody/java/bridge/diagnostics.m -framework Foundation -o "$build/selectors"
"$build/selectors" > "$build/selectors.txt" 2> "$build/selectors.err"
diff "$build/selectors-donor.txt" "$build/selectors.txt"
sed 's/^.*] //' "$build/selectors-donor.err" | egrep '^(ObjCJava:|error:|warning:|       this|fixture)' > "$build/selectors-donor.diagnostics"
sed 's/^.*] //' "$build/selectors.err" | egrep '^(ObjCJava:|error:|warning:|       this|fixture)' > "$build/selectors.diagnostics" || :
diff "$build/selectors-donor.diagnostics" "$build/selectors.diagnostics"
echo 'PASS bridge selectors'
