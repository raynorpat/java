"""Check the generated peer archive's exact compatibility boundary."""
import importlib.util
from pathlib import Path
import sys
import zipfile

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("audit", Path(__file__).resolve().parents[2] / "tools/awt118-audit.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
base, solaris, output = map(Path, sys.argv[1:])
baseline = audit.read_archive(base)
with zipfile.ZipFile(solaris) as original, zipfile.ZipFile(output / "motif-peers.zip") as peers:
    names = [n for n in peers.namelist() if n.endswith(".class")]
    assert len(names) == 76
    assert not any(n[:-6] in baseline for n in names)
    for name in names:
        if name == "sun/awt/im/InputContext.class":
            before = audit.parse_class(original.read(name))
            after = audit.parse_class(peers.read(name))
            for key in ("name", "super", "interfaces", "fields"):
                assert before[key] == after[key], key
            signature = lambda m: (m["name"], m["descriptor"], m["access"])
            assert sorted(map(signature, before["methods"])) == sorted(map(signature, after["methods"]))
        else:
            assert peers.read(name) == original.read(name), name
    for resource in ("printcontrol", "updialog"):
        name = "sun/awt/motif/resources/" + resource + ".properties"
        assert peers.read(name) == original.read(name)
    assert peers.read("META-INF/SOLARIS-LICENSE")
    assert peers.read("META-INF/SOLARIS-COPYRIGHT")
selected = audit.read_archive(output / "motif-peers.zip")
closure = audit.check_overlay(baseline, selected)
assert not closure["missing_classes"] and not closure["missing_members"]
try:
    audit.check_overlay(baseline, {"java/lang/Object": baseline["java/lang/Object"]})
except ValueError:
    pass
else:
    raise AssertionError("Baseline replacement was accepted")
print("76 classes; one compatible adaptation; resources and baseline preserved")
