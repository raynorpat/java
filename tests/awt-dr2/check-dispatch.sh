#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests:$image/lib"
export DYLD_LIBRARY_PATH
mkdir -p "$image/tests/dispatch"
"$image/bin/javac" -d "$image/tests/dispatch" tests/awt-dr2/dispatch/RToolkit.java tests/awt-dr2/dispatch/RMenuItemPeer.java tests/awt-dr2/dispatch/Dispatcher115.java
"$image/tests/desktop-launch" "$2" "$image/bin/java" -classpath "$image/tests/dispatch:$image/lib/classes.zip" com.apple.rhapsody.awt.Dispatcher115 > "$image/tests/dispatch.log" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/dispatch.log"
        echo 'DR2 Java dispatch timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/dispatch.log"
test "$status" -eq 0
echo 'DR2 Java dispatch re-enters AppKit during callbacks and GC' > "$image/tests/dispatch.expected"
diff "$image/tests/dispatch.expected" "$image/tests/dispatch.log"
