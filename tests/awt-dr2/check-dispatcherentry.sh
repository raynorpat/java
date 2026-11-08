#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/DispatcherEntry115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" DispatcherEntry115 > "$image/tests/toolkit/dispatcherentry.actual" 2> "$image/tests/toolkit/dispatcherentry.errors" &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/dispatcherentry.actual"
        echo 'DR2 dispatcherentry check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/dispatcherentry.actual"
test "$status" -eq 0
echo 'DR2 obsolete dispatcher preserves donor exception and recovery' > "$image/tests/toolkit/dispatcherentry.expected"
diff "$image/tests/toolkit/dispatcherentry.expected" "$image/tests/toolkit/dispatcherentry.actual"
grep 'AwtDispatcher runWithEnv:.*selector not recognized' "$image/tests/toolkit/dispatcherentry.errors" >/dev/null
test `wc -l < "$image/tests/toolkit/dispatcherentry.errors"` -eq 1
