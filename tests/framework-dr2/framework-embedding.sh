#!/bin/sh
set -eu
framework=`cd "$1" && pwd`
work=$2
mkdir -p "$work"
work=`cd "$work" && pwd`
parent=`dirname "$framework"`
"$framework/Commands/javac" -d "$work" tests/framework-dr2/ProxyFixture.java tests/framework-dr2/FrameworkDefined.java
cc -traditional-cpp -F"$parent" tests/framework-dr2/framework-embedding.m -framework JavaVM -framework Foundation -o "$work/client"
unset DYLD_LIBRARY_PATH || :
# Only application classes are supplied. Bootstrap classes must come from the bundle.
CLASSPATH="$work" DYLD_FRAMEWORK_PATH="$parent" "$work/client" "$framework/" "$work/FrameworkDefined.class"
