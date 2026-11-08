#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/TextArea115.java
"$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" TextArea115 track > "$image/tests/toolkit/textarea.actual" 2>&1 &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 30; then
        kill $pid
        wait $pid || :
        cat "$image/tests/toolkit/textarea.actual"
        echo 'DR2 textarea check timed out' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$image/tests/toolkit/textarea.actual"
test "$status" -eq 0
cat > "$image/tests/toolkit/textarea.expected" <<'EOF'
0:57x49:127x94
1:57x34:127x79
2:42x49:112x94
3:42x34:112x79
DR2 text areas preserve multiline editing, selection and scrollbar sizing
EOF
diff "$image/tests/toolkit/textarea.expected" "$image/tests/toolkit/textarea.actual"
