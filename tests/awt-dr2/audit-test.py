"""Pin the supplied AppKit peer boundary without replacing baseline classes."""
import importlib.util
from pathlib import Path
import sys
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]


class AuditTest(unittest.TestCase):
    def test_original_boundary(self):
        tool = ROOT / 'tools/rhapsody115-audit.py'
        self.assertTrue(tool.exists(), 'Rhapsody audit tool missing')
        spec = importlib.util.spec_from_file_location('dr2_audit', tool)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        result = module.audit(ROOT / 'vendor/jdk1.1.5/lib/classes.zip',
                              ROOT / 'vendor/rhapsody-dr2')
        self.assertEqual(result['baseline_sha256'],
                         '750757205976f91cf54017dd4193af0f2abab7e9c829c6a9761389fe090fbb2d')
        self.assertEqual(len(result['classes']), 38)
        self.assertEqual(len(result['baseline_duplicates_excluded']), 13)
        self.assertFalse(result['missing_classes'])
        self.assertFalse(result['missing_members'])
        self.assertFalse(result['changed_declared_instance_fields'])
        hashes = result['donor_sha256']
        prefix = 'System/Library/Frameworks/JavaVM.framework/'
        self.assertEqual(hashes[prefix + 'Classes/awt.jar'],
                         '3d252bd54aa02ec6f7a0e291eddbf558afdf94d4d8a638edbdcef272b007057e')
        self.assertEqual(hashes[prefix + 'Classes/classes.jar'],
                         '4beeba1d9d1a86ecc4378a376dfd5ad82bcb34570be1286e861cdcb31b0798cd')
        self.assertEqual(hashes[prefix + 'Libraries/libawt.A.dylib'],
                         '0e1b5ea4f25f85d9870ed2ac279e7356ac3eba42d280a161933e03f1e5b6a846')
        self.assertEqual(len(result['native_methods']), 168)
        self.assertTrue(all(n['entry_present'] for n in result['native_methods']))
        self.assertFalse(set(result['classes']) & set(result['baseline_duplicates_excluded']))
        button = [n for n in result['native_methods'] if n['owner'].endswith('/RButtonPeer')
                  and n['name'] == 'setLabel']
        self.assertEqual(button[0]['descriptor'], '(Ljava/lang/String;)V')
        peek = {n['symbol'] for n in result['native_methods'] if n['name'] == 'peekEvent'}
        self.assertEqual(peek, {'_Java_com_apple_rhapsody_awt_REventQueue_peekEvent__',
                                '_Java_com_apple_rhapsody_awt_REventQueue_peekEvent__I'})


if __name__ == '__main__':
    unittest.main()
