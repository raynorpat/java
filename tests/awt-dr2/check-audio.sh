#!/bin/sh
set -e
image=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$image/tests/toolkit:$image/lib"
export DYLD_LIBRARY_PATH
"$image/bin/javac" -d "$image/tests/toolkit" tests/awt-dr2/Audio115.java tests/awt-dr2/AudioBackend115.java tests/vm/AudioLibrary115.java
for test in Audio115 AudioBackend115 AudioLibrary115; do
    "$image/tests/desktop-launch" "$3" "$image/bin/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$image/tests/toolkit:$2:$image/lib/classes.zip" "$test" > "$image/tests/toolkit/$test.actual" 2> "$image/tests/toolkit/$test.errors" &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$image/tests/toolkit/$test.actual" "$image/tests/toolkit/$test.errors"
            echo 'DR2 audio check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    cat "$image/tests/toolkit/$test.actual" "$image/tests/toolkit/$test.errors"
    test "$status" -eq 0
done
diff tests/awt-dr2/audio-backend.expected "$image/tests/toolkit/AudioBackend115.actual"
grep 'DR2 audio open, silence writes and close passed' "$image/tests/toolkit/Audio115.actual" >/dev/null
grep '^audio library passed$' "$image/tests/toolkit/AudioLibrary115.actual" >/dev/null
# The probe deliberately exercises both activation failures. SoundKit also logs
# its two legacy HashTable cleanup warnings for the posed device/stream classes.
grep 'stream activate transient failure (301)' "$image/tests/toolkit/AudioBackend115.errors" >/dev/null
grep 'stream activate definitive failure (300)' "$image/tests/toolkit/AudioBackend115.errors" >/dev/null
test `wc -l < "$image/tests/toolkit/AudioBackend115.errors"` -eq 4
test ! -s "$image/tests/toolkit/AudioLibrary115.errors"
