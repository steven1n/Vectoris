#!/usr/bin/env python3
"""
AegisMathLib Clang-Tidy Qualification Harness (P2-STA)
Executes clang-tidy across compile_commands.json, isolates production
diagnostics (include/AegisMath/**), and enforces 0 unsuppressed warnings.
"""

import argparse
import glob
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
            
    # Check PATH
    try:
        which_out = subprocess.check_output(["which", "clang-tidy"], stderr=subprocess.DEVNULL).decode().strip()
        if which_out and os.path.isfile(which_out):
            return which_out
    except Exception:
        pass
        
    return None

def get_compiler_resource_include():
    try:
        res = subprocess.check_output(["/usr/bin/c++", "-print-resource-dir"], stderr=subprocess.DEVNULL, text=True).strip()
        if res and os.path.isdir(res):
            inc = os.path.join(res, "include")
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

def run_tu(clang_tidy_bin, build_dir, source_file, extra_args, warnings_as_errors):
    cmd = [clang_tidy_bin, f"-p={build_dir}"]
    cmd.extend(extra_args)
    if warnings_as_errors:
        cmd.append("--warnings-as-errors=*")
    cmd.append(source_file)
    
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return source_file, res.returncode, res.stdout, res.stderr

def main():
    parser = argparse.ArgumentParser(description="AegisMathLib Clang-Tidy Qualification Runner")
    parser.add_argument("--build-dir", default="cmake-build-p2sta", help="Build directory containing compile_commands.json")
    parser.add_argument("--clang-tidy", default=None, help="Path to clang-tidy binary")
    parser.add_argument("--warnings-as-errors", action="store_true", help="Treat warnings as errors")
    parser.add_argument("--jobs", "-j", type=int, default=os.cpu_count() or 4, help="Number of concurrent workers")
    parser.add_argument("--output", default=None, help="File to write raw clang-tidy output")
    parser.add_argument("--dump-checks", default=None, help="File to dump expanded effective checks list")
    args = parser.parse_args()

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    build_dir = os.path.abspath(args.build_dir)
    compdb_path = os.path.join(build_dir, "compile_commands.json")

    clang_tidy_bin = find_clang_tidy(args.clang_tidy)
    if not clang_tidy_bin:
        print("ERROR: clang-tidy binary not found on host system.", file=sys.stderr)
        sys.exit(1)

    try:
        ver_out = subprocess.check_output([clang_tidy_bin, "--version"], text=True)
        first_ver_line = ver_out.splitlines()[0] if ver_out.splitlines() else ver_out
    except Exception as e:
        first_ver_line = f"Unknown ({e})"

    if args.dump_checks:
        config_path = os.path.join(repo_root, ".clang-tidy")
        cmd_list = [clang_tidy_bin, "--list-checks"]
        if os.path.isfile(config_path):
            cmd_list.append(f"--config-file={config_path}")
        out = subprocess.check_output(cmd_list, text=True)
        with open(args.dump_checks, "w", encoding="utf-8") as f:
            f.write(out)
        print(f"Effective checks dumped to: {args.dump_checks}")

    if not os.path.isfile(compdb_path):
        print(f"ERROR: Compilation database not found at {compdb_path}", file=sys.stderr)
        sys.exit(1)

    print("=" * 80)
    print("VectorisNumerics Clang-Tidy Qualification Runner")
    print("=" * 80)
    print(f"Binary:     {clang_tidy_bin}")
    print(f"Version:    {first_ver_line}")
    print(f"Build Dir:  {build_dir}")
    print(f"Workers:    {args.jobs}")
    print(f"WarningsAsErrors: {args.warnings_as_errors}")
    resource_inc = get_compiler_resource_include()
    print(f"Resource Include: {resource_inc}")
    print("=" * 80)

    with open(compdb_path, "r", encoding="utf-8") as f:
        compdb = json.load(f)

    # Gather unique source files in the project
    source_files = []
    seen = set()
    for entry in compdb:
        sf = entry.get("file")
        if sf and sf not in seen:
            seen.add(sf)
            source_files.append(sf)

    print(f"Found {len(source_files)} translation units to analyze.")

    extra_args = get_macos_sdk_args()

    all_stdout = []
    all_stderr = []
    failed_tus = []
    
    # Run analysis
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(run_tu, clang_tidy_bin, build_dir, sf, extra_args, args.warnings_as_errors): sf
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
    # Format typically: /path/to/file:line:col: warning/error: message [check-name]
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
    print(f"Total Raw Diagnostic Occurrences:              {raw_diag_count}")
    print(f"Total Unique Diagnostics:                      {len(unique_production_diags) + len(unique_test_diags) + len(unique_system_diags)}")
    print(f"Production Diagnostics (modules/VectorisNumerics/**):  {len(unique_production_diags)} unique ({len(raw_production_diags)} raw)")
    print(f"Test Diagnostics (modules/VectorisNumerics/tests/**):  {len(unique_test_diags)} unique ({len(raw_test_diags)} raw)")
    print(f"System / External Diagnostics:                 {len(unique_system_diags)} unique ({len(raw_system_diags)} raw)")
    print(f"Failed Translation Units (crashed/errored):    {len(failed_tus)}")
    print("=" * 80)

    if unique_production_diags:
        print("\n--- Production Diagnostics Details ---")
        for d in unique_production_diags.values():
            rel = os.path.relpath(d["file"], real_repo_root)
            print(f"{rel}:{d['line']}:{d['col']}: {d['severity']}: {d['message']} [{d['check']}]")
        print("=" * 80)

    if failed_tus:
        print("\n--- Failed Translation Units Details ---")
        for sf, retcode, stdout, stderr in failed_tus:
            print(f"TU {sf} exited with {retcode}")
            if stderr:
                print("  Stderr snippet:", "\n".join(stderr.splitlines()[:5]))
        print("=" * 80)

    if len(unique_production_diags) > 0 or len(failed_tus) > 0:
        print("RESULT: FAIL (Unresolved production diagnostics or failed TUs)")
        sys.exit(1)
    else:
        print("RESULT: PASS (0 production diagnostics)")
        sys.exit(0)

if __name__ == "__main__":
    main()
