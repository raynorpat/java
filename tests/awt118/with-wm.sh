#!/bin/sh
set -eu
openbox >/dev/null 2>&1 &
wm=$!
trap 'kill "$wm" 2>/dev/null || true' EXIT
sleep 1
"$@"
