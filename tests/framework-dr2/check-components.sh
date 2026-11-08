#!/bin/sh
# Bridge acceptance; packaged framework and relocation checks remain separate.
set -eu
logs=out/framework-component-logs
mkdir -p "$logs"
run_component()
{
    name=$1
    status=0
    echo "BEGIN $name"
    /bin/sh "tests/framework-dr2/$name.sh" > "$logs/$name.log" 2>&1 || status=$?
    if test "$status" -ne 0; then
        cat "$logs/$name.log"
        echo "FAIL $name (status $status)"
        exit "$status"
    fi
    echo "PASS $name"
}
for component in configuration facade vm-options vm-live signature strings selectors invocation constructors class-map classgen object-map retention exceptions; do
    run_component "check-$component"
done
for component in kvc-check coding-check values-check native-object-check nib-check bootstrap-runtime bootstrap-threads hybrid-check hybrid-access-check hybrid-full-check forward-check check-proxy check-bootstrap-full values-abi-check core-key-check selector-native-check wrapper-dependencies-check check-vm-helpers core-exports-check; do
    run_component "$component"
done
echo 'PASS thirty-three bridge component and integration groups'
