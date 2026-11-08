#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests" tests/awt-dr2/EventCover115.java
"$image/bin/java" -classpath "$image/tests:$image/lib/classes.zip" EventCover115 > "$image/tests/event-cover.actual"
echo 'DR2 event covers preserve Java identity across GC and clear released references' > "$image/tests/event-cover.expected"
diff "$image/tests/event-cover.expected" "$image/tests/event-cover.actual"
cat "$image/tests/event-cover.actual"
