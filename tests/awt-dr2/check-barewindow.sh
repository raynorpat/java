#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/BareWindow115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" BareWindow115 track > "$image/tests/toolkit/barewindow.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/barewindow.actual"
        echo 'DR2 borderless window check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/barewindow.actual"
test "$status" -eq 0
echo 'DR2 borderless windows preserve geometry, sizing and focus contracts' > "$image/tests/toolkit/barewindow.expected"
diff "$image/tests/toolkit/barewindow.expected" "$image/tests/toolkit/barewindow.actual"
