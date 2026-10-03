#!/usr/bin/env python3
"""
Vectoris Public API & Template Instantiation Surface Verification Gate (VRT-10)
Verifies:
1. Manifest integrity & classification semantics (fail-closed against empty/missing probes, duplicates, non-existent files)
2. Public header set consistency: Git working-tree public headers == CMake public headers == manifest
3. Positive API surface compilation & execution via CMake/CTest (records registered, executed, passed, failed)
4. Negative compile probes with control probes and pattern verification (prevents arbitrary compiler error false-passes)
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from positive_probe_contract import verify_source, verify_ctest
from verify_core_include_contract import read_contracts, compile_contracts

def resolve_compiler(explicit_cxx, build_dir):
    # 1. Explicit argument
    if explicit_cxx and shutil.which(explicit_cxx):
        return os.path.abspath(shutil.which(explicit_cxx))
    if explicit_cxx and os.path.isfile(explicit_cxx) and os.access(explicit_cxx, os.X_OK):
        return os.path.abspath(explicit_cxx)

    # 2. CXX environment variable
    env_cxx = os.environ.get("CXX")
    if env_cxx and shutil.which(env_cxx):
        return os.path.abspath(shutil.which(env_cxx))
    if env_cxx and os.path.isfile(env_cxx) and os.access(env_cxx, os.X_OK):
        return os.path.abspath(env_cxx)

    # 3. CMake compiler information if available
    if build_dir:
        cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
        if os.path.isfile(cmake_cache):
            with open(cmake_cache, "r", encoding="utf-8") as f:
                for line in f:
                    if line.startswith("CMAKE_CXX_COMPILER:FILEPATH="):
                        path = line.split("=", 1)[1].strip()
                        if os.path.isfile(path) and os.access(path, os.X_OK):
                            return os.path.abspath(path)

    # 4. Platform fallback
    for name in ["clang++", "g++", "c++"]:
        which = shutil.which(name)
        if which:
            return os.path.abspath(which)

    return None

def parse_cmake_public_headers(cmake_path):
    if not os.path.isfile(cmake_path):
        print(f"ERROR: CMakeLists.txt not found at {cmake_path}", file=sys.stderr)
        return []
    with open(cmake_path, "r", encoding="utf-8") as f:
        content = f.read()

    no_comments = re.sub(r"#.*", "", content)
    match = re.search(r"set\s*\(\s*VECTORIS_NUMERICS_PUBLIC_HEADERS\s+(.*?)\)", no_comments, re.DOTALL)
    if not match:
        print("ERROR: VECTORIS_NUMERICS_PUBLIC_HEADERS not found in CMakeLists.txt", file=sys.stderr)
        return []

    headers = []
    for token in match.group(1).split():
        if token.endswith(".h"):
            norm = os.path.join("modules", "VectorisNumerics", token).replace("\\", "/")
            headers.append(norm)
    return sorted(headers)

def parse_cmake_list_variable(cmake_path, variable):
    if not os.path.isfile(cmake_path):
        return []
    with open(cmake_path, "r", encoding="utf-8") as f:
        content = re.sub(r"#.*", "", f.read())
    match = re.search(r"set\s*\(\s*" + re.escape(variable) + r"\s+(.*?)\)", content, re.DOTALL)
    return match.group(1).split() if match else []

def get_git_public_headers(repo_root):
    try:
        # VRT-12 may introduce contract headers in an explicitly uncommitted tree.
        # Include nonignored additions; exact CMake/manifest equality still applies.
        out = subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard",
             "modules/VectorisNumerics/include/**/*.h"],
            cwd=repo_root, text=True
        )
        return sorted([line.strip() for line in out.splitlines() if line.strip()])
    except Exception as e:
        print(f"ERROR: Failed to query git for public headers: {e}", file=sys.stderr)
        return []

def main():
    parser = argparse.ArgumentParser(description="Vectoris Public API Verification Gate")
    parser.add_argument("--repo-root", default=None, help="Repository root directory")
    parser.add_argument("--manifest", default=None, help="Path to public_api_manifest.json")
    parser.add_argument("--build-dir", default=None, help="Build directory for test execution")
    parser.add_argument("--cxx", default=None, help="Path to C++ compiler executable")
    parser.add_argument("--skip-build-exec", action="store_true", help="Skip CMake build & test execution (for fast dry-run or mock tests)")
    args = parser.parse_args()

    repo_root = os.path.abspath(args.repo_root) if args.repo_root else os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    manifest_path = os.path.abspath(args.manifest) if args.manifest else os.path.join(repo_root, "tools", "api_surface", "public_api_manifest.json")
    cmake_path = os.path.join(repo_root, "modules", "VectorisNumerics", "CMakeLists.txt")
    build_dir = os.path.abspath(args.build_dir) if args.build_dir else os.path.join(repo_root, ".build", "debug")

    print("=" * 80)
    print("Vectoris Public API & Instantiation Surface Verification Gate (VRT-10)")
    print("=" * 80)

    # 1. Manifest existence and validity
    if not os.path.isfile(manifest_path):
        print(f"ERROR: Manifest file not found: {manifest_path}", file=sys.stderr)
        sys.exit(1)

    if os.path.getsize(manifest_path) == 0:
        print(f"ERROR: Manifest file is empty (0 bytes): {manifest_path}", file=sys.stderr)
        sys.exit(1)

    try:
        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)
    except Exception as e:
        print(f"ERROR: Failed to parse manifest JSON: {e}", file=sys.stderr)
        sys.exit(1)

    if not isinstance(manifest, dict):
        print(f"ERROR: Manifest must be a JSON object, got {type(manifest).__name__}", file=sys.stderr)
        sys.exit(1)

    manifest_header_entries = manifest.get("public_headers", [])
    declared_count = manifest.get("public_headers_count")
    if declared_count != len(manifest_header_entries):
        print(f"ERROR: Manifest declared count ({declared_count}) != actual entries ({len(manifest_header_entries)})", file=sys.stderr)
        sys.exit(1)

    # 2. Public header set consistency check
    git_headers = get_git_public_headers(repo_root)
    cmake_headers = parse_cmake_public_headers(cmake_path)
    manifest_headers = sorted([e.get("header_path", "") for e in manifest_header_entries])

    # Check duplicates in manifest
    if len(manifest_headers) != len(set(manifest_headers)):
        print("ERROR: Manifest contains duplicate header entries!", file=sys.stderr)
        sys.exit(1)

    # Check on-disk existence for every manifest header
    for h in manifest_headers:
        disk_path = os.path.join(repo_root, h)
        if not os.path.isfile(disk_path):
            print(f"ERROR: Manifest header file does not exist on disk: {disk_path}", file=sys.stderr)
            sys.exit(1)

    print(f"Git Working-Tree Public Headers:  {len(git_headers)}")
    print(f"CMake Declared Public Headers:    {len(cmake_headers)}")
    print(f"Manifest Declared Public Headers: {len(manifest_headers)}")

    if not (set(git_headers) == set(cmake_headers) == set(manifest_headers)):
        diff_git_cmake = set(git_headers) ^ set(cmake_headers)
        diff_git_manifest = set(git_headers) ^ set(manifest_headers)
        if diff_git_cmake:
            print(f"ERROR: Discrepancy between Git and CMake headers:\n  {diff_git_cmake}", file=sys.stderr)
        if diff_git_manifest:
            print(f"ERROR: Discrepancy between Git and Manifest headers:\n  {diff_git_manifest}", file=sys.stderr)
        sys.exit(1)

    print("Header Set Consistency:           PASS (All 3 sources strictly 100% identical)")

    # 3. Verify Positive Probes in PublicApiSurfaceTest.cpp
    test_src = os.path.join(repo_root, "modules", "VectorisNumerics", "tests", "PublicApi", "PublicApiSurfaceTest.cpp")
    if not os.path.isfile(test_src):
        print(f"ERROR: Public API surface test source not found at {test_src}", file=sys.stderr)
        sys.exit(1)

    with open(test_src, "r", encoding="utf-8") as f:
        src_content = f.read()

    required_identities = manifest.get("required_positive_tests", [])
    try:
        source_defined = verify_source(required_identities, src_content)
    except (ValueError, TypeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
    discovered_source_tests = {name.split(".", 1)[1] for name in source_defined}
    print(f"Positive API Test Source:         {os.path.relpath(test_src, repo_root)}")
    print(f"Discovered Test Definitions:      {len(source_defined)}")
    print("REQUIRED=" + json.dumps(sorted(required_identities)))
    print("SOURCE_DEFINED=" + json.dumps(sorted(source_defined)))

    # 4. Machine-verifiable declared Dynamics semantic API contracts.
    dynamics_contracts = manifest.get("dynamics_public_api_contracts", [])
    if not isinstance(dynamics_contracts, list) or not dynamics_contracts:
        print("ERROR: No Dynamics public API contracts declared in manifest!", file=sys.stderr)
        sys.exit(1)

    dynamics_module = os.path.join(repo_root, "modules", "VectorisDynamics")
    dynamics_cmake = os.path.join(dynamics_module, "CMakeLists.txt")
    dynamics_headers = {
        os.path.join("modules", "VectorisDynamics", token).replace("\\", "/")
        for token in parse_cmake_list_variable(dynamics_cmake, "VECTORIS_DYNAMICS_PUBLIC_HEADERS")
        if token.endswith(".h")
    }
    dynamics_test_sources = set(parse_cmake_list_variable(dynamics_cmake, "VECTORIS_DYNAMICS_TEST_SOURCES"))
    # Existing CMake files list test sources directly inside the target declaration.
    if os.path.isfile(dynamics_cmake):
        with open(dynamics_cmake, "r", encoding="utf-8") as f:
            dynamics_cmake_text = re.sub(r"#.*", "", f.read())
        tests_match = re.search(r"add_executable\s*\(\s*VectorisDynamics_Tests\s+(.*?)\)", dynamics_cmake_text, re.DOTALL)
        if tests_match:
            dynamics_test_sources.update(token for token in tests_match.group(1).split() if token.endswith(".cpp"))

    dynamics_positive_tests = []
    for contract in dynamics_contracts:
        if not isinstance(contract, dict):
            print("ERROR: Dynamics API contract entries must be objects.", file=sys.stderr)
            sys.exit(1)
        header_path = contract.get("header_path", "")
        if contract.get("module") != "VectorisDynamics" or contract.get("canonical_namespace") != "vectoris::dynamics":
            print(f"ERROR: Invalid module or namespace in Dynamics API contract: {contract}", file=sys.stderr)
            sys.exit(1)
        if header_path not in dynamics_headers or not os.path.isfile(os.path.join(repo_root, header_path)):
            print(f"ERROR: Dynamics API contract header is not a declared public header: {header_path}", file=sys.stderr)
            sys.exit(1)
        test_source = contract.get("positive_test_source", "")
        source_path = os.path.join(repo_root, test_source)
        if not os.path.isfile(source_path):
            print(f"ERROR: Dynamics API positive test source not found: {test_source}", file=sys.stderr)
            sys.exit(1)
        module_relative_source = os.path.relpath(source_path, dynamics_module).replace("\\", "/")
        if module_relative_source not in dynamics_test_sources:
            print(f"ERROR: Dynamics API positive source is not in VectorisDynamics_Tests: {test_source}", file=sys.stderr)
            sys.exit(1)
        with open(source_path, "r", encoding="utf-8") as f:
            dynamics_source = f.read()
        discovered_dynamics_tests = {
            f"{suite}.{name}"
            for suite, name in re.findall(r"TEST\s*\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*\)", dynamics_source)
        }
        positive_test = contract.get("positive_test", "")
        if not positive_test or positive_test not in discovered_dynamics_tests:
            print(f"ERROR: Dynamics API contract references non-existent Dynamics API probe: {positive_test}", file=sys.stderr)
            sys.exit(1)
        if not contract.get("api") or not contract.get("compile_time_contracts"):
            print(f"ERROR: Dynamics API contract is missing API or compile-time contract details: {header_path}", file=sys.stderr)
            sys.exit(1)
        dynamics_positive_tests.append(positive_test)

    print(f"Declared Dynamics API Contracts: {len(dynamics_contracts)}")

    # 5. Machine-verifiable manifest classifications & probe references
    valid_classifications = {
        "declaration_only",
        "concept_or_trait",
        "constant_or_metadata",
        "runtime_template_api",
        "compile_time_api",
        "internal_public_helper"
    }

    runtime_template_headers = []
    other_headers = []

    for entry in manifest_header_entries:
        h_path = entry.get("header_path", "")
        classification = entry.get("classification")
        if not classification or classification not in valid_classifications:
            print(f"ERROR: Header {h_path} has invalid or missing classification: {classification}", file=sys.stderr)
            sys.exit(1)

        # VRT-12 namespace metadata is checked against the header's owning layer.
        family = h_path.split("/Vectoris/Numerics/")[-1].split("/")[0]
        if (entry.get("canonical_namespace") != "vectoris::numerics::" + family.lower() or
                entry.get("compatibility_namespace") != "vectoris::numerics::" + family):
            print(f"ERROR: Invalid namespace contract for {h_path}", file=sys.stderr)
            sys.exit(1)
        namespace_probe = entry.get("namespace_probe")
        if namespace_probe and not os.path.isfile(os.path.join(repo_root, namespace_probe)):
            print(f"ERROR: Namespace probe does not exist: {namespace_probe}", file=sys.stderr)
            sys.exit(1)

        if classification == "runtime_template_api":
            runtime_template_headers.append(entry)
            probes = entry.get("positive_probes", [])
            if not probes or len(probes) == 0:
                print(f"ERROR: runtime_template_api header has 0 positive probes: {h_path}", file=sys.stderr)
                sys.exit(1)
            for p in probes:
                if p not in discovered_source_tests:
                    print(f"ERROR: Header {h_path} references non-existent positive probe: {p}", file=sys.stderr)
                    sys.exit(1)
        else:
            other_headers.append(entry)

    print(f"Classification Breakdown:")
    print(f"  - runtime_template_api (explicit probes): {len(runtime_template_headers)}")
    print(f"  - declaration / trait / metadata / helper: {len(other_headers)}")

    # 6. Build and Execute Positive Tests via CMake/CTest (VRT-10A)
    registered_tests = 0
    executed_tests = 0
    passed_tests = 0
    failed_tests = 0

    if not args.skip_build_exec:
        print("\nBuilding and Executing Positive API Tests (CMake/CTest):")
        if not os.path.isdir(build_dir):
            print(f"ERROR: Build directory does not exist: {build_dir}", file=sys.stderr)
            sys.exit(1)

        # The declared namespace surface includes all standalone header probes.
        print(f"Building Numerics/Dynamics API tests and HeaderIsolation in {build_dir}...")
        build_res = subprocess.run(["cmake", "--build", build_dir, "--target", "VectorisNumerics_Tests", "VectorisNumerics_HeaderIsolation", "VectorisDynamics_Tests", "VectorisDynamics_HeaderIsolation", "-j16"], capture_output=True, text=True)
        if build_res.returncode != 0:
            print("ERROR: Compilation of a Numerics/Dynamics API test or HeaderIsolation target failed!", file=sys.stderr)
            print(build_res.stdout)
            print(build_res.stderr, file=sys.stderr)
            sys.exit(1)

        try:
            identities = verify_ctest(build_dir, required_identities)
        except (ValueError, KeyError, OSError) as exc:
            print(f"ERROR: Positive API identity verification failed: {exc}", file=sys.stderr)
            sys.exit(1)
        for stage, names in identities.items():
            print(stage + "=" + json.dumps(names))
        registered_tests = len(identities["REGISTERED"])
        executed_tests = len(identities["EXECUTED"])
        passed_tests = len(identities["PASSED"])
        print("Positive Test Execution:          PASS (exact identity equality)")

        # Each declared Dynamics contract is compiled in the module test target and
        # executed independently so manifest entries cannot be descriptive-only.
        for dynamics_test in dynamics_positive_tests:
            try:
                verify_ctest(build_dir, [dynamics_test],
                             selection="^" + re.escape(dynamics_test) + "$")
            except (ValueError, KeyError, OSError) as exc:
                print(f"ERROR: Dynamics API body verification failed: {exc}", file=sys.stderr)
                sys.exit(1)
            print(f"Dynamics API Probe:               PASS ({dynamics_test})")
    else:
        print("Positive Test Execution:          SKIPPED (--skip-build-exec)")

    # 6. Negative Compile Probes & Controls (VRT-10C)
    compiler_bin = resolve_compiler(args.cxx, build_dir)
    if not compiler_bin:
        print("ERROR: Could not resolve C++ compiler executable!", file=sys.stderr)
        sys.exit(1)

    print(f"Resolved C++ Compiler:            {compiler_bin}")

    negative_probes = manifest.get("negative_compile_probes", [])
    if not negative_probes or len(negative_probes) == 0:
        print("ERROR: No negative compile probes declared in manifest!", file=sys.stderr)
        sys.exit(1)

    inc_dir = os.path.join(repo_root, "modules", "VectorisNumerics", "include")
    print("\nExecuting Compile-Fail Probes & Positive Controls:")
    for probe in negative_probes:
        pid = probe.get("id", "UNKNOWN")
        invalid_rel = probe.get("file", "")
        control_rel = probe.get("control_file", "")
        desc = probe.get("description", "")
        pattern = probe.get("expected_failure_pattern", "")

        invalid_file = os.path.join(repo_root, invalid_rel)
        control_file = os.path.join(repo_root, control_rel)

        if not os.path.isfile(invalid_file):
            print(f"ERROR: Invalid probe file not found: {invalid_file}", file=sys.stderr)
            sys.exit(1)
        if not os.path.isfile(control_file):
            print(f"ERROR: Control probe file not found: {control_file}", file=sys.stderr)
            sys.exit(1)

        # Step 6a: Run positive control probe (MUST compile cleanly)
        ctrl_cmd = [compiler_bin, "-std=c++20", "-fsyntax-only", f"-I{inc_dir}", control_file]
        ctrl_res = subprocess.run(ctrl_cmd, capture_output=True, text=True)
        if ctrl_res.returncode != 0:
            print(f"ERROR: Positive control probe failed to compile: {control_rel}", file=sys.stderr)
            print(ctrl_res.stderr, file=sys.stderr)
            sys.exit(1)
        print(f"  [PASS] Control probe compiled cleanly: {os.path.basename(control_file)}")

        # Step 6b: Run invalid probe (MUST fail compilation AND contain expected diagnostic)
        inv_cmd = [compiler_bin, "-std=c++20", "-fsyntax-only", f"-I{inc_dir}", invalid_file]
        inv_res = subprocess.run(inv_cmd, capture_output=True, text=True)
        if inv_res.returncode == 0:
            print(f"ERROR: Negative probe compiled unexpectedly (expected failure): {invalid_rel}", file=sys.stderr)
            sys.exit(1)

        if pattern and pattern not in inv_res.stderr:
            print(f"ERROR: Negative probe failed without expected diagnostic pattern {pattern}:\n{inv_res.stderr}", file=sys.stderr)
            sys.exit(1)

        print(f"  [PASS] Invalid probe rejected with expected diagnostic: {os.path.basename(invalid_file)} ({desc})")

    # AFA3-001: the documentation itself supplies the source, rather than a
    # separately maintained consumer with unrelated umbrella includes.
    try:
        core_sources = read_contracts(repo_root, manifest=manifest_path)
        if not args.skip_build_exec:
            compiler_style = "MSVC" if os.path.basename(compiler_bin).lower() in ("cl", "cl.exe") else "GNU"
            compile_contracts(repo_root, compiler_bin, compiler_style, core_sources)
        else:
            print("Core documented consumers: SOURCE VALIDATED; execution NOT VERIFIED (--skip-build-exec)")
    except (OSError, ValueError, IndexError, subprocess.SubprocessError) as exc:
        print(f"ERROR: Core documented include contract: {exc}", file=sys.stderr)
        sys.exit(1)

    print("\n" + "=" * 80)
    print("Public API Surface Verification Summary")
    print("=" * 80)
    print(f"Public Headers Represented:        {len(manifest_headers)} / {len(git_headers)} (100.0%)")
    print(f"Positive API Tests Registered:     {registered_tests}")
    print(f"Positive API Tests Executed:       {executed_tests}")
    print(f"Positive API Tests Passed:         {passed_tests}")
    print(f"Positive API Tests Failed:         {failed_tests}")
    print(f"Negative Compile Probes:           {len(negative_probes)} / {len(negative_probes)} (100.0%)")
    print(f"Migrated Requires Assertions:      {len(manifest.get('migrated_requires_assertions', []))}")
    print("=" * 80)
    print("RESULT: PARTIAL (source/compile checks only; execution NOT VERIFIED)" if args.skip_build_exec
          else "RESULT: PASS (Declared supported public API instantiation surface verified)")
    sys.exit(0)

if __name__ == "__main__":
    main()
