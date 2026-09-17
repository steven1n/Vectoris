#!/usr/bin/env python3
"""
AegisMathLib Stable-Core Coverage Runner (P2-COV)

Orchestrates:
1. Clean profiles directory
2. Execute instrumented test binary with LLVM_PROFILE_FILE
3. Merge raw profiles with llvm-profdata
4. Export production summary with llvm-cov
5. Enforce coverage threshold gate via verify_coverage.py
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys

def parse_args():
    parser = argparse.ArgumentParser(description="Run AegisMathLib coverage pipeline.")
    parser.add_argument("--test-binary", required=True, help="Path to AegisMathLib_Tests binary")
    parser.add_argument("--llvm-profdata", required=True, help="Path to llvm-profdata executable")
    parser.add_argument("--llvm-cov", required=True, help="Path to llvm-cov executable")
    parser.add_argument("--build-dir", required=True, help="CMake build directory")
    parser.add_argument("--repo-root", default=".", help="Repository root directory")
    parser.add_argument("--html-dir", default=None, help="Optional output directory for HTML report")
    return parser.parse_args()

def main():
    args = parse_args()
    build_dir = os.path.abspath(args.build_dir)
    repo_root = os.path.abspath(args.repo_root)
    test_binary = os.path.abspath(args.test_binary)
    profdata_tool = args.llvm_profdata
    cov_tool = args.llvm_cov

    if not os.path.exists(test_binary):
        print(f"ERROR: Test binary not found: {test_binary}", file=sys.stderr)
        sys.exit(1)

    profiles_dir = os.path.join(build_dir, "profiles")
    profdata_file = os.path.join(build_dir, "aegismath.profdata")
    summary_json = os.path.join(build_dir, "coverage-summary.json")

    # Inspect and display qualified toolchain pairing
    profdata_ver_res = subprocess.run([profdata_tool, "--version"], capture_output=True, text=True)
    profdata_ver = profdata_ver_res.stdout.strip().splitlines()[0] if profdata_ver_res.stdout else "unknown"
    cov_ver_res = subprocess.run([cov_tool, "--version"], capture_output=True, text=True)
    cov_ver = cov_ver_res.stdout.strip().splitlines()[0] if cov_ver_res.stdout else "unknown"

    print("=" * 80)
    print("AegisMathLib LLVM Coverage Toolchain Qualification")
    print("=" * 80)
    print(f"llvm-profdata: {profdata_tool}")
    print(f"  Version:     {profdata_ver}")
    print(f"llvm-cov:      {cov_tool}")
    print(f"  Version:     {cov_ver}")
    print(f"Test Binary:   {test_binary}")
    print("Toolchain pairing: Qualified Apple developer / LLVM companion tools")
    print("=" * 80)

    # Phase 13: Clean raw profile directory
    print(f"[Coverage] Cleaning profile directory: {profiles_dir}")
    if os.path.exists(profiles_dir):
        shutil.rmtree(profiles_dir)
    os.makedirs(profiles_dir, exist_ok=True)

    if os.path.exists(profdata_file):
        os.remove(profdata_file)
    if os.path.exists(summary_json):
        os.remove(summary_json)

    # Phase 14: Execute test binary
    profraw_pattern = os.path.join(profiles_dir, "aegis-%p.profraw")
    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = profraw_pattern

    print(f"[Coverage] Executing test suite: {test_binary}")
    ret = subprocess.run([test_binary], env=env)
    if ret.returncode != 0:
        print(f"ERROR: Test binary failed with exit code {ret.returncode}", file=sys.stderr)
        sys.exit(ret.returncode)

    # Collect .profraw files
    profraw_files = glob.glob(os.path.join(profiles_dir, "*.profraw"))
    if not profraw_files:
        print(f"ERROR: No .profraw files found in {profiles_dir}", file=sys.stderr)
        sys.exit(1)
    print(f"[Coverage] Collected {len(profraw_files)} profile file(s).")

    # Phase 16: Merge profiles
    print(f"[Coverage] Merging profiles into {profdata_file}")
    merge_cmd = [profdata_tool, "merge", "-sparse"] + profraw_files + ["-o", profdata_file]
    ret = subprocess.run(merge_cmd)
    if ret.returncode != 0:
        print(f"ERROR: llvm-profdata merge failed with exit code {ret.returncode}", file=sys.stderr)
        sys.exit(ret.returncode)

    if not os.path.exists(profdata_file) or os.path.getsize(profdata_file) == 0:
        print(f"ERROR: Merged profile {profdata_file} is missing or empty.", file=sys.stderr)
        sys.exit(1)

    # Phase 17 & 18: Export JSON summary
    print(f"[Coverage] Exporting production coverage summary to {summary_json}")
    ignore_regex = r"(^|/)(tests|cmake-build-|\.build|_deps|googletest|googlemock|AegisDynamics)(/|$)"
    export_cmd = [
        cov_tool, "export",
        test_binary,
        f"-instr-profile={profdata_file}",
        "-summary-only",
        "-format=text",
        f"-ignore-filename-regex={ignore_regex}"
    ]
    with open(summary_json, "w", encoding="utf-8") as f_out:
        ret = subprocess.run(export_cmd, stdout=f_out)
        if ret.returncode != 0:
            print(f"ERROR: llvm-cov export failed with exit code {ret.returncode}", file=sys.stderr)
            sys.exit(ret.returncode)

    # Phase 24: HTML report if requested
    if args.html_dir:
        html_path = os.path.abspath(args.html_dir)
        print(f"[Coverage] Generating HTML coverage report in {html_path}")
        html_cmd = [
            cov_tool, "show",
            test_binary,
            f"-instr-profile={profdata_file}",
            "-format=html",
            f"-output-dir={html_path}",
            "-show-line-counts-or-regions",
            "-show-branches=count",
            "-show-instantiations",
            f"-ignore-filename-regex={ignore_regex}"
        ]
        ret = subprocess.run(html_cmd)
        if ret.returncode != 0:
            print(f"WARNING: HTML generation returned {ret.returncode}", file=sys.stderr)

    # Phase 19, 20, 22: Enforce threshold gate
    verify_script = os.path.join(repo_root, "tools", "coverage", "verify_coverage.py")
    print(f"[Coverage] Enforcing coverage threshold gate: {verify_script}")
    ret = subprocess.run([sys.executable, verify_script, summary_json, "--repo-root", repo_root])
    sys.exit(ret.returncode)

if __name__ == "__main__":
    main()
