#!/bin/sh
set -eu
build=out/dr2-framework-tests/bootstrap
mkdir -p "$build"
cc -traditional-cpp -DDONOR tests/framework-dr2/bootstrap-threads.m -framework Foundation -o "$build/threads-donor"
"$build/threads-donor" > "$build/threads-donor.txt"
cc -traditional-cpp tests/framework-dr2/bootstrap-threads.m src/rhapsody/java/bridge/thread-support.m -framework Foundation -o "$build/threads"
"$build/threads" > "$build/threads.txt"
diff "$build/threads-donor.txt" "$build/threads.txt"
echo 'PASS deferred release native differential'
