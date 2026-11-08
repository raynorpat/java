#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -classpath "$2:$image/lib/classes.zip" -d "$image/tests/toolkit" tests/awt-dr2/Print115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" com.apple.rhapsody.awt.Print115 > "$image/tests/toolkit/print.actual" 2> "$image/tests/toolkit/print.errors" &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/print.actual"
        echo 'DR2 print check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/print.actual"
test "$status" -eq 0
echo 'DR2 print constants, job properties, cancellation and surfaces passed' > "$image/tests/toolkit/print.expected"
diff "$image/tests/toolkit/print.expected" "$image/tests/toolkit/print.actual"
# The direct getGraphics negative test exercises the donor's unsupported surface.
grep 'AwtPrintJob awtColor.*selector not recognized' "$image/tests/toolkit/print.errors" >/dev/null
test `wc -l < "$image/tests/toolkit/print.errors"` -eq 1
