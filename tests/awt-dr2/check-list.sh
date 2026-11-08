#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/List115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" List115 track > "$image/tests/toolkit/list.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/list.actual"
        echo 'DR2 list check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/list.actual"
test "$status" -eq 0
cat > "$image/tests/toolkit/list.expected" <<'EOF'
0:128x34:128x94
1:128x34:128x94
DR2 lists preserve items, selection and row sizing
EOF
diff "$image/tests/toolkit/list.expected" "$image/tests/toolkit/list.actual"
