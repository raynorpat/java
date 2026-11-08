#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/ScrollPane115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" ScrollPane115 track > "$image/tests/toolkit/scrollpane.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/scrollpane.actual"
        echo 'DR2 scrollpane check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/scrollpane.actual"
test "$status" -eq 0
cat > "$image/tests/toolkit/scrollpane.expected" <<'EOF'
0:large:2,2,17,17:201x161
0:bars:15,15
0:small:2,2,17,17:201x161
1:large:2,2,17,17:201x161
1:bars:15,15
1:small:2,2,17,17:201x161
2:large:2,2,2,2:216x176
2:bars:0,0
2:small:2,2,2,2:216x176
DR2 scroll panes preserve policies, viewport sizing and adjustable positions
EOF
diff "$image/tests/toolkit/scrollpane.expected" "$image/tests/toolkit/scrollpane.actual"
