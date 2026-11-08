#!/bin/sh
set -eu
framework=`cd "$1" && pwd`
work=$2
desktop=$3
mkdir -p "$work"
work=`cd "$work" && pwd`
"$framework/Commands/javac" -classpath "$framework/Classes/rhapsody-peers.zip:$framework/Classes/classes.zip" -d "$work" tests/framework-dr2/FrameworkRuntime.java tests/vm/AudioLibrary115.java
cc -dynamiclib -I"$framework/Headers" -I"$framework/Headers/unix" tests/framework-dr2/framework-runtime.c -o "$work/libframeworkprobe.dylib"
cc tests/awt-dr2/desktop-launch.c -o "$work/desktop-launch"
"$work/desktop-launch" "$desktop" "$framework/Commands/java" -Dawt.toolkit=com.apple.rhapsody.awt.RToolkit -classpath "$work:$framework/Classes/rhapsody-peers.zip:$framework/Classes/classes.zip" FrameworkRuntime "$work/libframeworkprobe.dylib" "$framework" > "$work/runtime.txt" 2> "$work/runtime.err" &
pid=$!
count=0
while kill -0 $pid 2>/dev/null; do
    count=`expr $count + 1`
    if test "$count" -ge 60; then
        kill $pid
        wait $pid || :
        cat "$work/runtime.txt" "$work/runtime.err"
        echo 'FAIL framework runtime timeout' >&2
        exit 1
    fi
    sleep 1
done
status=0
wait $pid || status=$?
cat "$work/runtime.txt" "$work/runtime.err"
test "$status" -eq 0
grep '^PASS framework console JNI AWT audio and provenance$' "$work/runtime.txt"
