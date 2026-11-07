"""Audit Solaris Motif class references against an unchanged JDK 1.1.5 archive.

This checks declarations and hierarchy resolution, not access rules, bytecode
verification, dynamic reflection, native layouts or runtime compatibility.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile


def parse_class(data):
    offset = 0

    def read(fmt):
        nonlocal offset
        values = struct.unpack_from(">" + fmt, data, offset)
        offset += struct.calcsize(">" + fmt)
        return values[0] if len(values) == 1 else values

    if read("I") != 0xCAFEBABE:
        raise ValueError("Not a class file")
    minor, major = read("HH")
    if major != 45:
        raise ValueError("Expected a Java 1.1 class file")
    pool = [None] * read("H")
    index = 1
    while index < len(pool):
        tag = read("B")
        if tag == 1:
            length = read("H")
            # Class/member names use UTF-8; modified NUL and surrogate encodings
            # may also occur in unrelated string constants in the same pool.
            value = data[offset:offset + length].replace(b"\xc0\x80", b"\0").decode("utf-8", "surrogatepass")
            offset += length
        elif tag in (3, 4):
            value = read("I")
        elif tag in (5, 6):
            value = read("Q")
        elif tag in (7, 8):
            value = read("H")
        elif tag in (9, 10, 11, 12):
            value = read("HH")
        else:
            raise ValueError("Unsupported constant pool tag %d" % tag)
        pool[index] = (tag, value)
        index += 2 if tag in (5, 6) else 1

    def utf(index):
        return pool[index][1]

    def class_name(index):
        return utf(pool[index][1]) if index else None

    def skip_attributes():
        nonlocal offset
        for _ in range(read("H")):
            _, length = read("HI")
            offset += length

    def members():
        result = []
        for _ in range(read("H")):
            access, name, descriptor = read("HHH")
            result.append({"access": access, "name": utf(name), "descriptor": utf(descriptor)})
            skip_attributes()
        return result

    access, name, superclass = read("HHH")
    interfaces = [class_name(read("H")) for _ in range(read("H"))]
    fields, methods = members(), members()
    skip_attributes()
    if offset != len(data):
        raise ValueError("Unexpected class-file length")
    references = []
    for entry in pool[1:]:
        if entry and entry[0] in (9, 10, 11):
            owner, signature = entry[1]
            member_name, descriptor = pool[signature][1]
            references.append({"kind": "fields" if entry[0] == 9 else "methods",
                               "owner": class_name(owner), "name": utf(member_name),
                               "descriptor": utf(descriptor)})
    return {"name": class_name(name), "version": [major, minor], "access": access,
            "super": class_name(superclass), "interfaces": interfaces,
            "fields": fields, "methods": methods, "references": references,
            "class_references": sorted({class_name(i) for i, entry in enumerate(pool)
                                        if entry and entry[0] == 7})}


def read_archive(path):
    with zipfile.ZipFile(path) as archive:
        classes = [parse_class(archive.read(name)) for name in archive.namelist()
                   if name.endswith(".class")]
    return {item["name"]: item for item in classes}


def has_member(classes, owner, kind, name, descriptor, visited=None):
    visited = set() if visited is None else visited
    if owner in visited or owner not in classes:
        return False
    visited.add(owner)
    item = classes[owner]
    if any(m["name"] == name and m["descriptor"] == descriptor for m in item[kind]):
        return True
    if name in ("<init>", "<clinit>"):
        return False
    return any(has_member(classes, parent, kind, name, descriptor, visited)
               for parent in [item["super"]] + item["interfaces"] if parent)


def audit(base_path, solaris_path):
    base, solaris = read_archive(base_path), read_archive(solaris_path)
    peers = {n: c for n, c in solaris.items() if n.startswith("sun/awt/motif/")}
    if set(peers) & set(base):
        raise ValueError("Baseline already contains Motif classes; overlay would replace them")
    overlay = dict(base, **peers)
    entries = set(Path(__file__).with_name("solaris118-awt-native-entries.txt").read_text().splitlines())
    missing_classes, missing_members, referenced, natives = set(), [], set(), []
    for owner, peer in sorted(peers.items()):
        dependencies = set()
        for name in peer["class_references"]:
            dependencies.update(re.findall(r"L([^;]+);", name) if name.startswith("[") else [name])
        for member in peer["fields"] + peer["methods"] + peer["references"]:
            dependencies.update(re.findall(r"L([^;]+);", member["descriptor"]))
        referenced.update(dependencies)
        missing_classes.update(dependencies - overlay.keys())
        for ref in peer["references"]:
            # Array clone is a VM-defined member, not an archive declaration.
            if ref["owner"].startswith("["):
                continue
            if ref["owner"] in overlay and not has_member(overlay, ref["owner"], ref["kind"],
                                                         ref["name"], ref["descriptor"]):
                missing_members.append(dict(source=owner, **ref))
        for method in peer["methods"]:
            if method["access"] & 0x100:
                symbol = "Java_" + owner.replace("/", "_") + "_" + method["name"] + "_stub"
                natives.append(dict(owner=owner, legacy_stub=symbol, entry_present=symbol in entries,
                                    **method))
    layouts = []
    for name in sorted(referenced & base.keys() & solaris.keys()):
        old = [(f["name"], f["descriptor"]) for f in base[name]["fields"] if not f["access"] & 8]
        new = [(f["name"], f["descriptor"]) for f in solaris[name]["fields"] if not f["access"] & 8]
        if old != new:
            layouts.append({"class": name, "baseline_declared_fields": old,
                            "solaris_declared_fields": new})
    return {"baseline_sha256": hashlib.sha256(Path(base_path).read_bytes()).hexdigest(),
            "solaris_sha256": hashlib.sha256(Path(solaris_path).read_bytes()).hexdigest(),
            "motif_classes": sorted(peers), "native_methods": natives,
            "missing_classes": sorted(missing_classes), "missing_members": missing_members,
            "changed_declared_instance_fields": layouts}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("solaris", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.baseline, args.solaris)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Motif classes=%d native methods=%d missing classes=%d missing members=%d changed layouts=%d" %
          tuple(len(result[key]) for key in ("motif_classes", "native_methods", "missing_classes",
                                             "missing_members", "changed_declared_instance_fields")))


if __name__ == "__main__":
    main()
