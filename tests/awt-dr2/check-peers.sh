#!/bin/sh
# Run from the guest source root with a private image and host-prepared peers.
set -e
home=`cd "$1" && pwd`
peers=`cd "$2" && pwd`
mkdir -p "$peers/tests" "$peers/generated"
"$home/bin/javac" -classpath "$home/lib/classes.zip:$peers/rhapsody-peers.zip" -d "$peers/tests" tests/awt-dr2/PeerLink115.java
"$home/bin/java" -classpath "$peers/tests:$home/lib/classes.zip" PeerLink115 "$peers/rhapsody-peers.zip"
"$home/bin/javah" -jni -classpath "$home/lib/classes.zip:$peers/rhapsody-peers.zip" -d "$peers/generated" `cat "$peers/native-classes.txt"`
