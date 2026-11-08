#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build"
export FRAMEWORK_TEST_ROOT=/fixture
unset FRAMEWORK_UNDEFINED || :
cc -traditional-cpp tests/framework-dr2/configuration.m -framework Foundation -framework JavaVM -o "$build/configuration-donor"
"$build/configuration-donor" > "$build/configuration-donor.txt"
diff tests/framework-dr2/configuration.expected "$build/configuration-donor.txt"
if test "${1-}" = donor; then
    echo 'PASS donor configuration'
    exit 0
fi
cc -traditional-cpp -DRECONSTRUCTED -Isrc/rhapsody/java/framework tests/framework-dr2/configuration.m src/rhapsody/java/framework/NSJavaConfiguration.m -framework Foundation -o "$build/configuration"
"$build/configuration" > "$build/configuration.txt"
diff "$build/configuration-donor.txt" "$build/configuration.txt"
echo 'PASS framework configuration'
