#!/bin/sh
# Commands and Home/bin both resolve to this same physical Commands directory.
set -e
commands=`cd "\`dirname "$0"\`" && pwd -P`
framework=`cd "$commands/.." && pwd -P`
name=`basename "$0"`
DYLD_LIBRARY_PATH="$framework/Libraries"
JAVA_HOME="$framework/Home"
export DYLD_LIBRARY_PATH JAVA_HOME
exec "$framework/Home/libexec/$name" "$@"
