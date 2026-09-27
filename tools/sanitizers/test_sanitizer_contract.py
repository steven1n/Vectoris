"""AFA006 controlled outcomes complement the mandatory native positive/negative probes."""
import subprocess
from pathlib import Path
import unittest
from verify_sanitizers_gate import MARKERS, classify_negative, classify_clean

class AFA006SanitizerContract(unittest.TestCase):
    def result(self,family,code,text,started=True):
        marker=MARKERS[family]+'\n' if started else ''
        return subprocess.CompletedProcess(['controlled-probe'],code,'',marker+text)
    def test_expected_asan(self):
        self.assertEqual(classify_negative(self.result('ASan',-6,'ERROR: AddressSanitizer: heap-use-after-free on address 0x123'),'ASan'),'EXPECTED_SANITIZER_FAILURE')
    def test_expected_ubsan(self):
        self.assertEqual(classify_negative(self.result('UBSan',1,'ubsan_probe.cpp:6:15: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type int'),'UBSan'),'EXPECTED_SANITIZER_FAILURE')
    def test_nondiagnostic_nonzero(self):
        for family in ('ASan','UBSan'):
            self.assertEqual(classify_negative(self.result(family,1,'plain exit(1)'),family),'NO_DIAGNOSTIC')
    def test_loader_failure(self):
        for family in ('ASan','UBSan'):
            self.assertEqual(classify_negative(self.result(family,1,'loader failure, library not loaded',False),family),'INFRASTRUCTURE_FAILURE')
    def test_clean(self):
        self.assertEqual(classify_clean(self.result('clean',0,'')),'CLEAN_SUCCESS')
    def test_diagnostic_exit_zero(self):
        self.assertEqual(classify_negative(self.result('ASan',0,'AddressSanitizer: heap-use-after-free'),'ASan'),'FAIL_FAST_VIOLATION')
        self.assertEqual(classify_negative(self.result('UBSan',0,'runtime error: signed integer overflow'),'UBSan'),'FAIL_FAST_VIOLATION')
    def test_actual_toolchain_fixtures(self):
        for family in ('ASan','UBSan'):
            text=(Path(__file__).parent/'fixtures'/f'{family.lower()}.txt').read_text()
            self.assertEqual(classify_negative(self.result(family,-6,text),family),'EXPECTED_SANITIZER_FAILURE')
    def test_dyld_backtrace_is_not_loader_failure(self):
        text='AddressSanitizer: heap-use-after-free\n    #1 start (dyld:x86_64+0x123)'
        self.assertEqual(classify_negative(self.result('ASan',-6,text),'ASan'),'EXPECTED_SANITIZER_FAILURE')
    def test_wrong_violation(self):
        self.assertEqual(classify_negative(self.result('ASan',1,'AddressSanitizer: stack-buffer-overflow'),'ASan'),'UNEXPECTED_NONZERO')
    def test_probe_never_started(self):
        self.assertEqual(classify_negative(self.result('ASan',1,'AddressSanitizer: heap-use-after-free',False),'ASan'),'INFRASTRUCTURE_FAILURE')
    def test_clean_not_actually_run(self):
        self.assertEqual(classify_clean(self.result('clean',0,'',False)),'INFRASTRUCTURE_FAILURE')
if __name__=='__main__': unittest.main(verbosity=2)
