#!/bin/sh
set -eu
framework=$1
work=$2
mkdir -p "$work"
framework=`cd "$framework" && pwd`
parent=`dirname "$framework"`
for name in JavaVM Headers/JavaVM.h Headers/NSJavaVirtualMachine.h Headers/NSJavaConfiguration.h Resources/Info-nextstep.plist Classes/classes.zip Classes/rhapsody-peers.zip Home/lib/classes.zip Libraries/libjavai.dylib Libraries/libObjCJava.dylib; do test -f "$framework/$name"; done
for name in java javac javah javap jar jre appletviewer native2ascii serialver javadoc javakey rmic rmiregistry jdb; do test -x "$framework/Commands/$name"; test -x "$framework/Home/libexec/$name"; done
test -L "$framework/Home/lib"
test -L "$framework/Libraries/libjava.A.dylib"
test -L "$framework/Libraries/libObjCJava.A.dylib"
cc -traditional-cpp -F"$parent" tests/framework-dr2/framework-layout-client.m -framework JavaVM -framework Foundation -o "$work/client"
DYLD_FRAMEWORK_PATH="$parent" "$work/client" "$framework/" > "$work/client.txt"
grep '^NSJavaVirtualMachine$' "$work/client.txt" >/dev/null
cat > "$work/FrameworkLayout.java" <<'EOF'
public class FrameworkLayout {
    public static void main(String[] args) {
        System.out.println("framework commands");
        System.out.println(System.getProperty("java.home"));
    }
}
EOF
"$framework/Commands/javac" -d "$work" "$work/FrameworkLayout.java"
"$framework/Commands/java" -classpath "$work:$framework/Classes/classes.zip" FrameworkLayout > "$work/java.txt"
grep '^framework commands$' "$work/java.txt" >/dev/null
grep "$framework/Home" "$work/java.txt" >/dev/null
"$framework/Commands/jar" tf "$framework/Classes/classes.zip" > "$work/classes.txt" 2>&1
grep '^com/apple/yellow/foundation/NSRuntime.class$' "$work/classes.txt" >/dev/null
cc -traditional-cpp -F"$parent" tests/framework-dr2/framework-layout-bridge-client.m -framework JavaVM -framework Foundation -L"$framework/Libraries" -lObjCJava -ljavai -o "$work/bridge-client"
DYLD_FRAMEWORK_PATH="$parent" DYLD_LIBRARY_PATH="$framework/Libraries" "$work/bridge-client" > "$work/bridge-client.txt"
grep '^NSJavaObject$' "$work/bridge-client.txt" >/dev/null
cc -traditional-cpp tests/framework-dr2/framework-layout-paths.m -framework Foundation -o "$work/paths"
"$work/paths" "$framework" > "$work/paths.txt"
cc -traditional-cpp -u _objc_msgSend tests/framework-dr2/framework-layout-imports.m -framework Foundation -o "$work/imports"
"$work/imports" "$framework" > "$work/imports.txt"
cc -traditional-cpp -u _objc_msgSend tests/framework-dr2/core-exports-check.m -L"$framework/Libraries" -lObjCJava -ljavai -framework Foundation -o "$work/exports"
DYLD_LIBRARY_PATH="$framework/Libraries" "$work/exports" "$framework/Libraries/libObjCJava.dylib" tests/framework-dr2/core-exports.tsv "$framework/Libraries/libjavai.dylib" > "$work/exports.txt"
echo 'PASS framework layout, link client and commands'
