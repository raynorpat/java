#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/MouseValues115.java
"$image/tests/desktop-launch" "$2" "$image/bin/java" -classpath "$image/tests/toolkit:$image/lib/classes.zip" MouseValues115 > "$image/tests/toolkit/mouse-values.actual" 2>&1
cat "$image/tests/toolkit/mouse-values.actual"
echo 'DR2 mouse conversion matches all 144 type/modifier/hit cases' > "$image/tests/toolkit/mouse-values.expected"
diff "$image/tests/toolkit/mouse-values.expected" "$image/tests/toolkit/mouse-values.actual"
