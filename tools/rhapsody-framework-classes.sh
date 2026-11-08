#!/bin/sh
set -eu
work=$1
framework=$2
runtime=$3
jdk=$4
apple=$5
root=`pwd`
DYLD_LIBRARY_PATH="$root/$runtime/lib"
export DYLD_LIBRARY_PATH
stage="$work/archive-stage-$$"
mkdir -p "$work" "$framework/Classes" "$framework/Versions/A/Resources"
mkdir "$stage"
trap 'rm -rf "$stage"' 0 1 2 3 15
mkdir "$stage/retained" "$stage/overlay"
# Only class files from the retained DR2 archive; never any donor native library.
(cd "$apple" && find com -name '*.class' -print) > "$stage/apple-classes.list"
(cd "$apple" && tar cf - `cat "$root/$stage/apple-classes.list"`) | (cd "$stage/retained" && tar xf -)
"$runtime/bin/javac" -classpath "$jdk/lib/classes.zip" -d "$stage/overlay" src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java
cp -R "$stage/overlay/." "$stage/retained/"
cp "$jdk/lib/classes.zip" "$stage/classes.zip"
# DR2's 1.1 jar cannot update archives. Native zip preserves the baseline entries.
(cd "$stage/retained" && /usr/bin/zip -q -0 -r "$root/$stage/classes.zip" com)
mv "$stage/classes.zip" "$framework/Classes/classes.zip"
cp "$stage/apple-classes.list" "$framework/Versions/A/Resources/apple-java-classes.list"
(cd "$apple" && cksum `cat "$root/$stage/apple-classes.list"`) > "$framework/Versions/A/Resources/apple-java-classes.cksum"
