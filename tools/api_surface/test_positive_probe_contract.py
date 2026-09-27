"""AFA003 real CTest registration/execution mutations; no production tree changes."""
import contextlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from positive_probe_contract import require_equal, verify_ctest, verify_source

ROOT = Path(__file__).resolve().parents[2]
REQUIRED = json.loads((ROOT / 'tools/api_surface/public_api_manifest.json').read_text())['required_positive_tests']
SOURCE = (ROOT / 'modules/VectorisNumerics/tests/PublicApi/PublicApiSurfaceTest.cpp').read_text()

class AFA003IdentityContract(unittest.TestCase):
    def ctest_case(self, mode):
        with tempfile.TemporaryDirectory(prefix='afa003-') as tmp:
            root = Path(tmp)
            names = REQUIRED[:-1] if mode == 'missing' else REQUIRED
            content = ['cmake_minimum_required(VERSION 3.14)', 'project(AFA003 NONE)', 'enable_testing()']
            for name in names:
                outcome = 'false' if mode == 'failed' and name == REQUIRED[0] else 'true'
                content.append(f'add_test(NAME {name} COMMAND "${{CMAKE_COMMAND}}" -E {outcome})')
            if mode == 'unexpected':
                content.append('add_test(NAME PublicApiSurfaceTest.Unexpected COMMAND "${CMAKE_COMMAND}" -E true)')
            if mode == 'not_executed':
                content.append(f'set_tests_properties({REQUIRED[0]} PROPERTIES DISABLED TRUE)')
            if mode == 'duplicate':
                (root/'child').mkdir()
                (root/'child/CMakeLists.txt').write_text(f'add_test(NAME {REQUIRED[0]} COMMAND "${{CMAKE_COMMAND}}" -E true)\n')
                content.append('add_subdirectory(child)')
            (root/'CMakeLists.txt').write_text('\n'.join(content)+'\n')
            configure = subprocess.run(['cmake','-S',tmp,'-B',str(root/'build')],capture_output=True,text=True)
            self.assertEqual(configure.returncode,0,configure.stderr)
            with contextlib.redirect_stdout(io.StringIO()):
                return verify_ctest(root/'build', REQUIRED)

    def test_all_present(self):
        verify_source(REQUIRED,SOURCE)
        sets=self.ctest_case('clean')
        self.assertEqual(sets['PASSED'],sorted(REQUIRED))
    def test_astra_19_source_18_registrations(self):
        self.assertEqual(len(verify_source(REQUIRED,SOURCE)),19)
        with self.assertRaisesRegex(ValueError,'REGISTERED: missing='):
            self.ctest_case('missing')
    def test_duplicate_registration(self):
        with self.assertRaisesRegex(ValueError,'REGISTERED: duplicate'):
            self.ctest_case('duplicate')
    def test_unexpected_registration(self):
        with self.assertRaisesRegex(ValueError,'unexpected=.*Unexpected'):
            self.ctest_case('unexpected')
    def test_registered_not_executed(self):
        with self.assertRaisesRegex(ValueError,'EXECUTED:'):
            self.ctest_case('not_executed')
    def test_executed_failed(self):
        with self.assertRaisesRegex(ValueError,'EXECUTED: CTest failure'):
            self.ctest_case('failed')
    def test_missing_source(self):
        name=REQUIRED[0].split('.')[1]
        with self.assertRaisesRegex(ValueError,'SOURCE_DEFINED:'):
            verify_source(REQUIRED,SOURCE.replace('PublicApiSurfaceTest, '+name,'OtherSuite, '+name))
    def test_missing_manifest_identity(self):
        with self.assertRaisesRegex(ValueError,'SOURCE_DEFINED:'):
            verify_source(REQUIRED[:-1],SOURCE)
    def test_same_count_wrong_identity(self):
        with self.assertRaisesRegex(ValueError,'PASSED:'):
            require_equal(REQUIRED,REQUIRED[:-1]+['PublicApiSurfaceTest.Impostor'],'PASSED')

if __name__=='__main__':
    unittest.main(verbosity=2)
