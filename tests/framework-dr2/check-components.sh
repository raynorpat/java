#!/bin/sh
# Component acceptance only: full framework startup/packaging remains separate.
set -eu
for component in configuration facade vm-options vm-live signature strings selectors invocation; do
    /bin/sh "tests/framework-dr2/check-$component.sh"
done
echo 'PASS eight framework component groups (not full framework acceptance)'
