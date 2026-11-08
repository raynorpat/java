#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/retention-classes" "$build/classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/retention-classes" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
out/dr2/bin/javac -classpath /System/Library/Java:vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/ExceptionFixture.java
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include'
cc $flags -c tests/framework-dr2/exception-state.c -o "$build/exception-state.o"
cc -traditional-cpp -DDONOR tests/framework-dr2/exceptions.m "$build/exception-state.o" -framework Foundation -o "$build/exceptions-donor"
"$build/exceptions-donor" "$build/retention-classes:$build/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/exceptions-donor.txt" 2> "$build/exceptions-donor.err"
diff tests/framework-dr2/exceptions.expected "$build/exceptions-donor.txt"
cc $flags -c src/rhapsody/java/bridge/exceptions.m -o "$build/exceptions-adapter.o"
cc $flags tests/framework-dr2/exceptions.m "$build/exception-state.o" "$build/exceptions-adapter.o" src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/object-map.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/exceptions"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/exceptions" "$build/retention-classes:$build/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$build/exceptions.txt" 2> "$build/exceptions.err"
diff "$build/exceptions-donor.txt" "$build/exceptions.txt"
sed 's/^.*] //' "$build/exceptions-donor.err" > "$build/exceptions-donor.diagnostics"
sed 's/^.*] //' "$build/exceptions.err" > "$build/exceptions.diagnostics"
diff "$build/exceptions-donor.diagnostics" "$build/exceptions.diagnostics"
echo 'PASS bridge exception conversion'
