#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/KeyValues115.java
"$image/tests/desktop-launch" "$2" "$image/bin/java" -classpath "$image/tests/toolkit:$image/lib/classes.zip" KeyValues115 > "$image/tests/toolkit/key-values.actual" 2>&1
cat "$image/tests/toolkit/key-values.actual"
echo 'DR2 key conversion matches all 61 character/modifier cases' > "$image/tests/toolkit/key-values.expected"
diff "$image/tests/toolkit/key-values.expected" "$image/tests/toolkit/key-values.actual"
