#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/Menu115.java tests/awt-dr2/Popup115.java
for fixture in Menu115 Popup115; do
    actual="$image/tests/toolkit/$fixture.actual"
    "$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" "$fixture" track > "$actual" 2>&1 &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$actual"
            echo 'DR2 menu check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    cat "$actual"
    test "$status" -eq 0
    case "$fixture" in
        Menu115) echo 'DR2 menus preserve hierarchy, labels, state and attachment' ;;
        Popup115) echo 'DR2 popup menus deliver native actions and survive callback disposal' ;;
    esac > "$image/tests/toolkit/$fixture.expected"
    diff "$image/tests/toolkit/$fixture.expected" "$actual"
done
