"""Package the audited DR2 AppKit class closure without changing class bytes."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import zipfile

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("audit", Path(__file__).with_name("rhapsody115-audit.py"))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def build(baseline, donor, output):
    result = audit.audit(baseline, donor)
    if (result["missing_classes"] or result["missing_members"] or
            result["changed_declared_instance_fields"] or
            not all(entry["entry_present"] for entry in result["native_methods"])):
        raise ValueError("AppKit class/native boundary does not match the baseline")
    output.mkdir(parents=True, exist_ok=True)
    framework = donor / "System/Library/Frameworks/JavaVM.framework/Classes"
    archive = output / "rhapsody-peers.zip"
    with zipfile.ZipFile(framework / "classes.jar") as core, \
            zipfile.ZipFile(framework / "awt.jar") as awt, \
            zipfile.ZipFile(archive, "w") as peers:
        awt_names = set(awt.namelist())
        for owner in sorted(result["classes"]):
            name = owner + ".class"
            original = awt if name in awt_names else core
            # Stored entries and a fixed ZIP timestamp are deterministic and
            # supported by the original VM; no native library enters the ZIP.
            peers.writestr(zipfile.ZipInfo(name), original.read(name))
    for filename, owners in [
            ("peer-classes.txt", result["classes"]),
            ("native-classes.txt", {entry["owner"] for entry in result["native_methods"]})]:
        (output / filename).write_text("".join(owner.replace("/", ".") + "\n"
                                               for owner in sorted(owners)))
    result["peer_archive_sha256"] = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / "peer-audit.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("donor", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    build(args.baseline, args.donor, args.output)
