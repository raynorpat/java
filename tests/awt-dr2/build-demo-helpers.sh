#!/bin/sh
# Run from the source root; these helpers never enter the installed image.
set -e
image=`cd "$1" && pwd`
mkdir -p "$2"
helpers=`cd "$2" && pwd`
DYLD_LIBRARY_PATH="$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$helpers" tests/awt-dr2/DR2DemoObserver115.java tests/awt118/DemoObserver115.java tests/tools/WinAwtDemoApplet115.java
cc -dynamiclib -Isrc/share/java/include -Isrc/unix/java/include tests/awt-dr2/demoprobe115.m \
    -framework AppKit -framework Foundation -L"$image/lib" -lawt -o "$helpers/libdemoprobe115.dylib"
