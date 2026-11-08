#!/bin/sh
set -e
image=`cd "$1" && pwd`
peers="$2"
desktop="$3"
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -classpath "$peers:$image/lib/classes.zip" -d "$image/tests/toolkit" tests/awt-dr2/ToolkitStartup115.java tests/awt-dr2/ToolkitValues115.java
for name in ToolkitStartup115 com.apple.rhapsody.awt.ToolkitValues115; do
    log="$image/tests/toolkit/$name.log"
    "$image/tests/desktop-launch" "$desktop" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$peers:$image/lib/classes.zip" "$name" > "$log" 2>&1 &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$log"
            echo 'DR2 Toolkit check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    cat "$log"
    test "$status" -eq 0
done
