#!/bin/sh
set -e
JAVA_HOME=`cd "$1" && pwd`
DYLD_LIBRARY_PATH="$JAVA_HOME/tests:$JAVA_HOME/lib"
export JAVA_HOME DYLD_LIBRARY_PATH
"$JAVA_HOME/bin/javac" -d "$JAVA_HOME/tests" tests/awt-dr2/ExceptionBridge115.java
echo 'DR2 Java/Objective-C exceptions preserve identity across callbacks and GC' > "$JAVA_HOME/tests/exception.expected"
"$JAVA_HOME/bin/java" -classpath "$JAVA_HOME/tests:$JAVA_HOME/lib/classes.zip" ExceptionBridge115 > "$JAVA_HOME/tests/exception.actual"
diff "$JAVA_HOME/tests/exception.expected" "$JAVA_HOME/tests/exception.actual"
"$JAVA_HOME/bin/jre" -cp "$JAVA_HOME/tests" ExceptionBridge115 > "$JAVA_HOME/tests/exception-jre.actual"
diff "$JAVA_HOME/tests/exception.expected" "$JAVA_HOME/tests/exception-jre.actual"
cat "$JAVA_HOME/tests/exception.actual"
