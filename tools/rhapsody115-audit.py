"""Structural AppKit peer preflight; not proof of JNI ABI or DR2 runtime behavior."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('class_audit', Path(__file__).with_name('awt118-audit.py'))
classes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(classes)


def audit(baseline, donor_root):
    framework = donor_root / 'System/Library/Frameworks/JavaVM.framework'
    awt_path = framework / 'Classes/awt.jar'
    core_path = framework / 'Classes/classes.jar'
    native_path = framework / 'Libraries/libawt.A.dylib'
    base = classes.read_archive(baseline)
    awt = classes.read_archive(awt_path)
    donor = dict(classes.read_archive(core_path), **awt)
    selected = set(awt) - base.keys()
    while True:
        needed = set().union(*(classes.dependencies(donor[n]) for n in selected))
        available = (needed - base.keys() - selected) & donor.keys()
        if not available:
            break
        selected.update(available)
    result = classes.check_overlay(base, {n: donor[n] for n in selected})
    entries = {line.split()[1]: line.split()[0] for line in
               Path(__file__).with_name('rhapsody115-awt-native-entries.txt').read_text().splitlines()}
    natives = []
    for owner in sorted(selected):
        for method in donor[owner]['methods']:
            if method['access'] & 256:
                symbol = '_Java_' + owner.replace('/', '_') + '_' + method['name']
                if symbol not in entries:
                    parameters = method['descriptor'].split(')')[0][1:]
                    suffix = ''.join(c if c.isascii() and c.isalnum() else
                                     {'/': '_', '_': '_1', ';': '_2', '[': '_3'}.get(c, '_0%04x' % ord(c))
                                     for c in parameters)
                    symbol += '__' + suffix
                natives.append(dict(owner=owner, symbol=symbol, address=entries.get(symbol),
                                    entry_present=symbol in entries, **method))
    referenced = set().union(*(classes.dependencies(donor[n]) for n in selected))
    layouts = []
    for name in sorted(referenced & base.keys() & donor.keys()):
        old = [f for f in base[name]['fields'] if not f['access'] & 8]
        new = [f for f in donor[name]['fields'] if not f['access'] & 8]
        if [(f['name'], f['descriptor']) for f in old] != [(f['name'], f['descriptor']) for f in new]:
            layouts.append(dict(name=name, baseline_fields=old, donor_fields=new))
    result.update(native_methods=natives, baseline_duplicates_excluded=sorted(awt.keys() & base.keys()),
                  changed_declared_instance_fields=layouts,
                  baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                  donor_sha256={str(p.relative_to(donor_root)).replace('\\', '/'):
                                hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in (awt_path, core_path, native_path)})
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('donor_root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.baseline, args.donor_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
