#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/lib"
export DYLD_LIBRARY_PATH
"$image/tests/desktop-launch" "$2" "$image/tests/context" > "$image/tests/context.log" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/context.log"
        echo 'DR2 context check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/context.log"
exit $status
