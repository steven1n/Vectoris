#!/usr/bin/env python3
"""
Vectoris Public API Surface Gate Self-Test Suite (VRT-10D)
Deterministic verification that tools/api_surface/verify_api_surface.py fails-closed against:
1. Missing manifest
2. Empty manifest (0-byte)
3. Malformed manifest (invalid JSON)
4. Duplicate header entry in manifest
5. Header missing from manifest
6. Nonexistent referenced positive probe
7. runtime_template_api with zero positive probes
8. Missing positive test suite source
9. Zero discovered positive test definitions
10. Failing positive API test
11. Negative probe unexpectedly accepted (compiles without error)
12. Broken negative-probe control (fails to compile)
13. Minimal valid controlled PASS
14. Untracked undeclared public header
15. Wrong canonical namespace metadata
16. Missing direct namespace probe
17. Non-existent declared Dynamics semantic API probe
"""

import json
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

def run_gate(args, cwd=None):
    verifier = os.path.abspath(os.path.join(os.path.dirname(__file__), "verify_api_surface.py"))
    cmd = [sys.executable, verifier] + args
    return subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)

def test_missing_manifest():
    res = run_gate(["--manifest", "/nonexistent/manifest.json", "--skip-build-exec"])
    assert res.returncode != 0, "Gate should fail on missing manifest"
    assert "Manifest file not found" in res.stderr
    print("  [TEST 1/17] Missing manifest ... PASS")

def test_empty_manifest():
    with tempfile.NamedTemporaryFile(suffix=".json") as tf:
        res = run_gate(["--manifest", tf.name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on empty manifest"
        assert "is empty (0 bytes)" in res.stderr
    print("  [TEST 2/17] Empty manifest (0-byte) ... PASS")

def test_malformed_manifest():
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tf:
        tf.write("{ invalid json: [")
        tf_name = tf.name
    try:
        res = run_gate(["--manifest", tf_name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on malformed JSON"
        assert "Failed to parse manifest JSON" in res.stderr
    finally:
        os.remove(tf_name)
    print("  [TEST 3/17] Malformed manifest JSON ... PASS")

def test_duplicate_header(repo_root, valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    dup_entry = bad["public_headers"][0]
    bad["public_headers"].append(dup_entry)
    bad["public_headers_count"] = len(bad["public_headers"])
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tf:
        json.dump(bad, tf)
        tf_name = tf.name
    try:
        res = run_gate(["--manifest", tf_name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on duplicate header"
        assert "duplicate header entries" in res.stderr
    finally:
        os.remove(tf_name)
    print("  [TEST 4/17] Duplicate header entry ... PASS")

def test_missing_header_from_manifest(repo_root, valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    bad["public_headers"].pop() # Remove one header
    bad["public_headers_count"] = len(bad["public_headers"])
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tf:
        json.dump(bad, tf)
        tf_name = tf.name
    try:
        res = run_gate(["--manifest", tf_name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on missing header"
        assert "Discrepancy between Git and Manifest headers" in res.stderr
    finally:
        os.remove(tf_name)
    print("  [TEST 5/17] Header missing from manifest ... PASS")

def test_nonexistent_probe_reference(repo_root, valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    for h in bad["public_headers"]:
        if h.get("classification") == "runtime_template_api":
            h["positive_probes"] = ["NonExistentProbe_XYZ_12345"]
            break
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tf:
        json.dump(bad, tf)
        tf_name = tf.name
    try:
        res = run_gate(["--manifest", tf_name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on nonexistent probe reference"
        assert "references non-existent positive probe" in res.stderr
    finally:
        os.remove(tf_name)
    print("  [TEST 6/17] Nonexistent positive probe reference ... PASS")

def test_zero_probes_for_runtime_template(repo_root, valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    for h in bad["public_headers"]:
        if h.get("classification") == "runtime_template_api":
            h["positive_probes"] = []
            break
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tf:
        json.dump(bad, tf)
        tf_name = tf.name
    try:
        res = run_gate(["--manifest", tf_name, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on empty probes for runtime_template_api"
        assert "runtime_template_api header has 0 positive probes" in res.stderr
    finally:
        os.remove(tf_name)
    print("  [TEST 7/17] runtime_template_api with 0 positive probes ... PASS")

def test_missing_positive_suite(repo_root, valid_manifest):
    with tempfile.TemporaryDirectory() as temp_dir:
        shutil.copytree(os.path.join(repo_root, "modules"), os.path.join(temp_dir, "modules"))
        subprocess.run(["git", "init"], cwd=temp_dir, capture_output=True)
        subprocess.run(["git", "add", "."], cwd=temp_dir, capture_output=True)
        test_path = os.path.join(temp_dir, "modules", "VectorisNumerics", "tests", "PublicApi", "PublicApiSurfaceTest.cpp")
        if os.path.isfile(test_path):
            os.remove(test_path)
        manifest_copy = os.path.join(temp_dir, "manifest.json")
        with open(manifest_copy, "w") as f:
            json.dump(valid_manifest, f)
        res = run_gate(["--repo-root", temp_dir, "--manifest", manifest_copy, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on missing positive test suite"
        assert "Public API surface test source not found" in res.stderr
    print("  [TEST 8/17] Missing positive test suite source ... PASS")

def test_zero_discovered_positive_tests(repo_root, valid_manifest):
    with tempfile.TemporaryDirectory() as temp_dir:
        shutil.copytree(os.path.join(repo_root, "modules"), os.path.join(temp_dir, "modules"))
        subprocess.run(["git", "init"], cwd=temp_dir, capture_output=True)
        subprocess.run(["git", "add", "."], cwd=temp_dir, capture_output=True)
        test_path = os.path.join(temp_dir, "modules", "VectorisNumerics", "tests", "PublicApi", "PublicApiSurfaceTest.cpp")
        with open(test_path, "w") as f:
            f.write("// Empty test file without TEST() macros\n")
        manifest_copy = os.path.join(temp_dir, "manifest.json")
        with open(manifest_copy, "w") as f:
            json.dump(valid_manifest, f)
        res = run_gate(["--repo-root", temp_dir, "--manifest", manifest_copy, "--skip-build-exec"])
        assert res.returncode != 0, "Gate should fail on zero discovered tests"
        assert "SOURCE_DEFINED:" in res.stderr
    print("  [TEST 9/17] Zero discovered positive test definitions ... PASS")

def test_failing_positive_api_test(repo_root):
    # Non-existent build directory forces execution failure
    res = run_gate(["--build-dir", "/nonexistent/build_dir"])
    assert res.returncode != 0, "Gate should fail when test build directory does not exist"
    assert "Build directory does not exist" in res.stderr
    print("  [TEST 10/17] Positive API test failure propagation ... PASS")

def test_negative_probe_unexpectedly_accepted(repo_root, valid_manifest):
    with tempfile.TemporaryDirectory() as temp_dir:
        fake_probe = os.path.join(temp_dir, "valid_probe.cpp")
        with open(fake_probe, "w") as f:
            f.write("int main() { return 0; }\n") # This compiles cleanly!

        bad_manifest = json.loads(json.dumps(valid_manifest))
        bad_manifest["negative_compile_probes"] = [{
            "id": "NEG-FAKE",
            "file": os.path.relpath(fake_probe, repo_root),
            "control_file": os.path.relpath(fake_probe, repo_root),
            "description": "Probe that compiles unexpectedly",
            "expected_failure_pattern": "some_pattern"
        }]
        mf_path = os.path.join(temp_dir, "manifest.json")
        with open(mf_path, "w") as f:
            json.dump(bad_manifest, f)

        res = run_gate(["--manifest", mf_path, "--skip-build-exec"])
        assert res.returncode != 0, "Gate must fail if negative probe compiles unexpectedly"
        assert "compiled unexpectedly" in res.stderr
    print("  [TEST 11/17] Negative probe unexpectedly accepted ... PASS")

def test_broken_negative_probe_control(repo_root, valid_manifest):
    with tempfile.TemporaryDirectory() as temp_dir:
        broken_ctrl = os.path.join(temp_dir, "broken_control.cpp")
        with open(broken_ctrl, "w") as f:
            f.write("syntax error here !@#$%\n")

        bad_manifest = json.loads(json.dumps(valid_manifest))
        bad_manifest["negative_compile_probes"] = [{
            "id": "NEG-BROKEN-CTRL",
            "file": "tools/api_surface/negative_probes/result_same_type.cpp",
            "control_file": os.path.relpath(broken_ctrl, repo_root),
            "description": "Control with syntax error",
            "expected_failure_pattern": "cannot be the same type"
        }]
        mf_path = os.path.join(temp_dir, "manifest.json")
        with open(mf_path, "w") as f:
            json.dump(bad_manifest, f)

        res = run_gate(["--manifest", mf_path, "--skip-build-exec"])
        assert res.returncode != 0, "Gate must fail if negative probe control fails to compile"
        assert "control probe failed to compile" in res.stderr
    print("  [TEST 12/17] Broken negative-probe control ... PASS")

def test_valid_controlled_pass(build_dir):
    res = run_gate(["--build-dir", build_dir])
    assert res.returncode == 0, f"Valid gate run failed: {res.stderr}\n{res.stdout}"
    assert "RESULT: PASS" in res.stdout
    print("  [TEST 13/17] Minimal valid controlled PASS ... PASS")


def test_untracked_extra_header(repo_root, valid_manifest):
    with tempfile.TemporaryDirectory() as temp_dir:
        shutil.copytree(os.path.join(repo_root, "modules"), os.path.join(temp_dir, "modules"))
        subprocess.run(["git", "init"], cwd=temp_dir, check=True, capture_output=True)
        subprocess.run(["git", "add", "."], cwd=temp_dir, check=True, capture_output=True)
        extra = os.path.join(temp_dir, "modules", "VectorisNumerics", "include", "Vectoris", "Numerics", "Core", "UndeclaredNamespace.h")
        with open(extra, "w") as f:
            f.write("#pragma once\n")
        manifest_copy = os.path.join(temp_dir, "manifest.json")
        with open(manifest_copy, "w") as f:
            json.dump(valid_manifest, f)
        res = run_gate(["--repo-root", temp_dir, "--manifest", manifest_copy, "--skip-build-exec"])
        assert res.returncode != 0, "Untracked undeclared header must fail exact inventory"
        assert "Discrepancy between Git and Manifest headers" in res.stderr
    print("  [TEST 14/17] Untracked undeclared public header ... PASS")

def test_namespace_metadata(valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    bad["public_headers"][0]["canonical_namespace"] = "vectoris::dynamics::core"
    with tempfile.TemporaryDirectory() as temp_dir:
        path = os.path.join(temp_dir, "manifest.json")
        with open(path, "w") as f:
            json.dump(bad, f)
        res = run_gate(["--manifest", path, "--skip-build-exec"])
        assert res.returncode != 0
        assert "Invalid namespace contract" in res.stderr
    print("  [TEST 15/17] Wrong canonical namespace metadata ... PASS")

def test_missing_namespace_probe(valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    bad["public_headers"][0]["namespace_probe"] = "nonexistent_namespace_probe.inc"
    with tempfile.TemporaryDirectory() as temp_dir:
        path = os.path.join(temp_dir, "manifest.json")
        with open(path, "w") as f:
            json.dump(bad, f)
        res = run_gate(["--manifest", path, "--skip-build-exec"])
        assert res.returncode != 0
        assert "Namespace probe does not exist" in res.stderr
    print("  [TEST 16/17] Missing direct namespace probe ... PASS")

def test_missing_dynamics_api_probe(valid_manifest):
    bad = json.loads(json.dumps(valid_manifest))
    bad["dynamics_public_api_contracts"][0]["positive_test"] = "DynamicsUnitsTest.MissingRotationalCrossProbe"
    with tempfile.TemporaryDirectory() as temp_dir:
        path = os.path.join(temp_dir, "manifest.json")
        with open(path, "w") as f:
            json.dump(bad, f)
        res = run_gate(["--manifest", path, "--skip-build-exec"])
        assert res.returncode != 0
        assert "non-existent Dynamics API probe" in res.stderr
    print("  [TEST 17/17] Missing Dynamics semantic API probe ... PASS")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True)
    args = parser.parse_args()
    print("=" * 80)
    print("Vectoris Public API Surface Gate Self-Test Suite (VRT-10D)")
    print("=" * 80)

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    manifest_path = os.path.join(repo_root, "tools", "api_surface", "public_api_manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as f:
        valid_manifest = json.load(f)

    test_missing_manifest()
    test_empty_manifest()
    test_malformed_manifest()
    test_duplicate_header(repo_root, valid_manifest)
    test_missing_header_from_manifest(repo_root, valid_manifest)
    test_nonexistent_probe_reference(repo_root, valid_manifest)
    test_zero_probes_for_runtime_template(repo_root, valid_manifest)
    test_missing_positive_suite(repo_root, valid_manifest)
    test_zero_discovered_positive_tests(repo_root, valid_manifest)
    test_failing_positive_api_test(repo_root)
    test_negative_probe_unexpectedly_accepted(repo_root, valid_manifest)
    test_broken_negative_probe_control(repo_root, valid_manifest)
    test_valid_controlled_pass(args.build_dir)
    test_untracked_extra_header(repo_root, valid_manifest)
    test_namespace_metadata(valid_manifest)
    test_missing_namespace_probe(valid_manifest)
    test_missing_dynamics_api_probe(valid_manifest)

    print("=" * 80)
    subprocess.run([sys.executable, os.path.join(os.path.dirname(__file__), "test_positive_probe_contract.py")], check=True)
    print("VRT-10 legacy controls: 17 passed; AFA003 CTest identity controls: see unittest execution summary above.")
    print("=" * 80)

if __name__ == "__main__":
    main()
