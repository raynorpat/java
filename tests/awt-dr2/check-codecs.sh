#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt/GifRaster115.java tests/awt/DecoderImages115.java
: > "$image/tests/toolkit/codecs.actual"
for test in GifRaster115 DecoderImages115; do
    "$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" "$test" >> "$image/tests/toolkit/codecs.actual" 2>&1 &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$image/tests/toolkit/codecs.actual"
            echo 'DR2 codec check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    if test "$status" -ne 0; then
        cat "$image/tests/toolkit/codecs.actual"
        exit "$status"
    fi
done
cat "$image/tests/toolkit/codecs.actual"
diff tests/awt-dr2/codecs.expected "$image/tests/toolkit/codecs.actual"
