#!/bin/sh
set -eu
framework=`cd "$1" && pwd -P`
work=$2
desktop=${3-}
root=`pwd -P`
# Only generated images inside this checkout may be made temporarily unavailable.
case "$framework" in "$root"/out/*/JavaVM.framework) ;; *) echo 'Expected a generated framework under out/' >&2; exit 1;; esac
mkdir -p "$work/Relocated Frameworks With Spaces/JavaVM.framework"
work=`cd "$work" && pwd -P`
case "$work" in "$framework"|"$framework"/*) echo 'Relocation work must be outside the source framework' >&2; exit 1;; esac
moved="$work/Relocated Frameworks With Spaces/JavaVM.framework"
backup="$framework.relocation-disabled-$$"
test ! -e "$backup"
/bin/sh tests/framework-dr2/framework-layout.sh "$framework" "$work/original-test"
/bin/sh tests/framework-dr2/framework-embedding.sh "$framework" "$work/original-embedding"
(cd "$framework" && tar cf - .) | (cd "$moved" && tar xf -)
restore()
{
    status=$1
    if test -d "$backup"; then mv "$backup" "$framework" || exit 1; fi
    exit "$status"
}
trap 'restore $?' 0
trap 'exit 1' 1 2 3 15
mv "$framework" "$backup"
/bin/sh tests/framework-dr2/framework-layout.sh "$moved" "$work/relocated-test"
/bin/sh tests/framework-dr2/framework-embedding.sh "$moved" "$work/relocated-embedding"
# Both clients linked before relocation must select the moved facade.
unset DYLD_LIBRARY_PATH || :
DYLD_FRAMEWORK_PATH="$work/Relocated Frameworks With Spaces" "$work/original-test/client" "$moved/"
CLASSPATH="$work/original-embedding" DYLD_FRAMEWORK_PATH="$work/Relocated Frameworks With Spaces" "$work/original-embedding/client" "$moved/" "$work/original-embedding/FrameworkDefined.class"
if test -n "$desktop"; then
    /bin/sh tests/framework-dr2/framework-runtime.sh "$moved" "$work/relocated-runtime" "$desktop"
fi
echo 'PASS framework relocation with spaces and original image unavailable'
