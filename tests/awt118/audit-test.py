"""Structural audit checks against the unchanged supplied class archives."""
import importlib.util
from pathlib import Path
import sys
import unittest
import zipfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
BASE, SOLARIS = map(Path, sys.argv[1:3])
del sys.argv[1:3]


class AuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tool = ROOT / "tools/awt118-audit.py"
        if not tool.exists():
            raise AssertionError("Motif compatibility audit tool is missing")
        spec = importlib.util.spec_from_file_location("awt118_audit", tool)
        cls.audit = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.audit)

    def test_original_native_descriptors(self):
        with zipfile.ZipFile(SOLARIS) as archive:
            peer = self.audit.parse_class(archive.read("sun/awt/motif/MButtonPeer.class"))
        self.assertEqual(peer["name"], "sun/awt/motif/MButtonPeer")
        self.assertEqual(peer["version"], [45, 3])
        natives = {(m["name"], m["descriptor"]) for m in peer["methods"] if m["access"] & 0x100}
        self.assertIn(("create", "(Lsun/awt/motif/MComponentPeer;)V"), natives)
        self.assertIn(("setLabel", "(Ljava/lang/String;)V"), natives)

    def test_inherited_lookup_and_missing_member(self):
        base = self.audit.read_archive(BASE)
        solaris = self.audit.read_archive(SOLARIS)
        overlay = dict(base)
        overlay.update((n, c) for n, c in solaris.items() if n.startswith("sun/awt/motif/"))
        self.assertTrue(self.audit.has_member(overlay, "sun/awt/motif/MButtonPeer",
                                             "methods", "getClass", "()Ljava/lang/Class;"))
        self.assertFalse(self.audit.has_member(overlay, "sun/awt/motif/MButtonPeer",
                                              "methods", "absentAuditMethod", "()V"))
        self.assertFalse(self.audit.has_member(overlay, "sun/awt/motif/MButtonPeer",
                                              "methods", "getClass", "()I"))
        self.assertFalse(self.audit.has_member(overlay, "sun/awt/motif/MButtonPeer",
                                              "methods", "<init>", "()V"))
        self.assertTrue(self.audit.has_member(overlay, "sun/awt/motif/MButtonPeer",
                                             "fields", "target", "Ljava/awt/Component;"))
        self.assertTrue(self.audit.has_member(base, "java/awt/peer/ButtonPeer",
                                             "methods", "setFont", "(Ljava/awt/Font;)V"))

    def test_audit_preserves_baseline_and_reports_all_peers(self):
        result = self.audit.audit(BASE, SOLARIS)
        self.assertEqual(len(result["motif_classes"]), 67)
        self.assertEqual(result["baseline_sha256"],
                         "750757205976f91cf54017dd4193af0f2abab7e9c829c6a9761389fe090fbb2d")
        self.assertEqual(result["solaris_sha256"],
                         "63459d23a40d1f64c7c37d7f4b87c0affaea2b873df2dc54db25274fdfbfab29")
        self.assertEqual(len(result["native_methods"]), 199)
        self.assertTrue(all(m["entry_present"] for m in result["native_methods"]))
        self.assertEqual(result["missing_classes"], [
            "sun/awt/X11DrawingSurface", "sun/awt/im/InputMethod",
            "sun/awt/im/InputMethodAdapter", "sun/io/CharToByteEUC_CN",
            "sun/io/CharToByteEUC_KR", "sun/io/CharToByteEUC_TW", "sun/io/CharToByteISO8859_1"])
        self.assertEqual(result["missing_members"], [])
        self.assertEqual(len(result["changed_declared_instance_fields"]), 7)

    def test_support_closure_exposes_toolkit_contract(self):
        self.assertTrue(hasattr(self.audit, "peer_closure"), "Support closure audit is missing")
        result = self.audit.peer_closure(self.audit.read_archive(BASE), self.audit.read_archive(SOLARIS))
        self.assertEqual(len(result["classes"]), 76)
        self.assertEqual(result["missing_classes"], [])
        self.assertEqual(result["missing_members"], [{
            "source": "sun/awt/im/InputContext", "kind": "methods",
            "owner": "sun/awt/SunToolkit", "name": "getInputMethodAdapter",
            "descriptor": "()Lsun/awt/im/InputMethod;"}])


if __name__ == "__main__":
    unittest.main()
