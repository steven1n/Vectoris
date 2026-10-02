"""AFA003 real CTest registration/execution mutations; no production tree changes."""
import argparse
import sys
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
    @classmethod
    def setUpClass(cls):
        if not BUILD_DIR:
            raise RuntimeError("mandatory real GoogleTest build fixture missing: use --build-dir")
        query = subprocess.run(["ctest", "--test-dir", BUILD_DIR,
                                "-R", "^PublicApiSurfaceTest\\.", "--show-only=json-v1"],
                               capture_output=True, text=True, check=True)
        records = json.loads(query.stdout)["tests"]
        require_equal(REQUIRED, [x["name"] for x in records], "FIXTURE_REGISTERED")
        cls.binary = records[0]["command"][0]
        if not Path(cls.binary).is_file():
            raise RuntimeError("mandatory GoogleTest fixture executable missing")

    def ctest_case(self, mode):
        with tempfile.TemporaryDirectory(prefix='afa003-') as tmp:
            root = Path(tmp)
            names = REQUIRED[:-1] if mode == 'missing' else REQUIRED
            content = ['cmake_minimum_required(VERSION 3.14)', 'project(AFA003 NONE)', 'enable_testing()']
            for name in names:
                selected = 'NoSuchSuite.NoSuchBody' if mode == 'zero_tests' else name
                if mode == 'wrong_body' and name == REQUIRED[0]:
                    selected = REQUIRED[1]
                cmd = f'"{self.binary}" "--gtest_filter={selected}"'
                if mode == 'missing_report':
                    cmd = '"${CMAKE_COMMAND}" -E true'
                if mode == 'failed' and name == REQUIRED[0]:
                    cmd = '"${CMAKE_COMMAND}" -E false'
                content.append(f'add_test(NAME {name} COMMAND {cmd})')
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
    def test_actual_zero_gtest_bodies(self):
        with self.assertRaisesRegex(ValueError, 'GTEST_DEFINED:'):
            self.ctest_case('zero_tests')
    def test_actual_wrong_gtest_body(self):
        with self.assertRaisesRegex(ValueError, 'GTEST_DEFINED:'):
            self.ctest_case('wrong_body')
    def test_empty_command_is_not_body_execution(self):
        with self.assertRaisesRegex(ValueError, 'missing fresh GoogleTest'):
            self.ctest_case('missing_report')
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

BUILD_DIR = None
if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', required=True)
    args, remaining = parser.parse_known_args()
    BUILD_DIR = args.build_dir
    unittest.main(argv=[sys.argv[0]] + remaining, verbosity=2)
