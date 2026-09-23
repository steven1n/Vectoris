#!/usr/bin/env python3
"""
Vectoris Clang-Tidy Qualification Harness (P2-STA / VRT-11)
Executes clang-tidy across compile_commands.json, verifies exact first-party TU set,
isolates production diagnostics, and enforces 0 unsuppressed warnings under explicit config.
"""

import argparse
import glob
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

def find_clang_tidy(explicit_path=None):
    if explicit_path and os.path.isfile(explicit_path):
        return explicit_path
    
    candidates = [
        os.environ.get("VECTORIS_CLANG_TIDY"),
        os.environ.get("AEGISMATH_CLANG_TIDY"),
        "/Applications/CLion.app/Contents/bin/clang/mac/x64/bin/clang-tidy",
        "/usr/local/opt/llvm/bin/clang-tidy",
        "/opt/homebrew/opt/llvm/bin/clang-tidy",
    ]
    for c in candidates:
        if c and os.path.isfile(c):
            return c
    return None

def get_compiler_resource_include():
    try:
        out = subprocess.check_output(["clang++", "-print-resource-dir"], stderr=subprocess.DEVNULL).decode().strip()
        inc = os.path.join(out, "include")
        if os.path.isdir(inc):
            return inc
    except Exception:
        pass
    clang_incs = glob.glob("/Library/Developer/CommandLineTools/usr/lib/clang/*/include")
    return clang_incs[0] if clang_incs else None

def get_macos_sdk_args():
    if platform.system() != "Darwin":
        return []
        
    args = []
    try:
        sdk_path = subprocess.check_output(["xcrun", "--show-sdk-path"], stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        sdk_path = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
        
    arch = platform.machine()
    target = f"{arch}-apple-darwin"
    args.append(f"--extra-arg=--target={target}")
    args.append("--extra-arg=-nostdinc++")
    args.append(f"--extra-arg=-isystem{sdk_path}/usr/include/c++/v1")
    
    resource_inc = get_compiler_resource_include()
    if resource_inc:
        args.append(f"--extra-arg=-isystem{resource_inc}")
        
    args.append("--extra-arg=-isysroot")
    args.append(f"--extra-arg={sdk_path}")
    return args

def run_tu(clang_tidy_bin, build_dir, source_file, extra_args, warnings_as_errors, config_file):
    cmd = [clang_tidy_bin, f"-p={build_dir}", f"--config-file={config_file}"]
    cmd.extend(extra_args)
    if warnings_as_errors:
        cmd.append("--warnings-as-errors=*")
    cmd.append(source_file)
    
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return source_file, res.returncode, res.stdout, res.stderr

def classify_tu(sf):
    basename = os.path.basename(sf)
    norm = sf.replace("\\", "/")
    if "iso_Vectoris_" in basename:
        return "standalone_public_header"
    if "order_poison_" in basename:
        return "include_order_poison"
    if "modules/VectorisNumerics/tests/" in norm:
        return "numerics_tests"
    if "modules/VectorisDynamics/tests/" in norm:
        return "dynamics_tests"
    if "googletest" in norm or "_deps" in norm:
        return "third_party"
    return "other"

def derive_expected_tu_set(repo_root, build_dir):
    expected_tus = set()

    # 1. Numerics Public Headers & Header Isolation
    num_cmake = os.path.join(repo_root, "modules", "VectorisNumerics", "CMakeLists.txt")
    if not os.path.isfile(num_cmake):
        print(f"ERROR: Numerics CMakeLists.txt not found at {num_cmake}", file=sys.stderr)
        return set()
    with open(num_cmake, "r", encoding="utf-8") as f:
        c_num = re.sub(r"#.*", "", f.read())

    m_num_hdr = re.search(r"set\s*\(\s*VECTORIS_NUMERICS_PUBLIC_HEADERS\s+(.*?)\)", c_num, re.DOTALL)
    if m_num_hdr:
        num_iso_dir = os.path.join(build_dir, "modules", "VectorisNumerics", "header_isolation")
        for token in m_num_hdr.group(1).split():
            if token.endswith(".h"):
                rel = token[len("include/"):] if token.startswith("include/") else token
                san = re.sub(r"[/.]", "_", rel)
                expected_tus.add(os.path.realpath(os.path.join(num_iso_dir, f"iso_{san}.cpp")))
        expected_tus.add(os.path.realpath(os.path.join(num_iso_dir, "order_poison_forward.cpp")))
        expected_tus.add(os.path.realpath(os.path.join(num_iso_dir, "order_poison_reverse.cpp")))

    # 2. Dynamics Public Headers & Header Isolation
    dyn_cmake = os.path.join(repo_root, "modules", "VectorisDynamics", "CMakeLists.txt")
    if os.path.isfile(dyn_cmake):
        with open(dyn_cmake, "r", encoding="utf-8") as f:
            c_dyn = re.sub(r"#.*", "", f.read())
        m_dyn_hdr = re.search(r"set\s*\(\s*VECTORIS_DYNAMICS_PUBLIC_HEADERS\s+(.*?)\)", c_dyn, re.DOTALL)
        if m_dyn_hdr:
            dyn_iso_dir = os.path.join(build_dir, "modules", "VectorisDynamics", "header_isolation")
            for token in m_dyn_hdr.group(1).split():
                if token.endswith(".h"):
                    rel = token[len("include/"):] if token.startswith("include/") else token
                    san = re.sub(r"[/.]", "_", rel)
                    expected_tus.add(os.path.realpath(os.path.join(dyn_iso_dir, f"iso_{san}.cpp")))
            expected_tus.add(os.path.realpath(os.path.join(dyn_iso_dir, "order_poison_forward.cpp")))
            expected_tus.add(os.path.realpath(os.path.join(dyn_iso_dir, "order_poison_reverse.cpp")))

    # 3. Numerics Tests
    m_num_tests = re.search(r"add_executable\s*\(\s*VectorisNumerics_Tests\s+(.*?)\)", c_num, re.DOTALL)
    if m_num_tests:
        for token in m_num_tests.group(1).split():
            if token.endswith(".cpp"):
                expected_tus.add(os.path.realpath(os.path.join(repo_root, "modules", "VectorisNumerics", token)))

    # 4. Dynamics Tests
    if os.path.isfile(dyn_cmake):
        m_dyn_tests = re.search(r"add_executable\s*\(\s*VectorisDynamics_Tests\s+(.*?)\)", c_dyn, re.DOTALL)
        if m_dyn_tests:
            for token in m_dyn_tests.group(1).split():
                if token.endswith(".cpp"):
                    expected_tus.add(os.path.realpath(os.path.join(repo_root, "modules", "VectorisDynamics", token)))

    return expected_tus

def main():
    parser = argparse.ArgumentParser(description="Vectoris Clang-Tidy Qualification Runner")
    parser.add_argument("--build-dir", default="cmake-build-p2sta", help="Build directory containing compile_commands.json")
    parser.add_argument("--clang-tidy", default=None, help="Path to clang-tidy binary")
    parser.add_argument("--config-file", default=None, help="Path to .clang-tidy configuration file")
    parser.add_argument("--warnings-as-errors", action="store_true", help="Treat warnings as errors")
    parser.add_argument("--skip-category-check", action="store_true", help="Skip project TU category audit (for isolated self-tests)")
    parser.add_argument("--skip-expected-set-check", action="store_true", help="Skip exact TU expected set comparison (for isolated self-tests)")
    parser.add_argument("--jobs", "-j", type=int, default=os.cpu_count() or 4, help="Number of concurrent workers")
    parser.add_argument("--repo-root", default=None, help="Path to repository root")
    parser.add_argument("--output", default=None, help="File to write raw clang-tidy output")
    parser.add_argument("--dump-checks", default=None, help="File to dump expanded effective checks list")
    args = parser.parse_args()

    repo_root = os.path.abspath(args.repo_root) if args.repo_root else os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    build_dir = os.path.abspath(args.build_dir)
    compdb_path = os.path.join(build_dir, "compile_commands.json")

    # 1. Clang-tidy executable check (fail-closed)
    clang_tidy_bin = find_clang_tidy(args.clang_tidy)
    if not clang_tidy_bin:
        print("ERROR: clang-tidy binary not found on host system.", file=sys.stderr)
        sys.exit(1)

    # 2. Authoritative configuration file check (fail-closed, no implicit discovery)
    config_path = os.path.abspath(args.config_file) if args.config_file else os.path.join(repo_root, ".clang-tidy")
    if not os.path.isfile(config_path):
        print(f"ERROR: Authoritative Clang-Tidy configuration file not found at {config_path}", file=sys.stderr)
        sys.exit(1)

    if os.path.getsize(config_path) == 0:
        print(f"ERROR: Authoritative Clang-Tidy configuration file is empty (0 bytes): {config_path}", file=sys.stderr)
        sys.exit(1)

    with open(config_path, "rb") as f:
        config_sha256 = hashlib.sha256(f.read()).hexdigest()

    try:
        ver_out = subprocess.check_output([clang_tidy_bin, "--version"], text=True)
        first_ver_line = ver_out.splitlines()[0] if ver_out.splitlines() else ver_out
    except Exception as e:
        first_ver_line = f"Unknown ({e})"

    if args.dump_checks:
        cmd_list = [clang_tidy_bin, "--list-checks", f"--config-file={config_path}"]
        out = subprocess.check_output(cmd_list, text=True)
        with open(args.dump_checks, "w", encoding="utf-8") as f:
            f.write(out)
        print(f"Effective checks dumped to: {args.dump_checks}")

    # 3. Compilation database checks (fail-closed)
    if not os.path.isfile(compdb_path):
        print(f"ERROR: Compilation database not found at {compdb_path}", file=sys.stderr)
        sys.exit(1)

    if os.path.getsize(compdb_path) == 0:
        print(f"ERROR: Compilation database is empty (0 bytes): {compdb_path}", file=sys.stderr)
        sys.exit(1)

    try:
        with open(compdb_path, "r", encoding="utf-8") as f:
            compdb = json.load(f)
    except Exception as e:
        print(f"ERROR: Failed to parse compilation database JSON: {e}", file=sys.stderr)
        sys.exit(1)

    if not isinstance(compdb, list):
        print(f"ERROR: Compilation database must be a JSON list, got {type(compdb).__name__}", file=sys.stderr)
        sys.exit(1)

    if len(compdb) == 0:
        print(f"ERROR: Compilation database contains 0 entries (empty database rejected): {compdb_path}", file=sys.stderr)
        sys.exit(1)

    # Validate each entry, check duplicates, partition first-party vs third-party
    seen_files = []
    duplicate_tus = set()

    for idx, entry in enumerate(compdb):
        if not isinstance(entry, dict):
            print(f"ERROR: Compilation database entry [{idx}] is not a dictionary", file=sys.stderr)
            sys.exit(1)
        sf = entry.get("file")
        if not sf or not isinstance(sf, str):
            print(f"ERROR: Compilation database entry [{idx}] missing valid file string", file=sys.stderr)
            sys.exit(1)
        if not entry.get("command") and not entry.get("arguments"):
            print(f"ERROR: Compilation database entry [{idx}] for {sf} missing compile command", file=sys.stderr)
            sys.exit(1)
        norm_sf = os.path.realpath(sf)
        if norm_sf in seen_files:
            duplicate_tus.add(sf)
        seen_files.append(norm_sf)

    if duplicate_tus:
        print(f"ERROR: Duplicate TU entries found in compilation database: {sorted(list(duplicate_tus))}", file=sys.stderr)
        sys.exit(1)

    category_counts = {
        "standalone_public_header": 0,
        "include_order_poison": 0,
        "numerics_tests": 0,
        "dynamics_tests": 0,
        "third_party": 0,
        "other": 0
    }

    first_party_eligible = []
    third_party_excluded = []

    for sf in seen_files:
        cat = classify_tu(sf)
        category_counts[cat] = category_counts.get(cat, 0) + 1
        if cat == "third_party":
            third_party_excluded.append(sf)
        else:
            first_party_eligible.append(sf)

    eligible_tu_count = len(first_party_eligible)
    if eligible_tu_count == 0:
        print("ERROR: Zero eligible translation units found in compilation database.", file=sys.stderr)
        sys.exit(1)

    # Category validation (fail-closed against empty project partitions)
    if not args.skip_category_check:
        missing_categories = []
        if category_counts["standalone_public_header"] == 0:
            missing_categories.append("standalone_public_header")
        if category_counts["numerics_tests"] == 0:
            missing_categories.append("numerics_tests")
        if category_counts["dynamics_tests"] == 0:
            missing_categories.append("dynamics_tests")
        if missing_categories:
            print(f"ERROR: Required project TU categories absent in compilation database: {missing_categories}", file=sys.stderr)
            sys.exit(1)

    # Exact expected TU set comparison (VRT-11A)
    expected_tu_count = 0
    if not args.skip_expected_set_check:
        expected_tus = derive_expected_tu_set(repo_root, build_dir)
        expected_tu_count = len(expected_tus)
        if expected_tu_count == 0:
            print("ERROR: Expected-set derivation returned 0 translation units.", file=sys.stderr)
            sys.exit(1)

        eligible_set = set(first_party_eligible)
        missing_tus = expected_tus - eligible_set
        unexpected_tus = eligible_set - expected_tus

        if missing_tus:
            print(f"ERROR: Compilation database missing {len(missing_tus)} expected project TUs:\n" + "\n".join(f"  - {t}" for t in sorted(missing_tus)), file=sys.stderr)
            sys.exit(1)

        if unexpected_tus:
            print(f"ERROR: Compilation database contains {len(unexpected_tus)} unexpected first-party TUs:\n" + "\n".join(f"  - {t}" for t in sorted(unexpected_tus)), file=sys.stderr)
            sys.exit(1)

        if expected_tus != eligible_set:
            print("ERROR: Expected TU set and eligible first-party TU set do not match exactly!", file=sys.stderr)
            sys.exit(1)

    source_files = sorted(list(first_party_eligible))

    print("=" * 80)
    print("Vectoris Clang-Tidy Qualification Runner")
    print("=" * 80)
    print(f"Binary:           {clang_tidy_bin}")
    print(f"Version:          {first_ver_line}")
    print(f"Config File:      {config_path}")
    print(f"Config SHA-256:   {config_sha256}")
    print(f"Build Dir:        {build_dir}")
    print(f"Workers:          {args.jobs}")
    print(f"WarningsAsErrors: {args.warnings_as_errors}")
    resource_inc = get_compiler_resource_include()
    print(f"Resource Include: {resource_inc}")
    if not args.skip_expected_set_check:
        print(f"Expected TUs:     {expected_tu_count}")
    print(f"Eligible TUs:     {eligible_tu_count}")
    print(f"Excluded 3rd-Pty: {len(third_party_excluded)} (GoogleTest/gmock)")
    print("TU Population Breakdown:")
    for cat, count in category_counts.items():
        print(f"  - {cat:26s}: {count:3d}")
    print("=" * 80)

    extra_args = get_macos_sdk_args()

    all_stdout = []
    all_stderr = []
    failed_tus = []
    
    # Run analysis
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(run_tu, clang_tidy_bin, build_dir, sf, extra_args, args.warnings_as_errors, config_path): sf
            for sf in source_files
        }
        completed_count = 0
        for fut in as_completed(futures):
            sf, retcode, stdout, stderr = fut.result()
            completed_count += 1
            all_stdout.append(stdout)
            all_stderr.append(stderr)
            if retcode != 0:
                failed_tus.append((sf, retcode, stdout, stderr))
            print(f"[{completed_count:2d}/{len(source_files):2d}] Analyzed: {os.path.basename(sf)}")

    combined_output = "\n".join(all_stdout + all_stderr)
    if args.output:
        with open(args.output, "w", encoding="utf-8") as out_f:
            out_f.write("--- STDOUT ---\n")
            out_f.write("\n".join(all_stdout))
            out_f.write("\n--- STDERR ---\n")
            out_f.write("\n".join(all_stderr))
        print(f"Raw report saved to: {args.output}")

    # Parse diagnostics from both stdout and stderr
    diag_pattern = re.compile(r"^([^:\n]+):(\d+):(\d+):\s+(warning|error):\s+(.*?)\s+\[([^\]]+)\]", re.MULTILINE)
    
    real_repo_root = os.path.realpath(repo_root)
    real_prod_root = os.path.realpath(os.path.join(real_repo_root, "modules", "VectorisNumerics", "include", "Vectoris", "Numerics"))
    real_test_root = os.path.realpath(os.path.join(real_repo_root, "modules", "VectorisNumerics", "tests"))

    def is_contained_in(path, parent):
        try:
            return os.path.commonpath([parent, path]) == parent
        except ValueError:
            return False

    raw_diag_count = 0
    raw_production_diags = []
    raw_test_diags = []
    raw_system_diags = []

    unique_production_diags = {}
    unique_test_diags = {}
    unique_system_diags = {}

    for match in diag_pattern.finditer(combined_output):
        fpath, line, col, severity, msg, check = match.groups()
        candidate = os.path.realpath(fpath)
        diag_item = {
            "file": candidate,
            "line": int(line),
            "col": int(col),
            "severity": severity,
            "message": msg,
            "check": check,
            "raw": match.group(0)
        }
        key = (candidate, int(line), int(col), check, msg)
        raw_diag_count += 1

        if is_contained_in(candidate, real_prod_root):
            raw_production_diags.append(diag_item)
            if key not in unique_production_diags:
                unique_production_diags[key] = diag_item
        elif is_contained_in(candidate, real_test_root):
            raw_test_diags.append(diag_item)
            if key not in unique_test_diags:
                unique_test_diags[key] = diag_item
        else:
            raw_system_diags.append(diag_item)
            if key not in unique_system_diags:
                unique_system_diags[key] = diag_item

    print("\n" + "=" * 80)
    print("Clang-Tidy Diagnostics Summary")
    print("=" * 80)
    if not args.skip_expected_set_check:
        print(f"Expected Translation Units:                    {expected_tu_count}")
    print(f"Eligible Translation Units:                    {eligible_tu_count}")
    print(f"Analyzed Translation Units:                    {completed_count}")
    print(f"Failed Translation Units (crashed/errored):    {len(failed_tus)}")
    print(f"Production Diagnostics (modules/VectorisNumerics/**):  {len(unique_production_diags)} unique ({len(raw_production_diags)} raw)")
    print(f"Test Diagnostics (modules/VectorisNumerics/tests/**):  {len(unique_test_diags)} unique ({len(raw_test_diags)} raw)")
    print(f"System / External Diagnostics:                 {len(unique_system_diags)} unique ({len(raw_system_diags)} raw)")
    print(f"Total Raw Diagnostic Occurrences:              {raw_diag_count}")
    print(f"Total Unique Diagnostics:                      {len(unique_production_diags) + len(unique_test_diags) + len(unique_system_diags)}")
    print("=" * 80)

    if unique_production_diags:
        print("\n--- Production Diagnostics Details ---")
        for d in unique_production_diags.values():
            rel = os.path.relpath(d["file"], real_repo_root)
            print(f"{rel}:{d[line]}:{d[col]}: {d[severity]}: {d[message]} [{d[check]}]")
        print("=" * 80)

    if failed_tus:
        print("\n--- Failed Translation Units Details ---")
        for sf, retcode, stdout, stderr in failed_tus:
            print(f"TU {sf} exited with {retcode}")
            if stdout:
                print("  Stdout snippet:", "\n".join(stdout.splitlines()[:10]))
            if stderr:
                print("  Stderr snippet:", "\n".join(stderr.splitlines()[:5]))
        print("=" * 80)

    if completed_count != eligible_tu_count:
        print(f"RESULT: FAIL (Accounting mismatch: analyzed {completed_count} != eligible {eligible_tu_count})")
        sys.exit(1)
    elif len(failed_tus) > 0:
        print(f"RESULT: FAIL ({len(failed_tus)} translation units failed)")
        sys.exit(1)
    elif len(unique_production_diags) > 0:
        print(f"RESULT: FAIL ({len(unique_production_diags)} unresolved production diagnostics)")
        sys.exit(1)
    else:
        print("RESULT: PASS (All eligible TUs analyzed, 0 failed TUs, 0 production diagnostics)")
        sys.exit(0)

if __name__ == "__main__":
    main()
