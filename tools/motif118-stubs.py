"""Select recovered bindings from complete javah output; no placeholder exports."""
from pathlib import Path
import re
import sys

manifest, generated, output = map(Path, sys.argv[1:])
names = manifest.read_text().splitlines()
if len(names) != len(set(names)):
    raise ValueError("Duplicate implemented binding")
stubs = {}
for path in list(generated.glob("sun_awt_motif_*.c")) + list(generated.glob("sun_awt_image_*.c")):
    for match in re.finditer(r"JAVA_PUBLIC_API\(stack_item\*\)\s+Java_(\w+)_stub\([^}]+^}", path.read_text(), re.M):
        stubs[match[1]] = match[0]
missing = set(names) - stubs.keys()
if missing:
    raise ValueError("Implemented binding absent from javah output: " + str(sorted(missing)))
output.write_text('#include "StubPreamble.h"\n\n' + "\n\n".join(stubs[name] for name in names) + "\n")
print("Selected %d recovered native bindings" % len(names))
