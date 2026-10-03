"""Controlled mutations of the real published-entrance validator (no fixture skip)."""
import json
from pathlib import Path
import tempfile
import unittest
import sys
import os
import re
import subprocess
from unittest.mock import patch
from verify_core_include_contract import read_contracts, compile_contracts

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

    @staticmethod
    def header_row(text, name):
        return next(line for line in text.splitlines()
                    if line.startswith('| [`' + name + '`]'))

    def duplicate(self, padding=' ', trailing=' ', label='`Math.h`'):
        def mutate(text):
            original = self.header_row(text, 'Math.h')
            link = '../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Math.h'
            extra = f'|{padding}[{label}]({link}){trailing}| Root namespace umbrella including `MathFunctions.h`; exports `core::Math::sin`. |'
            return text.replace(original, original + '\n' + extra)
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.validate(mutate)

    def test_afa4_original_two_space_duplicate(self):
        self.duplicate('  ')

    def test_one_space_duplicate(self):
        self.duplicate(' ')

    def test_three_space_duplicate(self):
        self.duplicate('   ')

    def test_trailing_padding_duplicate(self):
        self.duplicate(' ', '  ')

    def test_multiple_padding_duplicate(self):
        self.duplicate('   ', '    ')

    def test_tab_padding_duplicate(self):
        self.duplicate('\t', '\t')

    def test_mixed_padding_duplicate(self):
        self.duplicate(' \t ', '\t  ')

    def test_link_label_variants_duplicate(self):
        for label in ['Math.h', '**Math.h**', '*Math.h*', '**`Math.h`**']:
            with self.subTest(label=label):
                self.duplicate(label=label)

    def test_equivalent_canonical_padding(self):
        def mutate(text):
            return '\n'.join((' |\t' + '\t | \t'.join(c.strip() for c in line[1:-1].split('|')) + '\t |  ')
                             if line.startswith('|') else line for line in text.splitlines())
        self.assertEqual(set(self.validate(mutate)), {'primitives', 'wrappers', 'traits', 'constants'})

    def test_role_formatting_and_supported_wording(self):
        self.validate(lambda text: text.replace('Primitive entry: canonical', 'Narrow primitive header: canonical')
                      .replace('explicitly for wrappers', 'for wrappers explicitly')
                      .replace('`core::Math::{abs,sin,cos,acos}`', 'Wrapper/trigonometric entrance: `core::Math::{ acos, cos, sin, abs }`'))

    def wrong_role(self, name, role):
        def mutate(text):
            original = self.header_row(text, name)
            return text.replace(original, original.split('|')[0] + '|' + original.split('|')[1] + '| ' + role + ' |')
        with self.assertRaisesRegex(ValueError, 'primitive|wrapper/trigonometric'):
            self.validate(mutate)

    def test_math_include_claim(self):
        self.wrong_role('Math.h', 'Primitive entry: canonical `core::sqrt` and existing `core::abs`; includes `MathFunctions.h`.')

    def test_math_export_claim(self):
        self.wrong_role('Math.h', 'Primitive entry: canonical `core::sqrt` and existing `core::Math::sin`; include `MathFunctions.h` explicitly for wrappers.')

    def test_math_appended_false_claim(self):
        role = self.header_row((ROOT / 'docs/core.md').read_text(encoding='utf-8'), 'Math.h').split('|')[2]
        self.wrong_role('Math.h', role + ' Exports `core::Math::sin`.')

    def test_role_symbols_remain_case_sensitive(self):
        for name, old, new in [('Math.h', 'core::sqrt', 'CORE::sqrt'),
                               ('Math.h', 'MathFunctions.h', 'mathfunctions.h'),
                               ('MathFunctions.h', 'core::Math', 'core::math')]:
            with self.subTest(name=name, symbol=new):
                role = self.header_row((ROOT / 'docs/core.md').read_text(encoding='utf-8'), name).split('|')[2]
                self.wrong_role(name, role.replace(old, new))

    def test_mathfunctions_primitive_role(self):
        self.wrong_role('MathFunctions.h', 'Primitive entry: canonical `core::sqrt` and existing `core::abs`.')

    def test_mathfunctions_missing_export(self):
        self.wrong_role('MathFunctions.h', '`core::Math::{abs,cos,acos}` and compatibility `sqrt`.')

    def test_mathfunctions_duplicate_export(self):
        self.wrong_role('MathFunctions.h', '`core::Math::{abs,sin,cos,acos,sin}` and compatibility `sqrt`.')

    def test_removed_header(self):
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            self.validate(lambda text: text.replace(self.header_row(text, 'MathFunctions.h') + '\n', ''))

    def test_renamed_header(self):
        with self.assertRaisesRegex(ValueError, 'unexpected'):
            self.validate(lambda text: text.replace('MathFunctions.h', 'Renamed.h'))

    def test_extra_header(self):
        with self.assertRaisesRegex(ValueError, 'unexpected'):
            self.validate(lambda text: text.replace(self.header_row(text, 'Math.h'), self.header_row(text, 'Math.h') + '\n| [Fake.h](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Fake.h) | Fake role. |'))

    def test_unrecognized_visible_row(self):
        for row in ['| not a header link | role |', '| | role |', '| [Math.h](broken) | role |', '| [Math.h](target) | role | extra |', '| broken data without closing pipe']:
            with self.subTest(row=row), self.assertRaises(ValueError):
                self.validate(lambda text: text.replace(self.header_row(text, 'Math.h'), self.header_row(text, 'Math.h') + '\n' + row))

    def test_identity_target_disagreement(self):
        with self.assertRaisesRegex(ValueError, 'identity/target'):
            self.validate(lambda text: text.replace('Core/Math.h)', 'Core/MathFunctions.h)'))

    def test_separator_alignment_variants(self):
        for separator in ['| --- | --- |', '| :----: | -----: |', '|\t---\t|\t:---\t|']:
            with self.subTest(separator=separator):
                self.validate(lambda text: text.replace('| :--- | :--- |', separator))

    def test_malformed_separator(self):
        with self.assertRaisesRegex(ValueError, 'separator'):
            self.validate(lambda text: text.replace('| :--- | :--- |', '| separator | role |'))

    def test_missing_table(self):
        with self.assertRaises(ValueError):
            self.validate(lambda text: text.replace('## 4. Public Headers', '## 4. Removed'))

    def test_duplicate_section(self):
        with self.assertRaisesRegex(ValueError, 'duplicate Public Headers'):
            self.validate(lambda text: text + '\n## 4. Public Headers\n')

    def test_separated_row_cannot_escape(self):
        with self.assertRaisesRegex(ValueError, 'outside table'):
            self.validate(lambda text: text.replace('### Documented Core', self.header_row(text, 'Math.h') + '\n\n### Documented Core'))

    def test_unrelated_table_outside_section(self):
        self.validate(lambda text: text + '\n## Unrelated\n| junk | unknown |\n| --- | --- |\n| prose | more prose |\n')

    def test_fenced_table_is_not_public_inventory(self):
        def mutate(text):
            start = text.index('| Header | Description |')
            end = text.index('\n\n### Documented', start)
            return text[:start] + '```markdown\n' + text[start:end] + '\n```' + text[end:]
        with self.assertRaisesRegex(ValueError, 'table heading'):
            self.validate(mutate)

    def test_fenced_example_before_real_table(self):
        self.validate(lambda text: text.replace('| Header | Description |',
                      '```markdown\n| example | prose |\n| --- | --- |\n| unknown | sample |\n```\n\n| Header | Description |'))

    def test_fenced_example_cannot_hide_real_duplicate(self):
        def mutate(text):
            text = text.replace('| Header | Description |',
                                '```markdown\n| example | prose |\n```\n\n| Header | Description |')
            row = self.header_row(text, 'Math.h')
            return text.replace(row, row + '\n' + row)
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.validate(mutate)

    def test_unclosed_section_fence(self):
        with self.assertRaisesRegex(ValueError, 'unclosed fence'):
            self.validate(lambda text: text.replace('## 5. Scalar Policy', '```markdown\n## 5. Scalar Policy'))

    def test_empty_source(self):
        with self.assertRaisesRegex(ValueError, 'include entrance'):
            self.validate(lambda text: re.sub(r'(<!-- vectoris-core-contract: traits -->\s*```cpp\n).*?(\n```)', r'\1\2', text, flags=re.DOTALL))

    def test_missing_marked_code_fence(self):
        with self.assertRaisesRegex(ValueError, 'source identities'):
            self.validate(lambda text: text.replace('<!-- vectoris-core-contract: primitives -->\n```cpp', '<!-- vectoris-core-contract: primitives -->\n```c++'))

    def test_unmarked_display_fence_is_not_consumer(self):
        # The old audit's broken-block mutation changed only a display example.
        # Required source identities and all four executable consumers are intact.
        self.assertEqual(self.validate(), self.validate(lambda text: text.replace('```cpp', '```c++', 1)))

    def test_compiler_missing(self):
        with patch('verify_core_include_contract.subprocess.run', side_effect=FileNotFoundError('missing compiler')):
            with self.assertRaises(FileNotFoundError):
                compile_contracts(ROOT, 'missing', 'Clang', self.validate())

    def test_compile_nonzero(self):
        with patch('verify_core_include_contract.subprocess.run', return_value=subprocess.CompletedProcess([], 1, '', 'compile failed')):
            with self.assertRaisesRegex(ValueError, 'compile exit 1'):
                compile_contracts(ROOT, 'compiler', 'Clang', self.validate())

    def test_runtime_nonzero(self):
        outcomes = [subprocess.CompletedProcess([], 0, '', ''), subprocess.CompletedProcess([], 7, '', 'runtime failed')]
        with patch('verify_core_include_contract.subprocess.run', side_effect=outcomes):
            with self.assertRaisesRegex(ValueError, 'consumer exit 7'):
                compile_contracts(ROOT, 'compiler', 'Clang', self.validate())

    def test_compiler_timeout(self):
        with patch('verify_core_include_contract.subprocess.run', side_effect=subprocess.TimeoutExpired('compiler', 120)):
            with self.assertRaises(subprocess.TimeoutExpired):
                compile_contracts(ROOT, 'compiler', 'Clang', self.validate())

    def test_runtime_timeout(self):
        outcomes = [subprocess.CompletedProcess([], 0, '', ''), subprocess.TimeoutExpired('consumer', 30)]
        with patch('verify_core_include_contract.subprocess.run', side_effect=outcomes):
            with self.assertRaises(subprocess.TimeoutExpired):
                compile_contracts(ROOT, 'compiler', 'Clang', self.validate())


class IdentityResult(unittest.TextTestResult):
    """Record the identities actually entered and passed, not loader counts."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.executed_ids = []
        self.passed_ids = []

    def startTest(self, test):
        self.executed_ids.append(test.id().rsplit('.', 1)[-1])
        super().startTest(test)

    def addSuccess(self, test):
        self.passed_ids.append(test.id().rsplit('.', 1)[-1])
        super().addSuccess(test)


if __name__ == '__main__':
    expected = {
        'test_afa4_original_two_space_duplicate',
        'test_compile_nonzero',
        'test_compiler_missing',
        'test_compiler_timeout',
        'test_current_contract',
        'test_duplicate_section',
        'test_duplicate_source_identity',
        'test_empty_consumer_body',
        'test_empty_source',
        'test_equivalent_canonical_padding',
        'test_extra_header',
        'test_fenced_table_is_not_public_inventory',
        'test_fenced_example_before_real_table',
        'test_fenced_example_cannot_hide_real_duplicate',
        'test_unclosed_section_fence',
        'test_identity_target_disagreement',
        'test_link_label_variants_duplicate',
        'test_malformed_separator',
        'test_math_appended_false_claim',
        'test_math_export_claim',
        'test_math_include_claim',
        'test_mathfunctions_duplicate_export',
        'test_mathfunctions_missing_export',
        'test_mathfunctions_primitive_role',
        'test_missing_manifest_identity',
        'test_missing_marked_code_fence',
        'test_missing_source_identity',
        'test_missing_table',
        'test_mixed_padding_duplicate',
        'test_multiple_padding_duplicate',
        'test_old_umbrella_claim',
        'test_one_space_duplicate',
        'test_removed_header',
        'test_renamed_header',
        'test_role_formatting_and_supported_wording',
        'test_role_symbols_remain_case_sensitive',
        'test_runtime_nonzero',
        'test_runtime_timeout',
        'test_separated_row_cannot_escape',
        'test_separator_alignment_variants',
        'test_tab_padding_duplicate',
        'test_three_space_duplicate',
        'test_trailing_padding_duplicate',
        'test_unexpected_source_identity',
        'test_unknown_manifest_identity',
        'test_unmarked_display_fence_is_not_consumer',
        'test_unrecognized_visible_row',
        'test_unrelated_table_outside_section',
        'test_wrong_include_entrance',
    }
    actual = unittest.defaultTestLoader.getTestCaseNames(CoreIncludeContractTests)
    if len(actual) != len(expected) or set(actual) != expected:
        sys.exit('ERROR: mandatory Core include self-test identities mismatch')
    result = unittest.main(testRunner=unittest.TextTestRunner(verbosity=2, resultclass=IdentityResult), exit=False).result
    print(f'CORE_CONTRACT_CONTROLS executed={result.testsRun} failed={len(result.failures) + len(result.errors)} skipped={len(result.skipped)}')
    valid = (result.wasSuccessful() and result.testsRun == len(expected) and not result.skipped
             and len(result.executed_ids) == len(expected) and set(result.executed_ids) == expected
             and len(result.passed_ids) == len(expected) and set(result.passed_ids) == expected)
    # The native qualification jobs already upload these log directories. Keep
    # real internal execution evidence there without changing their workflows.
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        directories = [ROOT / name for name in ('artifacts-gcc', 'artifacts-clang', 'artifacts-msvc')
                       if (ROOT / name).is_dir()]
        if len(directories) > 1:
            sys.exit('ERROR: ambiguous native Core control evidence directory')
        if directories:
            evidence = directories[0] / 'core-contract-selftests.jsonl'
            record = {'candidate_sha': os.environ.get('GITHUB_SHA'), 'required': sorted(expected),
                      'executed': result.executed_ids, 'passed': result.passed_ids,
                      'failed': len(result.failures) + len(result.errors),
                      'skipped': len(result.skipped), 'valid': valid}
            with evidence.open('a', encoding='utf-8') as output:
                output.write(json.dumps(record) + '\n')
    sys.exit(0 if valid else 1)
