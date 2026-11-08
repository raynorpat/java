#!/bin/sh
# Run after building the requested Release or Debug framework image.
set -eu
framework=$1
work=$2
desktop=$3
/bin/sh tests/framework-dr2/framework-layout.sh "$framework" "$work/layout"
/bin/sh tests/framework-dr2/framework-embedding.sh "$framework" "$work/embedding"
/bin/sh tests/framework-dr2/framework-fresh-check.sh "$framework" "$work/runtime" "$desktop"
/bin/sh tests/framework-dr2/framework-layout-relocate.sh "$framework" "$work/relocation" "$desktop"
echo 'PASS complete private framework acceptance'
