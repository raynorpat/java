#!/bin/sh
# Run from the source root; retain the private relocated image as test evidence.
set -e
source=`pwd`
installed=`cd "$1" && pwd`
work="$installed/../launcher-check-$$"
mkdir "$work"
cp -R "$installed" "$work/relocated image"
home="$work/relocated image"
unset JAVA_HOME DYLD_LIBRARY_PATH CLASSPATH || :
cd "$work"
mkdir classes
"$home/bin/javac" -d "$work/classes" "$source/tests/vm/Hello115.java"
tr -d '\015' < "$source/tests/vm/Hello115.expected" > expected
"$home/bin/java" -classpath "$work/classes:$home/lib/classes.zip" Hello115 > java.actual
diff expected java.actual
"$home/bin/jre" -cp "$work/classes" Hello115 > jre.actual
diff expected jre.actual
PATH="$home/bin:$PATH"
export PATH
javac -d "$work/classes" "$source/tests/vm/Hello115.java"
java -classpath "$work/classes:$home/lib/classes.zip" Hello115 > path.actual
diff expected path.actual
jre -cp "$work/classes" Hello115 > path-jre.actual
diff expected path-jre.actual
./"relocated image"/bin/java -classpath "$work/classes:$home/lib/classes.zip" Hello115 > relative.actual
diff expected relative.actual
javah -jni -classpath "$source/tests/native:$home/lib/classes.zip" -o NativeHello.h NativeHello
tr -d '\015' < "$source/tests/native/NativeHello.jni.expected.h" > NativeHello.expected
diff NativeHello.expected NativeHello.h
echo "Installed launchers passed: $work"
