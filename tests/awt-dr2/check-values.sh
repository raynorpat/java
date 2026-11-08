#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests" tests/awt-dr2/Values115.java
"$image/bin/java" -classpath "$image/tests:$image/lib/classes.zip" Values115 > "$image/tests/values.actual"
echo 'DR2 Java strings and colors preserve native values' > "$image/tests/values.expected"
diff "$image/tests/values.expected" "$image/tests/values.actual"
cat "$image/tests/values.actual"
