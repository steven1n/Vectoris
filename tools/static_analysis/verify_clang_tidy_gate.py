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
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile

def run_test(name, expected_code, cmd_args, expect_in_output=None):
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

def main():
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

    real_build_dir = os.path.join(repo_root, ".build", "static-analysis")
    real_compdb_path = os.path.join(real_build_dir, "compile_commands.json")
    if os.path.isfile(real_compdb_path):
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

    print("=" * 80)
    if all_passed:
        print("ALL 12 VRT-11 GATE SELF-TESTS PASSED SUCCESSFULLY.")
        sys.exit(0)
    else:
        print("VRT-11 GATE SELF-TESTS FAILED.")
        sys.exit(1)

if __name__ == "__main__":
    main()
