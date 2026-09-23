#!/usr/bin/env python3
"""
verify_sanitizers_gate.py

Dedicated negative sanitizer gate verification script for Vectoris.
Validates that runtime violations under AddressSanitizer and UndefinedBehaviorSanitizer
reliably trigger fail-fast behavior with non-zero exit codes.

Complies with VRT-05 requirements:
- Isolated self-test (does not contaminate production sources or test targets).
- Proves runtime violation -> non-zero exit.
- Records raw exit codes and diagnostic output.
"""

import os
import sys
import subprocess
import tempfile

def find_cxx_compiler():
    candidates = ["clang++", "c++", "g++"]
    for c in candidates:
        res = subprocess.run(["which", c], capture_output=True, text=True)
        if res.returncode == 0:
            return res.stdout.strip()
    return "clang++"

def test_ubsan_gate(cxx):
    print("=" * 60)
    print("Testing UndefinedBehaviorSanitizer (UBSan) Fail-Fast Gate")
    print("=" * 60)

    source = """#include <climits>
#include <iostream>

int main() {
    volatile int a = INT_MAX;
    int b = a + 1;
    (void)b;
    return 0;
}
"""
    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = os.path.join(tmpdir, "ubsan_probe.cpp")
        exe_path = os.path.join(tmpdir, "ubsan_probe")
        with open(src_path, "w") as f:
            f.write(source)

        compile_cmd = [
            cxx, "-std=c++20",
            "-fsanitize=undefined",
            "-fno-sanitize-recover=undefined",
            "-fno-omit-frame-pointer",
            src_path, "-o", exe_path
        ]

        comp_res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if comp_res.returncode != 0:
            print(f"FAILED: Compilation of UBSan probe failed:\n{comp_res.stderr}")
            return False

        # Test 1: Run with default environment (no explicit UBSAN_OPTIONS)
        clean_env = {k: v for k, v in os.environ.items() if not k.startswith("UBSAN_")}
        run_res1 = subprocess.run([exe_path], capture_output=True, text=True, env=clean_env)

        diag_emitted1 = ("runtime error" in run_res1.stderr) or ("UndefinedBehaviorSanitizer" in run_res1.stderr)
        non_zero_exit1 = (run_res1.returncode != 0)

        print(f"  [Default Env] Diagnostic Emitted: {"YES" if diag_emitted1 else "NO"}")
        print(f"  [Default Env] Raw Exit Code:     {run_res1.returncode}")
        print(f"  [Default Env] Fail-Fast Result:  {"PASS" if non_zero_exit1 else "FAIL"}")

        # Test 2: Run with explicit UBSAN_OPTIONS=halt_on_error=1
        halt_env = dict(clean_env)
        halt_env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        run_res2 = subprocess.run([exe_path], capture_output=True, text=True, env=halt_env)

        diag_emitted2 = ("runtime error" in run_res2.stderr) or ("UndefinedBehaviorSanitizer" in run_res2.stderr)
        non_zero_exit2 = (run_res2.returncode != 0)

        print(f"  [halt_on_error] Diagnostic Emitted: {"YES" if diag_emitted2 else "NO"}")
        print(f"  [halt_on_error] Raw Exit Code:     {run_res2.returncode}")
        print(f"  [halt_on_error] Fail-Fast Result:  {"PASS" if non_zero_exit2 else "FAIL"}")

        if not (non_zero_exit1 and non_zero_exit2):
            print("ERROR: UBSan violation failed to cause a non-zero exit code!")
            return False

    print("UBSan Fail-Fast Gate: PASS\n")
    return True

def test_asan_gate(cxx):
    print("=" * 60)
    print("Testing AddressSanitizer (ASan) Fail-Fast Gate")
    print("=" * 60)

    source = """#include <iostream>

int main() {
    volatile int* ptr = new int[10];
    delete[] ptr;
    int val = ptr[0];
    (void)val;
    return 0;
}
"""
    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = os.path.join(tmpdir, "asan_probe.cpp")
        exe_path = os.path.join(tmpdir, "asan_probe")
        with open(src_path, "w") as f:
            f.write(source)

        compile_cmd = [
            cxx, "-std=c++20",
            "-fsanitize=address",
            "-fno-omit-frame-pointer",
            src_path, "-o", exe_path
        ]

        comp_res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if comp_res.returncode != 0:
            print(f"FAILED: Compilation of ASan probe failed:\n{comp_res.stderr}")
            return False

        clean_env = {k: v for k, v in os.environ.items() if not k.startswith("ASAN_")}
        run_res = subprocess.run([exe_path], capture_output=True, text=True, env=clean_env)

        diag_emitted = "AddressSanitizer" in run_res.stderr
        non_zero_exit = (run_res.returncode != 0)

        print(f"  Diagnostic Emitted: {"YES" if diag_emitted else "NO"}")
        print(f"  Raw Exit Code:     {run_res.returncode}")
        print(f"  Fail-Fast Result:  {"PASS" if non_zero_exit else "FAIL"}")

        if not non_zero_exit:
            print("ERROR: ASan violation failed to cause a non-zero exit code!")
            return False

    print("ASan Fail-Fast Gate: PASS\n")
    return True

def test_clean_execution(cxx):
    print("=" * 60)
    print("Testing Clean Program Execution Under Combined Sanitizers")
    print("=" * 60)

    source = """#include <iostream>

int main() {
    int sum = 0;
    for (int i = 0; i < 100; ++i) {
        sum += i;
    }
    return (sum == 4950) ? 0 : 1;
}
"""
    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = os.path.join(tmpdir, "clean_probe.cpp")
        exe_path = os.path.join(tmpdir, "clean_probe")
        with open(src_path, "w") as f:
            f.write(source)

        compile_cmd = [
            cxx, "-std=c++20",
            "-fsanitize=address,undefined",
            "-fno-sanitize-recover=undefined",
            "-fno-omit-frame-pointer",
            src_path, "-o", exe_path
        ]

        comp_res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if comp_res.returncode != 0:
            print(f"FAILED: Compilation of clean probe failed:\n{comp_res.stderr}")
            return False

        run_res = subprocess.run([exe_path], capture_output=True, text=True)
        print(f"  Raw Exit Code:     {run_res.returncode}")
        print(f"  Stderr empty:      {"YES" if len(run_res.stderr.strip()) == 0 else "NO"}")

        if run_res.returncode != 0:
            print("ERROR: Clean program unexpectedly returned non-zero exit code!")
            return False

    print("Clean Execution Gate: PASS\n")
    return True

def main():
    cxx = find_cxx_compiler()
    print(f"Using C++ Compiler: {cxx}")

    ubsan_ok = test_ubsan_gate(cxx)
    asan_ok = test_asan_gate(cxx)
    clean_ok = test_clean_execution(cxx)

    if ubsan_ok and asan_ok and clean_ok:
        print("=" * 60)
        print("ALL SANITIZER GATES VERIFIED: PASS")
        print("=" * 60)
        sys.exit(0)
    else:
        print("=" * 60)
        print("SANITIZER GATE VERIFICATION: FAIL")
        print("=" * 60)
        sys.exit(1)

if __name__ == "__main__":
    main()
