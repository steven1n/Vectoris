#!/usr/bin/env python3
"""VRT-05 / AFA006: a real fail-fast diagnostic, not any nonzero exit, is required."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

MARKERS = {'ASan':'VECTORIS_ASAN_PROBE_BEGIN', 'UBSan':'VECTORIS_UBSAN_PROBE_BEGIN', 'clean':'VECTORIS_CLEAN_PROBE_BEGIN'}
PATTERNS = {'ASan':r'AddressSanitizer:\s*heap-use-after-free',
            'UBSan':r'runtime error:\s*signed integer overflow'}

def classify_negative(result, family):
    text = result.stdout + '\n' + result.stderr
    if re.search(r'(?i)(library not loaded|cannot open shared object file|loader failure|command not found)',text):
        return 'INFRASTRUCTURE_FAILURE'
    diagnostic = re.search(PATTERNS[family], text) is not None
    if result.returncode == 0:
        return 'FAIL_FAST_VIOLATION' if diagnostic else 'NO_DIAGNOSTIC'
    if MARKERS[family] not in text:
        return 'INFRASTRUCTURE_FAILURE'
    if diagnostic:
        return 'EXPECTED_SANITIZER_FAILURE'
    if re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:',text):
        return 'UNEXPECTED_NONZERO'
    return 'NO_DIAGNOSTIC'

def classify_clean(result):
    text = result.stdout + '\n' + result.stderr
    if result.returncode == 0 and MARKERS['clean'] in text and not re.search(
            r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:',text):
        return 'CLEAN_SUCCESS'
    return 'INFRASTRUCTURE_FAILURE' if MARKERS['clean'] not in text else 'UNEXPECTED_NONZERO'

def run_probe(cxx, family):
    bodies = {
        'ASan': 'volatile int* p = new int[10]; delete[] p; int value = p[0]; (void)value; return 0;',
        'UBSan': 'volatile int a = INT_MAX; int b = a + 1; (void)b; return 0;',
        'clean': 'int sum = 0; for (int i=0; i<100; ++i) { sum += i; } return sum == 4950 ? 0 : 1;',
    }
    flag = {'ASan':'address','UBSan':'undefined','clean':'address,undefined'}[family]
    with tempfile.TemporaryDirectory(prefix='vectoris-sanitizer-') as tmp:
        source=Path(tmp)/f'{family.lower()}_probe.cpp'; exe=Path(tmp)/f'{family.lower()}_probe'
        source.write_text('#include <climits>\n#include <cstdio>\nint main() {\n'
                          f'  std::fputs("{MARKERS[family]}\\n", stderr);\n  '+bodies[family]+'\n}\n')
        command=[cxx,'-std=c++20','-O0',f'-fsanitize={flag}','-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer',str(source),'-o',str(exe)]
        try:
            compile_result=subprocess.run(command,capture_output=True,text=True)
            if compile_result.returncode:
                print('INFRASTRUCTURE_FAILURE: probe compilation failed\n'+compile_result.stderr)
                return False
            base={k:v for k,v in os.environ.items() if not k.startswith(('ASAN_','UBSAN_'))}
            explicit={**base,'ASAN_OPTIONS':'halt_on_error=1:abort_on_error=1',
                      'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
            environments=[('default',base),('fail-fast',explicit)] if family!='clean' else [('fail-fast',explicit)]
            outcomes=[]
            for label,env in environments:
                run=subprocess.run([str(exe)],capture_output=True,text=True,env=env)
                category=classify_clean(run) if family=='clean' else classify_negative(run,family)
                print(f'{family} {label}: exit={run.returncode}; classification={category}')
                print(run.stdout+run.stderr)
                outcomes.append(category==('CLEAN_SUCCESS' if family=='clean' else 'EXPECTED_SANITIZER_FAILURE'))
            return all(outcomes)
        except OSError as exc:
            print(f'INFRASTRUCTURE_FAILURE: {exc}')
            return False

def test_asan_gate(cxx): return run_probe(cxx,'ASan')
def test_ubsan_gate(cxx): return run_probe(cxx,'UBSan')
def test_clean_execution(cxx): return run_probe(cxx,'clean')

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--cxx',default=os.environ.get('CXX') or shutil.which('clang++'))
    args=parser.parse_args()
    if not args.cxx: parser.exit(1,'INFRASTRUCTURE_FAILURE: no compiler\n')
    tests=subprocess.run([sys.executable,str(Path(__file__).with_name('test_sanitizer_contract.py'))])
    outcomes=[test_ubsan_gate(args.cxx),test_asan_gate(args.cxx),test_clean_execution(args.cxx)]
    passed=tests.returncode==0 and all(outcomes)
    print('SANITIZER GATE VERIFICATION: '+('PASS' if passed else 'FAIL'))
    return 0 if passed else 1
if __name__=='__main__': sys.exit(main())
