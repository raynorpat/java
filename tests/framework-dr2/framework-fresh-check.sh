#!/bin/sh
# Exercise runtime acceptance with sources but no prebuilt Release output tree.
set -eu
framework=`cd "$1" && pwd -P`
root=`pwd -P`
work=$2
desktop=$3
mkdir -p "$work"
work=`cd "$work" && pwd -P`
context="$work/context-$$"
mkdir "$context"
ln -s "$root/tests" "$context/tests"
cd "$context"
/bin/sh "$root/tests/framework-dr2/framework-runtime.sh" "$framework" "$work/runtime" "$desktop"
echo 'PASS framework acceptance without prebuilt Release helpers'
