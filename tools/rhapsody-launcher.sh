#!/bin/sh
# DR2 dyld predates @loader_path. Keep native binaries beside the private libs.
bindir=`dirname "$0"`
home=`cd "$bindir/.." && pwd` || exit 1
name=`basename "$0"`
DYLD_LIBRARY_PATH="$home/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH
exec "$home/libexec/$name" "$@"
