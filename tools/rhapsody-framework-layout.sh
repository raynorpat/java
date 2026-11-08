#!/bin/sh
set -eu
framework=$1
runtime=$2
jdk=$3
shift 3
link()
{
    if test -L "$2"; then rm -f "$2"; fi
    ln -s "$1" "$2"
}
mkdir -p "$framework/Versions/A/Headers/unix" "$framework/Versions/A/Resources" "$framework/Commands" "$framework/Home" "$framework/Libraries" "$framework/Classes"
link A "$framework/Versions/Current"
for name in Headers Resources JavaVM; do link "Versions/Current/$name" "$framework/$name"; done
link ../Libraries "$framework/Home/lib"
link ../Commands "$framework/Home/bin"
link ../Headers "$framework/Home/include"
for name in "$@"; do cp tools/rhapsody-framework-launcher.sh "$framework/Commands/$name"; chmod +x "$framework/Commands/$name"; done
cp src/rhapsody/java/framework/*.h "$framework/Headers/"
cp src/share/java/include/jni.h "$framework/Headers/"
cp src/unix/java/include/jni_md.h src/unix/java/include/inttypes_md.h "$framework/Headers/unix/"
cp src/rhapsody/java/framework/Info-nextstep.plist "$framework/Resources/"
cp "$runtime/lib/rhapsody-peers.zip" "$framework/Classes/"
link classes.zip "$framework/Classes/classes.jar"
link ../Classes/classes.zip "$framework/Libraries/classes.zip"
link ../Classes/rhapsody-peers.zip "$framework/Libraries/rhapsody-peers.zip"
for name in javai net zip math sysresource jpeg agent awt mmedia ObjCJava; do link "lib$name.dylib" "$framework/Libraries/lib$name.A.dylib"; done
link libjavai.dylib "$framework/Libraries/libjava.dylib"
link libjavai.dylib "$framework/Libraries/libjava.A.dylib"
mkdir -p "$framework/Libraries/security" "$framework/Resources/legal"
cp "$jdk/lib/security/java.security" "$framework/Libraries/security/"
cp "$jdk"/lib/*.properties "$framework/Libraries/"
cp -R "$runtime/legal/." "$framework/Resources/legal/"
