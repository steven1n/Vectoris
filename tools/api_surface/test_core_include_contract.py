"""Controlled mutations of the real published-entrance validator (no fixture skip)."""
import json
from pathlib import Path
import tempfile
import unittest
import sys
from verify_core_include_contract import read_contracts

ROOT = Path(__file__).resolve().parents[2]


class CoreIncludeContractTests(unittest.TestCase):
    def validate(self, mutate_document=None, mutate_manifest=None):
        text = (ROOT / 'docs/core.md').read_text(encoding='utf-8')
        manifest = json.loads((ROOT / 'tools/api_surface/public_api_manifest.json').read_text(encoding='utf-8'))
        if mutate_document:
            text = mutate_document(text)
        if mutate_manifest:
            mutate_manifest(manifest)
        with tempfile.TemporaryDirectory() as tmp:
            document = Path(tmp) / 'core.md'
            spec = Path(tmp) / 'manifest.json'
            document.write_text(text, encoding='utf-8')
            spec.write_text(json.dumps(manifest), encoding='utf-8')
            return read_contracts(ROOT, document, spec)

    def test_current_contract(self):
        self.assertEqual(set(self.validate()), {'primitives', 'wrappers', 'traits', 'constants'})

    def test_old_umbrella_claim(self):
        with self.assertRaisesRegex(ValueError, 'narrow primitive'):
            self.validate(lambda text: text.replace('Primitive entry:', 'Root namespace umbrella:'))

    def test_missing_source_identity(self):
        with self.assertRaisesRegex(ValueError, 'source identities'):
            self.validate(lambda text: text.replace('vectoris-core-contract: wrappers', 'removed-contract: wrappers'))

    def test_duplicate_source_identity(self):
        with self.assertRaisesRegex(ValueError, 'source identities'):
            self.validate(lambda text: text.replace('vectoris-core-contract: constants', 'vectoris-core-contract: wrappers'))

    def test_unexpected_source_identity(self):
        with self.assertRaisesRegex(ValueError, 'source identities'):
            self.validate(lambda text: text.replace('vectoris-core-contract: wrappers', 'vectoris-core-contract: unexpected'))

    def test_wrong_include_entrance(self):
        with self.assertRaisesRegex(ValueError, 'include entrance'):
            self.validate(lambda text: text.replace('#include <Vectoris/Numerics/Core/MathFunctions.h>', '#include <Vectoris/Numerics/Core/Math.h>'))

    def test_empty_consumer_body(self):
        with self.assertRaisesRegex(ValueError, 'assertions missing'):
            self.validate(lambda text: text.replace('core::Math::sin(', 'core::Math::removed('))

    def test_missing_manifest_identity(self):
        with self.assertRaisesRegex(ValueError, 'manifest entrance'):
            self.validate(mutate_manifest=lambda manifest: manifest['documented_core_include_contracts'].pop('wrappers'))

    def test_unknown_manifest_identity(self):
        with self.assertRaisesRegex(ValueError, 'manifest entrance'):
            self.validate(mutate_manifest=lambda manifest: manifest['documented_core_include_contracts'].update({'extra': 'Math.h'}))


if __name__ == '__main__':
    expected = {
        'test_current_contract', 'test_old_umbrella_claim',
        'test_missing_source_identity', 'test_duplicate_source_identity',
        'test_unexpected_source_identity', 'test_wrong_include_entrance',
        'test_empty_consumer_body', 'test_missing_manifest_identity',
        'test_unknown_manifest_identity',
    }
    actual = unittest.defaultTestLoader.getTestCaseNames(CoreIncludeContractTests)
    if len(actual) != len(expected) or set(actual) != expected:
        sys.exit('ERROR: mandatory Core include self-test identities mismatch')
    result = unittest.main(verbosity=2, exit=False).result
    print(f'CORE_CONTRACT_CONTROLS executed={result.testsRun} failed={len(result.failures) + len(result.errors)} skipped={len(result.skipped)}')
    sys.exit(0 if result.wasSuccessful() and result.testsRun == len(expected) and not result.skipped else 1)
