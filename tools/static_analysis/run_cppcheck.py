#!/usr/bin/env python3
"""
AegisMathLib Cppcheck Qualification Harness (P2-STA)
Executes cppcheck across include/AegisMath/** if available.
If tool is missing from host, reports NOT RUN per Engineering Standard & Audit policy.
"""

import argparse
import os
import subprocess
import sys

def find_cppcheck(explicit_path=None):
    if explicit_path and os.path.isfile(explicit_path):
        return explicit_path
        
    candidate = os.environ.get("VECTORIS_CPPCHECK") or os.environ.get("AEGISMATH_CPPCHECK")
    if candidate and os.path.isfile(candidate):
        return candidate
        
    try:
        which_out = subprocess.check_output(["which", "cppcheck"], stderr=subprocess.DEVNULL).decode().strip()
        if which_out and os.path.isfile(which_out):
            return which_out
    except Exception:
        pass
        
    return None

def main():
    parser = argparse.ArgumentParser(description="VectorisNumerics Cppcheck Qualification Runner")
    parser.add_argument("--cppcheck", default=None, help="Path to cppcheck binary")
    parser.add_argument("--xml-output", default=None, help="Path to write XML report")
    args = parser.parse_args()

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    cppcheck_bin = find_cppcheck(args.cppcheck)

    print("=" * 80)
    print("VectorisNumerics Cppcheck Qualification Runner")
    print("=" * 80)

    if not cppcheck_bin:
        print("STATUS: NOT RUN (cppcheck binary not found on host system)")
        print("Cppcheck qualification:")
        print("  NOT RUN")
        print("  NON-BLOCKING UNDER CURRENT ENGINEERING STANDARD")
        print("Per ENGINEERING_STANDARD_V1.md Section 87 & Stable-Core Audit Policy:")
        print("  cppcheck is a RECOMMENDED analyzer (clang-tidy is MANDATORY).")
        print("  Host absence is recorded as NOT RUN; toolchain was not installed/upgraded.")
        print("=" * 80)
        # Return 0 so as not to crash informational builds when cppcheck is absent
        sys.exit(0)

    try:
        ver_out = subprocess.check_output([cppcheck_bin, "--version"], text=True).strip()
    except Exception as e:
        ver_out = f"Unknown ({e})"

    scope_path = os.path.join("modules", "VectorisNumerics", "include", "Vectoris", "Numerics")
    print(f"Binary:  {cppcheck_bin}")
    print(f"Version: {ver_out}")
    print(f"Scope:   {scope_path}")
    print("=" * 80)

    cmd = [
        cppcheck_bin,
        "--std=c++20",
        "--enable=warning,style,performance,portability",
        "--inconclusive",
        "--force",
        "--error-exitcode=1",
        "-I", os.path.join(repo_root, "modules", "VectorisNumerics", "include"),
    ]
    if args.xml_output:
        cmd.extend(["--xml", "--xml-version=2"])

    cmd.append(os.path.join(repo_root, scope_path))

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if args.xml_output:
        with open(args.xml_output, "w", encoding="utf-8") as xf:
            xf.write(res.stderr)
        print(f"Report written to: {args.xml_output}")
    else:
        print(res.stderr)

    sys.exit(res.returncode)

if __name__ == "__main__":
    main()
