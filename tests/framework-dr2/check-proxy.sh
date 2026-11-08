#!/bin/sh
set -eu
build=out/dr2-framework-tests
mkdir -p "$build/classes" "$build/retention-classes"
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/classes" tests/framework-dr2/ProxyFixture.java
out/dr2/bin/javac -classpath vendor/jdk1.1.5/lib/classes.zip -d "$build/retention-classes" tests/framework-dr2/retention/com/apple/yellow/foundation/NSRuntime.java
cc -traditional-cpp -DDONOR tests/framework-dr2/proxy.m -framework Foundation -framework JavaVM -o "$build/proxy-donor"
"$build/proxy-donor" "$build/classes:/System/Library/Java:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar" > "$build/proxy-donor.txt"
cat "$build/proxy-donor.txt"
flags='-traditional-cpp -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags tests/framework-dr2/proxy.m src/rhapsody/java/bridge/NSJavaObject.m src/rhapsody/java/bridge/forwarding.m src/rhapsody/java/bridge/objects.m src/rhapsody/java/bridge/dispatch.m src/rhapsody/java/bridge/hybrid-dispatch.m src/rhapsody/java/bridge/classgen.m src/rhapsody/java/bridge/class-map.m src/rhapsody/java/bridge/selectors.m src/rhapsody/java/bridge/signature.c src/rhapsody/java/bridge/vm.m src/rhapsody/java/bridge/exceptions.m src/rhapsody/java/bridge/invocation.c src/rhapsody/java/bridge/constructor.c src/rhapsody/java/bridge/retention.m src/rhapsody/java/bridge/strings.m src/rhapsody/java/bridge/diagnostics.m src/rhapsody/java/bridge/object-map.m src/rhapsody/java/bridge/key-value.m src/rhapsody/java/bridge/coding.m src/rhapsody/java/bridge/native-object.m src/rhapsody/java/bridge/value-conversions.m src/rhapsody/java/bridge/thread-support.m -Lout/dr2/lib -ljavai -framework Foundation -o "$build/proxy"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib" "$build/proxy" "$build/retention-classes:$build/classes:/System/Library/Java:vendor/jdk1.1.5/lib/classes.zip" > "$build/proxy.txt"
diff "$build/proxy-donor.txt" "$build/proxy.txt"
echo 'PASS Java proxy integration'
