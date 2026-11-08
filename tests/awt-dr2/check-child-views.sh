#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/ChildViews115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" ChildViews115 track > "$image/tests/toolkit/child-views.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/child-views.actual"
        echo 'DR2 child-views check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/child-views.actual"
test "$status" -eq 0
echo 'DR2 child views preserve attachment, bounds, enabled state and painting' > "$image/tests/toolkit/child-views.expected"
diff "$image/tests/toolkit/child-views.expected" "$image/tests/toolkit/child-views.actual"
