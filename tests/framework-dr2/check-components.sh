#!/bin/sh
# Component acceptance only: full framework startup/packaging remains separate.
set -eu
for component in configuration facade vm-options vm-live signature strings selectors invocation constructors class-map classgen object-map retention exceptions; do
    /bin/sh "tests/framework-dr2/check-$component.sh"
done
echo 'PASS fourteen framework component groups (not full framework acceptance)'
