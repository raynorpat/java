#!/bin/sh
set -eu
work=out/framework-build-dependencies
mkdir -p "$work"
header=src/share/java/include/oobj.h
makefile=build/rhapsody-framework.mk
targets='out/dr2-framework/obj/framework/runtime.o out/dr2-framework/obj/framework/path_md.o out/dr2-framework/obj/framework/link_cthreads.o out/dr2-framework/obj/src/rhapsody/java/bridge/objects.o out/dr2-framework/obj/src/rhapsody/java/bridge/invocation.o'
# Establish current objects, then simulate a header-only change without changing bytes.
/bin/gnumake -s -f "$makefile" $targets
cp -p "$header" "$work/header.timestamp"
cp -p "$makefile" "$work/makefile.timestamp"
restore()
{
    status=$1
    touch -r "$work/header.timestamp" "$header"
    touch -r "$work/makefile.timestamp" "$makefile"
    exit "$status"
}
trap 'restore $?' 0
trap 'exit 1' 1 2 3 15
for changed in "$header" "$makefile"; do
    sleep 1
    touch "$changed"
    for target in $targets; do
        status=0
        /bin/gnumake -q -f "$makefile" "$target" || status=$?
        if test "$status" -ne 1; then
            echo "FAIL $target did not become stale after $changed (status $status)"
            exit 1
        fi
    done
    touch -r "$work/header.timestamp" "$header"
    touch -r "$work/makefile.timestamp" "$makefile"
done
echo 'PASS framework VM header and makefile dependencies'
