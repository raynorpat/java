#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/CanvasDraw115.java tests/awt-dr2/ContainerDraw115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" ContainerDraw115 > "$image/tests/toolkit/container-draw.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/container-draw.actual"
        echo 'DR2 Container draw check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/container-draw.actual"
test "$status" -eq 0
echo 'DR2 containers clip child rectangles and preserve tracking and hit testing' > "$image/tests/toolkit/container-draw.expected"
diff "$image/tests/toolkit/container-draw.expected" "$image/tests/toolkit/container-draw.actual"
