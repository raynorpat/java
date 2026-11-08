#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/Button115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" Button115 track > "$image/tests/toolkit/button.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/button.actual"
        echo 'DR2 button check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/button.actual"
test "$status" -eq 0
echo 'DR2 buttons preserve labels, sizing, enabled state and action callbacks' > "$image/tests/toolkit/button.expected"
diff "$image/tests/toolkit/button.expected" "$image/tests/toolkit/button.actual"
