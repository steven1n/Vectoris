#!/usr/bin/env python3
"""
Vectoris Clang-Tidy Harness Verification Gate (VRT-11)
Deterministic self-test suite proving that run_clang_tidy.py is strictly fail-closed:
1. Rejects missing compile_commands.json
2. Rejects 0-byte compile_commands.json
3. Rejects empty JSON list [] compile_commands.json
4. Rejects compile database missing required project TU categories
5. Rejects malformed compile command entries (missing 'file' or compile command)
6. Rejects missing .clang-tidy configuration file
7. Successfully analyzes a minimal valid controlled translation unit
8-12. Preserves exact TU set and category fail-closed checks
13-20. Parses and safely classifies complete, unknown, incomplete, repeated, and malformed diagnostics
"""

import json
import argparse
import re
import os
import shutil
import subprocess
import sys
import tempfile
from run_clang_tidy import (
    classify_diagnostic,
    diagnostic_key,
    diagnostic_check_is_known,
    format_diagnostic,
    parse_enabled_checks,
    parse_clang_tidy_diagnostics,
)

def _run_test(name, expected_code, cmd_args, expect_in_output=None):
    print(f"[TEST] {name} ... ", end="", flush=True)
    res = subprocess.run(cmd_args, capture_output=True, text=True)
    combined = res.stdout + "\n" + res.stderr

    if res.returncode != expected_code:
        print(f"FAILED (expected exit code {expected_code}, got {res.returncode})")
        print("=== STDOUT ===")
        print(res.stdout)
        print("=== STDERR ===")
        print(res.stderr)
        return False

    if expect_in_output and expect_in_output not in combined:
        print(f"FAILED (expected substring '{expect_in_output}' not found in output)")
        print("=== COMBINED OUTPUT ===")
        print(combined)
        return False

    print("PASS")
    return True

def _run_inline_test(name, test):
    print(f"[TEST] {name} ... ", end="", flush=True)
    try:
        passed = bool(test())
    except Exception as exc:
        print(f"FAILED ({type(exc).__name__}: {exc})")
        return False
    print("PASS" if passed else "FAILED")
    return passed

RESULTS = []

def run_test(name, **kwargs):
    try:
        passed = _run_test(name, **kwargs)
    except Exception as exc:
        print(f"FAILED ({type(exc).__name__}: {exc})")
        passed = False
    RESULTS.append((name, passed))
    return passed

def run_inline_test(name, test):
    passed = _run_inline_test(name, test)
    RESULTS.append((name, passed))
    return passed

def main():
    parser = argparse.ArgumentParser(description="AFA005 mandatory exact-TU self-tests")
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--clang-tidy", required=True)
    args = parser.parse_args()
    real_build_dir = os.path.abspath(args.build_dir)
    real_compdb_path = os.path.join(real_build_dir, "compile_commands.json")
    if not os.path.isfile(real_compdb_path):
        parser.exit(1, "ERROR: mandatory compile_commands.json fixture missing; executed=0 passed=0 failed=0 skipped=0 preflight_failures=1\n")
    binary = shutil.which(args.clang_tidy)
    if binary is None:
        parser.exit(1, "ERROR: mandatory clang-tidy binary missing; executed=0 passed=0 failed=0 skipped=0 preflight_failures=1\n")
    try:
        version = subprocess.check_output([binary, "--version"], text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"ERROR: mandatory clang-tidy cannot execute: {exc}\n")
    if not re.search(r"\bversion 22\.", version):
        parser.exit(1, "ERROR: mandatory clang-tidy major 22 required\n")
    os.environ["VECTORIS_CLANG_TIDY"] = binary
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    harness_script = os.path.join(repo_root, "tools", "static_analysis", "run_clang_tidy.py")
    real_config = os.path.join(repo_root, ".clang-tidy")

    if not os.path.isfile(harness_script):
        print(f"ERROR: Harness script not found at {harness_script}", file=sys.stderr)
        sys.exit(1)
    if not os.path.isfile(real_config):
        print(f"ERROR: Repository .clang-tidy not found at {real_config}", file=sys.stderr)
        sys.exit(1)

    print("=" * 80)
    print("Vectoris Clang-Tidy Harness Fail-Closed Self-Test Suite (VRT-11)")
    print("=" * 80)

    all_passed = True

    # 1. Missing compile database
    with tempfile.TemporaryDirectory() as tmpdir:
        passed = run_test(
            "1. Missing compile_commands.json",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="Compilation database not found"
        )
        all_passed = all_passed and passed

    diagnostic_prefix = "/vectoris/modules/VectorisNumerics"
    production_sample = (
        f"{diagnostic_prefix}/include/Vectoris/Numerics/Core/Math.h:70:45: "
        "warning: narrowing conversion [bugprone-narrowing-conversions]"
    )
    diagnostic_cases = [
        ("13. Standard diagnostic preserves location and check", lambda: (
            len(parse_clang_tidy_diagnostics(production_sample)) == 1
            and parse_clang_tidy_diagnostics(production_sample)[0]["line"] == 70
            and parse_clang_tidy_diagnostics(production_sample)[0]["col"] == 45
            and parse_clang_tidy_diagnostics(production_sample)[0]["check"] == "bugprone-narrowing-conversions"
        )),
        ("14. Unknown check name is retained and rejected by the check contract", lambda: (
            parse_clang_tidy_diagnostics("/tmp/a.cpp:1:2: warning: newer check [future-check]")[0]["check"] == "future-check"
            and not diagnostic_check_is_known(
                parse_clang_tidy_diagnostics("/tmp/a.cpp:1:2: warning: newer check [future-check]")[0],
                parse_enabled_checks("Enabled checks:\n  known-check\n")
            )
        )),
        ("15. Missing fields are recorded and safely formatted", lambda: (
            parse_clang_tidy_diagnostics("warning: diagnostic without location or check")[0]["file"] is None
            and parse_clang_tidy_diagnostics("warning: diagnostic without location or check")[0]["incomplete_location"]
            and not diagnostic_check_is_known(
                parse_clang_tidy_diagnostics("warning: diagnostic without location or check")[0], set()
            )
            and "<unknown-file>:?:?" in format_diagnostic(
                parse_clang_tidy_diagnostics("warning: diagnostic without location or check")[0], "/vectoris"
            )
        )),
        ("16. Duplicate diagnostics deduplicate without losing parse rows", lambda: (
            len(parse_clang_tidy_diagnostics(production_sample + "\n" + production_sample)) == 2
            and len({diagnostic_key(d) for d in parse_clang_tidy_diagnostics(production_sample + "\n" + production_sample)}) == 1
        )),
        ("17. System diagnostic classification", lambda: (
            classify_diagnostic(parse_clang_tidy_diagnostics("/usr/include/vector:8:3: error: bad [tool-check]")[0], "/vectoris")
            == "system_external"
        )),
        ("18. Production diagnostic classification", lambda: (
            classify_diagnostic(parse_clang_tidy_diagnostics(production_sample)[0], "/vectoris") == "production"
        )),
        ("19. Test diagnostic classification", lambda: (
            classify_diagnostic(parse_clang_tidy_diagnostics(
                f"{diagnostic_prefix}/tests/Core/MathTest.cpp:5:2: warning: test [test-check]"
            )[0], "/vectoris") == "test"
        )),
        ("20. Abnormal line/column fields do not crash and are retained", lambda: (
            parse_clang_tidy_diagnostics("/vectoris/modules/VectorisNumerics/include/Vectoris/Numerics/Core/Math.h:no-line:no-column: error: odd [future-check]")[0]["invalid_position"]
            and parse_clang_tidy_diagnostics("/vectoris/modules/VectorisNumerics/include/Vectoris/Core/Math.h:no-line:no-column: error: odd [future-check]")[0]["line_raw"] == "no-line"
        )),
    ]
    for name, test in diagnostic_cases:
        passed = run_inline_test(name, test)
        all_passed = all_passed and passed

    # 2. 0-byte compile database
    with tempfile.TemporaryDirectory() as tmpdir:
        compdb = os.path.join(tmpdir, "compile_commands.json")
        with open(compdb, "w") as f:
            pass  # 0 bytes
        passed = run_test(
            "2. 0-byte compile_commands.json",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="is empty (0 bytes)"
        )
        all_passed = all_passed and passed

    # 3. Empty JSON list []
    with tempfile.TemporaryDirectory() as tmpdir:
        compdb = os.path.join(tmpdir, "compile_commands.json")
        with open(compdb, "w") as f:
            json.dump([], f)
        passed = run_test(
            "3. Empty JSON list [] compile_commands.json",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="contains 0 entries"
        )
        all_passed = all_passed and passed

    # 4. Missing required project TU categories
    with tempfile.TemporaryDirectory() as tmpdir:
        compdb = os.path.join(tmpdir, "compile_commands.json")
        dummy_cpp = os.path.join(tmpdir, "dummy.cpp")
        with open(dummy_cpp, "w") as f:
            f.write("int dummy() { return 0; }\n")
        with open(compdb, "w") as f:
            json.dump([{
                "directory": tmpdir,
                "file": dummy_cpp,
                "command": f"clang++ -c {dummy_cpp}"
            }], f)
        passed = run_test(
            "4. Missing required project TU categories",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="Required project TU categories absent"
        )
        all_passed = all_passed and passed

    # 5. Malformed compile command entry (missing file)
    with tempfile.TemporaryDirectory() as tmpdir:
        compdb = os.path.join(tmpdir, "compile_commands.json")
        with open(compdb, "w") as f:
            json.dump([{"directory": tmpdir, "command": "clang++ -c dummy.cpp"}], f)
        passed = run_test(
            "5. Malformed compile command entry (missing 'file')",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="missing valid file string"
        )
        all_passed = all_passed and passed

    # 6. Missing .clang-tidy configuration file
    with tempfile.TemporaryDirectory() as tmpdir:
        fake_config = os.path.join(tmpdir, "nonexistent.clang-tidy")
        passed = run_test(
            "6. Missing .clang-tidy configuration file",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", fake_config],
            expect_in_output="Authoritative Clang-Tidy configuration file not found"
        )
        all_passed = all_passed and passed

    # 7. Minimal valid controlled translation unit
    with tempfile.TemporaryDirectory() as tmpdir:
        compdb = os.path.join(tmpdir, "compile_commands.json")
        test_cpp = os.path.join(tmpdir, "test.cpp")
        with open(test_cpp, "w") as f:
            f.write("int main() { return 0; }\n")
        with open(compdb, "w") as f:
            json.dump([{
                "directory": tmpdir,
                "file": test_cpp,
                "command": f"clang++ -std=c++20 -c {test_cpp}"
            }], f)
        passed = run_test(
            "7. Minimal valid controlled translation unit",
            expected_code=0,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config, "--skip-category-check", "--skip-expected-set-check"],
            expect_in_output="RESULT: PASS"
        )
        all_passed = all_passed and passed

    with open(real_compdb_path, "r", encoding="utf-8") as f:
        raw_compdb_text = f.read()

    # 8. Compile DB missing one expected project TU
    with tempfile.TemporaryDirectory() as tmpdir:
        adapted_text = raw_compdb_text.replace(real_build_dir, tmpdir)
        compdb = json.loads(adapted_text)
        modified = [e for e in compdb if "PublicApiSurfaceTest.cpp" not in e.get("file", "")]
        with open(os.path.join(tmpdir, "compile_commands.json"), "w") as f:
            json.dump(modified, f)
        passed = run_test(
            "8. Compile DB missing one expected project TU",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="missing 1 expected project TUs"
        )
        all_passed = all_passed and passed

    # 9. Compile DB containing unexpected project TU
    with tempfile.TemporaryDirectory() as tmpdir:
        adapted_text = raw_compdb_text.replace(real_build_dir, tmpdir)
        compdb = json.loads(adapted_text)
        compdb.append({
            "directory": tmpdir,
            "file": os.path.join(repo_root, "unexpected_tu.cpp"),
            "command": "clang++ -c unexpected_tu.cpp"
        })
        with open(os.path.join(tmpdir, "compile_commands.json"), "w") as f:
            json.dump(compdb, f)
        passed = run_test(
            "9. Compile DB containing unexpected project TU",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="unexpected first-party TUs"
        )
        all_passed = all_passed and passed

    # 10. Duplicate project TU in compile DB
    with tempfile.TemporaryDirectory() as tmpdir:
        adapted_text = raw_compdb_text.replace(real_build_dir, tmpdir)
        compdb = json.loads(adapted_text)
        compdb.append(compdb[0])
        with open(os.path.join(tmpdir, "compile_commands.json"), "w") as f:
            json.dump(compdb, f)
        passed = run_test(
            "10. Duplicate project TU in compile DB",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config],
            expect_in_output="Duplicate TU entries found"
        )
        all_passed = all_passed and passed

    # 11. Expected-set derivation returns empty
    with tempfile.TemporaryDirectory() as tmpdir:
        fake_repo = os.path.join(tmpdir, "repo")
        os.makedirs(os.path.join(fake_repo, "modules", "VectorisNumerics"), exist_ok=True)
        with open(os.path.join(fake_repo, "modules", "VectorisNumerics", "CMakeLists.txt"), "w") as f:
            f.write("# Empty CMake\n")
        fake_build = os.path.join(tmpdir, "build")
        os.makedirs(fake_build, exist_ok=True)
        with open(os.path.join(fake_build, "compile_commands.json"), "w") as f:
            json.dump([{"directory": fake_build, "file": os.path.join(fake_repo, "dummy.cpp"), "command": "clang++ -c dummy.cpp"}], f)
        passed = run_test(
            "11. Expected-set derivation returns empty",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--repo-root", fake_repo, "--build-dir", fake_build, "--config-file", real_config, "--skip-category-check"],
            expect_in_output="Expected-set derivation returned 0 translation units"
        )
        all_passed = all_passed and passed

    # 12. Eligible-set derivation returns empty
    with tempfile.TemporaryDirectory() as tmpdir:
        gtest_entry = {"directory": tmpdir, "file": "/path/to/_deps/googletest-src/gtest.cc", "command": "clang++ -c gtest.cc"}
        with open(os.path.join(tmpdir, "compile_commands.json"), "w") as f:
            json.dump([gtest_entry], f)
        passed = run_test(
            "12. Eligible-set derivation returns empty",
            expected_code=1,
            cmd_args=[sys.executable, harness_script, "--build-dir", tmpdir, "--config-file", real_config, "--skip-category-check", "--skip-expected-set-check"],
            expect_in_output="Zero eligible translation units found"
        )
        all_passed = all_passed and passed

    # AFA005 mandatory controls use actual runner/tool execution, not banner matching alone.
    passed = run_test("21. Wrong tool version rejected", expected_code=1,
        cmd_args=[sys.executable, harness_script, "--build-dir", real_build_dir,
                  "--clang-tidy", binary, "--required-clang-tidy-major", "999"],
        expect_in_output="does not match required major")
    all_passed = all_passed and passed
    passed = run_test("22. Tool invocation failure rejected", expected_code=1,
        cmd_args=[sys.executable, harness_script, "--build-dir", real_build_dir,
                  "--clang-tidy", sys.executable],
        expect_in_output="Failed to query configured clang-tidy checks")
    all_passed = all_passed and passed
    with tempfile.TemporaryDirectory() as tmpdir:
        source = os.path.join(tmpdir, "modules", "VectorisNumerics", "tests", "AFA005Diagnostic.cpp")
        os.makedirs(os.path.dirname(source))
        with open(source, "w") as f:
            f.write("int narrow(double value) { return value; }\n")
        with open(os.path.join(tmpdir, "compile_commands.json"), "w") as f:
            json.dump([{"directory": tmpdir, "file": source,
                        "arguments": ["clang++", "-std=c++20", "-c", source]}], f)
        passed = run_test("23. Real generated diagnostic rejected", expected_code=1,
            cmd_args=[sys.executable, harness_script, "--repo-root", tmpdir, "--build-dir", tmpdir,
                      "--config-file", real_config, "--clang-tidy", binary, "--warnings-as-errors",
                      "--skip-category-check", "--skip-expected-set-check"],
            expect_in_output="bugprone-narrowing-conversions")
        all_passed = all_passed and passed
    passed = run_test("24. Actual fresh exact TU set analyzed cleanly", expected_code=0,
        cmd_args=[sys.executable, harness_script, "--build-dir", real_build_dir,
                  "--clang-tidy", binary, "--required-clang-tidy-major", "22", "--warnings-as-errors"],
        expect_in_output="RESULT: PASS")
    all_passed = all_passed and passed

    print("=" * 80)
    passed_count = sum(passed for _, passed in RESULTS)
    failed_count = len(RESULTS) - passed_count
    print(f"AFA005 executed={len(RESULTS)} passed={passed_count} failed={failed_count} skipped=0")
    if all_passed and failed_count == 0:
        print("MANDATORY VRT-11 GATE SELF-TESTS PASSED.")
        sys.exit(0)
    else:
        print("VRT-11 GATE SELF-TESTS FAILED.")
        sys.exit(1)

if __name__ == "__main__":
    main()
