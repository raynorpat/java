#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/SyntheticValues115.java
"$image/tests/desktop-launch" "$2" "$image/bin/java" -classpath "$image/tests/toolkit:$image/lib/classes.zip" SyntheticValues115 > "$image/tests/toolkit/synthetic-values.actual" 2>&1
cat "$image/tests/toolkit/synthetic-values.actual"
echo 'DR2 synthetic factories match all 21 event cases' > "$image/tests/toolkit/synthetic-values.expected"
diff "$image/tests/toolkit/synthetic-values.expected" "$image/tests/toolkit/synthetic-values.actual"
