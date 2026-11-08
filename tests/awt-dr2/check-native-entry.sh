#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests" tests/awt-dr2/NativeEntry115.java
echo 'DR2 JNI native entry survives nested callbacks and GC' > "$image/tests/nativeentry.expected"
"$image/tests/desktop-launch" "$2" "$image/bin/java" -classpath "$image/tests:$image/lib/classes.zip" NativeEntry115 > "$image/tests/nativeentry.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/nativeentry.actual"
        echo 'DR2 JNI native entry timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/nativeentry.actual"
test "$status" -eq 0
diff "$image/tests/nativeentry.expected" "$image/tests/nativeentry.actual"
