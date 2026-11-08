#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/Window115.java tests/awt-dr2/WindowLifetime115.java tests/awt-dr2/SmallFrame115.java tests/awt-dr2/Raster115.java
for name in Window115 WindowLifetime115 SmallFrame115 Raster115; do
    log="$image/tests/toolkit/$name.log"
    "$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" "$name" > "$log" 2>&1 &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$log"
            echo 'DR2 Frame check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    cat "$log"
    test "$status" -eq 0
    if grep 'Exception occurred during event dispatching:' "$log" >/dev/null; then
        echo 'DR2 Frame event dispatch failed' >&2
        exit 1
    fi
    if test "$name" = SmallFrame115; then
        tr -d '\015' < tests/awt-dr2/SmallFrame115.expected > "$image/tests/toolkit/SmallFrame115.expected"
        diff "$image/tests/toolkit/SmallFrame115.expected" "$log"
    fi
done
