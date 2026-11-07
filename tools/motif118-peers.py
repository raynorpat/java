"""Prepare and validate the separate Solaris-derived Motif peer archive."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import zipfile

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("awt118_audit", Path(__file__).with_name("awt118-audit.py"))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["prepare", "finish"])
    parser.add_argument("baseline", type=Path)
    parser.add_argument("solaris", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    archive = args.solaris / "SUNWjvrt/reloc/usr/java1.1/lib/classes.zip"
    for path, expected in [(args.baseline, "750757205976f91cf54017dd4193af0f2abab7e9c829c6a9761389fe090fbb2d"),
                           (archive, "63459d23a40d1f64c7c37d7f4b87c0affaea2b873df2dc54db25274fdfbfab29")]:
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError("Unsupported class archive: " + str(path))
    base, original = audit.read_archive(args.baseline), audit.read_archive(archive)
    closure = audit.peer_closure(base, original)
    if closure["missing_classes"]:
        raise ValueError("Unresolved support classes: " + str(closure["missing_classes"]))
    args.output.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        files = {name + ".class": source.read(name + ".class") for name in closure["classes"]}
        resources = {name: source.read(name) for name in source.namelist()
                     if name.startswith("sun/awt/motif/resources/") and name.endswith(".properties")}
    if args.mode == "finish":
        name = "sun/awt/im/InputContext.class"
        files[name] = (args.output / "overrides" / name).read_bytes()
        selected = {name[:-6]: audit.parse_class(data) for name, data in files.items()}
        closure = audit.check_overlay(base, selected)
        if closure["missing_classes"] or closure["missing_members"]:
            raise ValueError("Peer linkage preflight failed: " + json.dumps(closure))
    else:
        selected = {name: original[name] for name in closure["classes"]}
    files.update(resources)
    files["META-INF/SOLARIS-LICENSE"] = (args.solaris / "LICENSE").read_bytes()
    files["META-INF/SOLARIS-COPYRIGHT"] = (args.solaris / "SUNWjvrt/reloc/usr/java1.1/COPYRIGHT").read_bytes()
    filename = "motif-peers.zip" if args.mode == "finish" else "motif-raw.zip"
    with zipfile.ZipFile(args.output / filename, "w") as target:
        for name, data in sorted(files.items()):
            target.writestr(zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0)), data)
    (args.output / (args.mode + "-closure.json")).write_text(json.dumps(closure, indent=2) + "\n")
    owners = sorted(n.replace("/", ".") for n, c in selected.items()
                    if any(m["access"] & 0x100 for m in c["methods"]))
    (args.output / "native-classes.txt").write_text("\n".join(owners) + "\n")
    (args.output / "peer-classes.txt").write_text("\n".join(n.replace("/", ".") for n in sorted(selected)) + "\n")
    print("%s: %d classes, %d native owners, %d unresolved members" %
          (filename, len(selected), len(owners), len(closure["missing_members"])))


if __name__ == "__main__":
    main()
