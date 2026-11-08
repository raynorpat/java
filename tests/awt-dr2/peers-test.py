"""The AppKit overlay must retain donor class bytes and never replace the core."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]


class PeerArchiveTest(unittest.TestCase):
    def test_exact_separate_archive(self):
        baseline = ROOT / "vendor/jdk1.1.5/lib/classes.zip"
        donor = ROOT / "vendor/rhapsody-dr2"
        before = hashlib.sha256(baseline.read_bytes()).hexdigest()
        with tempfile.TemporaryDirectory(prefix="dr2-peers-") as temporary:
            output = Path(temporary)
            command = [sys.executable, str(ROOT / "tools/rhapsody115-peers.py"),
                       str(baseline), str(donor), str(output)]
            subprocess.run(command, check=True)
            archive = output / "rhapsody-peers.zip"
            first = archive.read_bytes()
            framework = donor / "System/Library/Frameworks/JavaVM.framework/Classes"
            with zipfile.ZipFile(framework / "classes.jar") as core, \
                    zipfile.ZipFile(framework / "awt.jar") as awt, \
                    zipfile.ZipFile(baseline) as base, zipfile.ZipFile(archive) as peers:
                names = peers.namelist()
                self.assertEqual(len(names), 38)
                self.assertEqual(names, sorted(names))
                self.assertFalse(set(names) & set(base.namelist()))
                for name in names:
                    self.assertTrue(name.endswith(".class"))
                    original = awt if name in awt.namelist() else core
                    self.assertEqual(peers.read(name), original.read(name))
            subprocess.run(command, check=True)
            self.assertEqual(first, archive.read_bytes())
            self.assertEqual(before, hashlib.sha256(baseline.read_bytes()).hexdigest())
            self.assertEqual(len((output / "peer-classes.txt").read_text().splitlines()), 38)


if __name__ == "__main__":
    unittest.main()
