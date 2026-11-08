#!/bin/sh
set -e
image=`cd "$1" && pwd`
"$image/tests/desktop-launch" "$2" "$image/tests/view" > "$image/tests/view.log" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/view.log"
        echo 'DR2 native view check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/view.log"
exit "$status"
