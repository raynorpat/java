#!/bin/sh
set -eu
b=out/dr2-export-coverage
mkdir -p "$b"
flags='-traditional-cpp -fno-common -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/rhapsody/java/bridge'
cc $flags -dynamiclib src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c -Lout/dr2/lib -ljavai -framework Foundation -o "$b/libObjCJava.dylib"
cc -traditional-cpp tests/framework-dr2/core-exports-check.m -L"$b" -lObjCJava -framework Foundation -o "$b/check"
DYLD_LIBRARY_PATH="`pwd`/out/dr2/lib:`pwd`/$b" "$b/check" "$b/libObjCJava.dylib" tests/framework-dr2/core-exports.tsv out/dr2/lib/libjavai.dylib
