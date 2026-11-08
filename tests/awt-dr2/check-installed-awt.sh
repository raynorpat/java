#!/bin/sh
set -e
source=`pwd`
installed=`cd "$1" && pwd`
build=`cd "$2" && pwd`
launcher="$build/tests/desktop-launch"
desktop="$3"
work="$installed/../awt-launcher-check-$$"
mkdir "$work"
cp -R "$installed" "$work/relocated image"
home="$work/relocated image"
unset JAVA_HOME DYLD_LIBRARY_PATH CLASSPATH || :
cd "$work"
"$home/bin/javac" -d . "$source/tests/awt-dr2/InstalledAwt115.java"
echo 'Installed DR2 AWT and audio libraries passed' > expected
for tool in java jre; do
    # JRE takes an application path separately from its default runtime path.
    set -- InstalledAwt115
    if test "$tool" = jre; then set -- -cp . InstalledAwt115; fi
    "$launcher" "$desktop" "$home/bin/$tool" "$@" > "$tool.actual" 2> "$tool.errors" &
    pid=$!
    count=0
    while kill -0 $pid 2>/dev/null; do
        count=`expr $count + 1`
        if test "$count" -ge 30; then
            kill $pid
            wait $pid || :
            cat "$tool.actual" "$tool.errors"
            echo 'Installed AWT check timed out' >&2
            exit 1
        fi
        sleep 1
    done
    status=0
    wait $pid || status=$?
    cat "$tool.actual" "$tool.errors"
    test "$status" -eq 0
    diff expected "$tool.actual"
    test ! -s "$tool.errors"
done
cmp "$build/lib/libawt.dylib" "$home/lib/libawt.dylib"
cmp "$build/lib/libmmedia.dylib" "$home/lib/libmmedia.dylib"
echo "Installed AWT launchers passed: $work"
