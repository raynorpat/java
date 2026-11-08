#!/bin/sh
# Run from the source root against a private reconstructed image.
set -e
JAVA_HOME=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$JAVA_HOME/lib"
export JAVA_HOME DYLD_LIBRARY_PATH
mkdir -p "$JAVA_HOME/tests"
for name in Hello115 NumericConversions115 ReflectValues SerialValues MonitorCache115 Interrupt; do
    "$JAVA_HOME/bin/java" -classpath "tests/vm:$JAVA_HOME/lib/classes.zip" "$name" > "$JAVA_HOME/tests/$name.actual"
    tr -d '\015' < "tests/vm/$name.expected" > "$JAVA_HOME/tests/$name.expected"
    diff "$JAVA_HOME/tests/$name.expected" "$JAVA_HOME/tests/$name.actual"
    echo "$name passed"
done
