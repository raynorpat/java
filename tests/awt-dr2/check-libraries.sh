#!/bin/sh
set -e
JAVA_HOME=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$JAVA_HOME/lib"
export JAVA_HOME DYLD_LIBRARY_PATH
mkdir -p "$JAVA_HOME/tests/jpeg" "$JAVA_HOME/tests/jniheaders"
for name in ZipLibrary115 MathLibrary115 ResourceLibrary115; do
    "$JAVA_HOME/bin/java" -classpath "tests/vm:tests/vm/resources115.zip:$JAVA_HOME/lib/classes.zip" "$name" "$JAVA_HOME/tests" > "$JAVA_HOME/tests/$name.actual"
    tr -d '\015' < "tests/vm/$name.expected" > "$JAVA_HOME/tests/$name.expected"
    diff "$JAVA_HOME/tests/$name.expected" "$JAVA_HOME/tests/$name.actual"
    echo "$name passed"
done
"$JAVA_HOME/bin/java" -classpath "tests/vm:tests/vm/resources:$JAVA_HOME/lib/classes.zip" ResourceLibrary115 > "$JAVA_HOME/tests/resource-file.actual"
diff "$JAVA_HOME/tests/ResourceLibrary115.expected" "$JAVA_HOME/tests/resource-file.actual"
"$JAVA_HOME/tests/jpeg-fixtures" "$JAVA_HOME/tests/jpeg"
"$JAVA_HOME/bin/java" -classpath "tests/vm:$JAVA_HOME/lib/classes.zip" JpegLibrary115 "$JAVA_HOME/tests/jpeg" > "$JAVA_HOME/tests/jpeg.actual" 2> "$JAVA_HOME/tests/jpeg.stderr"
tr -d '\015' < tests/vm/JpegLibrary115.expected > "$JAVA_HOME/tests/jpeg.expected"
tr -d '\015' < tests/vm/JpegLibrary115.stderr > "$JAVA_HOME/tests/jpeg.expected-stderr"
diff "$JAVA_HOME/tests/jpeg.expected" "$JAVA_HOME/tests/jpeg.actual"
diff "$JAVA_HOME/tests/jpeg.expected-stderr" "$JAVA_HOME/tests/jpeg.stderr"
echo 'JpegLibrary115 passed'
"$JAVA_HOME/bin/javah" -jni -classpath "tests/native:$JAVA_HOME/lib/classes.zip" -d "$JAVA_HOME/tests/jniheaders" JNI_Test NativeHello
tr -d '\015' < tests/native/JNI_Test.jni.expected.h > "$JAVA_HOME/tests/jni.expected"
diff "$JAVA_HOME/tests/jni.expected" "$JAVA_HOME/tests/jniheaders/JNI_0005fTest.h"
tr -d '\015' < tests/native/NativeHello.jni.expected.h > "$JAVA_HOME/tests/jni.expected"
diff "$JAVA_HOME/tests/jni.expected" "$JAVA_HOME/tests/jniheaders/NativeHello.h"
echo 'javah JNI headers passed'
"$JAVA_HOME/tests/agent-status"
"$JAVA_HOME/tests/library-path"
