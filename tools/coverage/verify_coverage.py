#!/usr/bin/env python3
"""
AegisMathLib Stable-Core Test Coverage Qualification Gate (P2-COV)

Enforces normative coverage thresholds from docs/ENGINEERING_STANDARD_V1.md Section 86:
- 100% Function coverage
- >= 95.0% Line coverage
- >= 90.0% Branch coverage

Excludes tests, dependencies, and external headers. Validates export scope.
"""

import argparse
import json
import os
import sys

REQ_FUNCTION_PERCENT = 100.0
REQ_LINE_PERCENT = 95.0
REQ_BRANCH_PERCENT = 90.0

def parse_args():
    parser = argparse.ArgumentParser(description="Verify AegisMathLib coverage thresholds.")
    parser.add_argument("summary_json", help="Path to llvm-cov export summary JSON file")
    parser.add_argument("--repo-root", default=".", help="Repository root directory")
    return parser.parse_args()

def classify_module(relpath):
    parts = relpath.split(os.sep)
    if len(parts) > 2 and parts[0] == "include" and parts[1] == "AegisMath":
        return parts[2]
    return "Other"

def main():
    args = parse_args()
    if not os.path.exists(args.summary_json):
        print(f"ERROR: Summary file not found: {args.summary_json}", file=sys.stderr)
        sys.exit(1)

    req_function = REQ_FUNCTION_PERCENT
    req_line = REQ_LINE_PERCENT
    req_branch = REQ_BRANCH_PERCENT
    active_deviation_id = None

    with open(args.summary_json, "r", encoding="utf-8") as f:
        data = json.load(f)

    if "data" not in data or not data["data"]:
        print("ERROR: Invalid llvm-cov export JSON format.", file=sys.stderr)
        sys.exit(1)

    export_files = data["data"][0].get("files", [])
    totals = data["data"][0].get("totals", {})

    # Vacuous PASS / empty denominator guard
    if not export_files:
        print("ERROR: Coverage export contains no files (empty denominator).", file=sys.stderr)
        sys.exit(1)

    # Repository canonical path validation
    repo_root = os.path.realpath(args.repo_root)
    production_root = os.path.realpath(os.path.join(repo_root, "include", "AegisMath"))

    invalid_files = []
    found_relpaths = set()
    for entry in export_files:
        fn = entry.get("filename", "")
        file_real = os.path.realpath(fn)
        try:
            if os.path.commonpath([production_root, file_real]) != production_root:
                invalid_files.append(fn)
                continue
        except ValueError:
            invalid_files.append(fn)
            continue
        rel_from_repo = os.path.relpath(file_real, repo_root)
        found_relpaths.add(rel_from_repo)

    if invalid_files:
        print(f"ERROR: Export contains files outside {production_root}:\n" + "\n".join(invalid_files), file=sys.stderr)
        sys.exit(1)

    # Phase 17 & 18: Comprehensive Scope Manifest Validation
    manifest_path = os.path.join(repo_root, "tools", "coverage", "coverage_scope.json")
    if os.path.exists(manifest_path):
        with open(manifest_path, "r", encoding="utf-8") as f_man:
            manifest = json.load(f_man)

        thresholds = manifest.get("coverage_thresholds", {})
        if thresholds:
            req_function = float(thresholds.get("functions", req_function))
            req_line = float(thresholds.get("lines", req_line))
            req_branch = float(thresholds.get("branches", req_branch))
            active_deviation_id = thresholds.get("deviation_id", None)

        tracked_headers = set(manifest.get("tracked_production_headers", []))
        runtime_headers = set(manifest.get("runtime_coverage_headers", []))
        compile_time_headers = set(manifest.get("compile_time_only_headers", []))
        template_headers = set(manifest.get("template_definition_headers", []))
        template_evidence = manifest.get("template_qualification_evidence", {})

        # 1. Unclassified and unknown header check
        classified = runtime_headers | compile_time_headers
        unclassified = tracked_headers - classified
        if unclassified:
            print("ERROR: Manifest contains unclassified tracked production headers:", file=sys.stderr)
            for u in sorted(unclassified):
                print(f"  - {u}", file=sys.stderr)
            sys.exit(1)

        unknown = classified - tracked_headers
        if unknown:
            print("ERROR: Manifest contains unknown headers not in tracked production headers:", file=sys.stderr)
            for unk in sorted(unknown):
                print(f"  - {unk}", file=sys.stderr)
            sys.exit(1)

        # 2. Check that all runtime_coverage_headers are present in llvm-cov export
        missing_runtime = runtime_headers - found_relpaths
        if missing_runtime:
            print("ERROR: Missing expected runtime coverage file(s) in llvm-cov export:", file=sys.stderr)
            for m in sorted(missing_runtime):
                print(f"  - {m}", file=sys.stderr)
            sys.exit(1)

        # 3. Check that every template_definition_header has explicit qualification evidence
        missing_evidence = []
        for th in template_headers:
            if th not in template_evidence or not template_evidence[th].get("symbols"):
                missing_evidence.append(th)
        if missing_evidence:
            print("ERROR: Template definition headers lack explicit qualification evidence:", file=sys.stderr)
            for me in sorted(missing_evidence):
                print(f"  - {me}", file=sys.stderr)
            sys.exit(1)

    # Module breakdown
    module_data = {}
    for entry in export_files:
        fn = entry.get("filename", "")
        # Extract relative path from include/AegisMath/
        idx = fn.find("include/AegisMath/")
        relpath = fn[idx:] if idx != -1 else fn
        mod = classify_module(relpath)
        if mod not in module_data:
            module_data[mod] = {
                "functions": {"count": 0, "covered": 0},
                "lines": {"count": 0, "covered": 0},
                "branches": {"count": 0, "covered": 0},
                "instantiations": {"count": 0, "covered": 0},
                "regions": {"count": 0, "covered": 0},
            }
        summary = entry.get("summary", {})
        for metric in ["functions", "lines", "branches", "instantiations", "regions"]:
            module_data[mod][metric]["count"] += summary.get(metric, {}).get("count", 0)
            module_data[mod][metric]["covered"] += summary.get(metric, {}).get("covered", 0)

    # Metrics from totals
    func_count = totals.get("functions", {}).get("count", 0)
    func_covered = totals.get("functions", {}).get("covered", 0)
    if func_count == 0:
        print("ERROR: Total function count is 0 (vacuous pass rejected).", file=sys.stderr)
        sys.exit(1)
    func_pct = func_covered / func_count * 100.0

    line_count = totals.get("lines", {}).get("count", 0)
    line_covered = totals.get("lines", {}).get("covered", 0)
    if line_count == 0:
        print("ERROR: Total line count is 0 (vacuous pass rejected).", file=sys.stderr)
        sys.exit(1)
    line_pct = line_covered / line_count * 100.0

    branch_count = totals.get("branches", {}).get("count", 0)
    branch_covered = totals.get("branches", {}).get("covered", 0)
    if branch_count == 0:
        print("ERROR: Total branch count is 0 (vacuous pass rejected).", file=sys.stderr)
        sys.exit(1)
    branch_pct = branch_covered / branch_count * 100.0

    inst_count = totals.get("instantiations", {}).get("count", 0)
    inst_covered = totals.get("instantiations", {}).get("covered", 0)
    inst_pct = (inst_covered / inst_count * 100.0) if inst_count > 0 else 0.0

    reg_count = totals.get("regions", {}).get("count", 0)
    reg_covered = totals.get("regions", {}).get("covered", 0)
    reg_pct = (reg_covered / reg_count * 100.0) if reg_count > 0 else 0.0

    print("=" * 80)
    print("AegisMathLib Stable-Core Test Coverage Report (P2-COV)")
    print("=" * 80)
    print(f"Production Scope: include/AegisMath/** (Files reporting: {len(export_files)})")
    print("-" * 80)
    print(f"{'Metric':<18} | {'Covered':<10} | {'Total':<10} | {'Percent':<10} | {'Threshold':<12} | {'Status'}")
    print("-" * 80)

    func_pass = (func_covered == func_count) and (func_count > 0)
    line_pass = (line_pct >= req_line) and (line_count > 0)
    branch_pass = (branch_pct >= req_branch) and (branch_count > 0)

    print(f"{'Functions':<18} | {func_covered:<10} | {func_count:<10} | {func_pct:>7.2f}%   | {req_function:>7.2f}%    | {'PASS' if func_pass else 'FAIL'}")
    print(f"{'Lines':<18} | {line_covered:<10} | {line_count:<10} | {line_pct:>7.2f}%   | >={req_line:>5.2f}%    | {'PASS' if line_pass else 'FAIL'}")
    print(f"{'Branches':<18} | {branch_covered:<10} | {branch_count:<10} | {branch_pct:>7.2f}%   | >={req_branch:>5.2f}%    | {'PASS' if branch_pass else 'FAIL'}")
    print(f"{'Instantiations*':<18} | {inst_covered:<10} | {inst_count:<10} | {inst_pct:>7.2f}%   | (informational)| PASS")
    print(f"{'Regions*':<18} | {reg_covered:<10} | {reg_count:<10} | {reg_pct:>7.2f}%   | (informational)| PASS")
    print("-" * 80)

    print("\n--- Coverage by Module ---")
    print(f"{'Module':<12} | {'Functions':<16} | {'Lines':<16} | {'Branches':<16}")
    print("-" * 66)
    for mod, metrics in sorted(module_data.items()):
        f_cov, f_tot = metrics["functions"]["covered"], metrics["functions"]["count"]
        f_p = (f_cov / f_tot * 100.0) if f_tot > 0 else 100.0
        l_cov, l_tot = metrics["lines"]["covered"], metrics["lines"]["count"]
        l_p = (l_cov / l_tot * 100.0) if l_tot > 0 else 100.0
        b_cov, b_tot = metrics["branches"]["covered"], metrics["branches"]["count"]
        b_p = (b_cov / b_tot * 100.0) if b_tot > 0 else 100.0
        print(f"{mod:<12} | {f_cov:>4}/{f_tot:<4} ({f_p:>5.1f}%) | {l_cov:>4}/{l_tot:<4} ({l_p:>5.1f}%) | {b_cov:>4}/{b_tot:<4} ({b_p:>5.1f}%)")
    print("=" * 80)

    all_passed = func_pass and line_pass and branch_pass
    if not all_passed:
        print("\nGATE STATUS: FAIL (Coverage below normative threshold)", file=sys.stderr)
        sys.exit(1)
    else:
        print("\nGATE STATUS: PASS (All normative thresholds satisfied)")
        if active_deviation_id:
            print(f"Active Registered Deviation Applied: {active_deviation_id} (see docs/DEVIATIONS.md)")
        sys.exit(0)

if __name__ == "__main__":
    main()
