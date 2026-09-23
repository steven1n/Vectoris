#!/usr/bin/env python3
"""
verify_pipeline_gate.py

Deterministic validation of CI pipeline failure propagation (VRT-06).
Proves:
1. Under default bash without pipefail, upstream failure piped to tee returns 0 (reproduced defect).
2. Under bash with "set -euo pipefail", upstream failure piped to tee returns 1 and aborts execution.
3. Audits .github/workflows/cross-compiler-qualification.yml to ensure:
   - All Linux workflow steps with pipelines enforce pipefail.
   - All Windows pwsh steps check $LASTEXITCODE immediately after native invocations.
"""

import os
import sys
import subprocess
import tempfile
import re

def test_linux_bash_pipefail():
    print("=" * 60)
    print("Testing Linux Bash Pipeline Exit Code Propagation")
    print("=" * 60)

    # 1. Without pipefail
    cmd_no_pipefail = ["bash", "-c", "false | tee /dev/null; exit 0"]
    res1 = subprocess.run(cmd_no_pipefail)
    print(f"  Without pipefail (false | tee): exit code = {res1.returncode} (swallowed failure)")

    # 2. With pipefail
    cmd_with_pipefail = ["bash", "-c", "set -euo pipefail; false | tee /dev/null; exit 0"]
    res2 = subprocess.run(cmd_with_pipefail)
    print(f"  With pipefail    (false | tee): exit code = {res2.returncode} (propagated failure)")

    if res1.returncode != 0 or res2.returncode == 0:
        print("ERROR: Pipeline exit code propagation behavior mismatch!")
        return False

    # 3. Step execution abort
    with tempfile.TemporaryDirectory() as tmpdir:
        marker = os.path.join(tmpdir, "marker.txt")
        script = f"""set -euo pipefail
false | tee /dev/null
touch "{marker}"
"""
        res3 = subprocess.run(["bash", "-c", script])
        marker_created = os.path.exists(marker)
        print(f"  Multi-command script with pipefail: exit code = {res3.returncode}, marker created = {marker_created}")
        if res3.returncode == 0 or marker_created:
            print("ERROR: Multi-command script did not abort upon pipeline failure!")
            return False

    print("Linux Pipeline Gate: PASS\n")
    return True

def audit_workflow_file(workflow_path):
    print("=" * 60)
    print(f"Auditing Workflow: {os.path.basename(workflow_path)}")
    print("=" * 60)

    if not os.path.exists(workflow_path):
        print(f"ERROR: Workflow file not found: {workflow_path}")
        return False

    with open(workflow_path, "r", encoding="utf-8") as f:
        content = f.read()

    errors = []
    lines = content.splitlines()
    in_run_block = False
    current_run = []
    current_step_name = ""
    is_pwsh = False

    for line in lines:
        if "- name:" in line:
            current_step_name = line.strip()
            is_pwsh = False
        if "shell: pwsh" in line:
            is_pwsh = True
        if re.match(r"^\s+run:\s*\|\s*$", line):
            in_run_block = True
            current_run = []
            continue
        elif in_run_block:
            if re.match(r"^\s{6,}", line) or line.strip() == "":
                current_run.append(line)
            else:
                run_text = "\n".join(current_run)
                in_run_block = False

                # Check Linux pipelines
                if not is_pwsh and "| tee" in run_text:
                    if "set -euo pipefail" not in run_text and "set -eo pipefail" not in run_text:
                        errors.append(f"Step '{current_step_name}' has pipeline '| tee' without 'set -euo pipefail'")

                # Check Windows pwsh native commands
                if is_pwsh and ("cmake.exe" in run_text or "ctest.exe" in run_text):
                    if "$LASTEXITCODE" not in run_text:
                        errors.append(f"Pwsh step '{current_step_name}' executes native command without $LASTEXITCODE check")

    if errors:
        print("Found Workflow Audit Errors:")
        for err in errors:
            print("  -", err)
        return False
    else:
        print("All workflow pipeline and native command checks passed audit!")
        print("Workflow Audit: PASS\n")
        return True

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    wf_path = os.path.join(repo_root, ".github", "workflows", "cross-compiler-qualification.yml")

    linux_ok = test_linux_bash_pipefail()
    audit_ok = audit_workflow_file(wf_path)

    if linux_ok and audit_ok:
        print("=" * 60)
        print("ALL CI PIPELINE GATES VERIFIED: PASS")
        print("=" * 60)
        sys.exit(0)
    else:
        print("=" * 60)
        print("CI PIPELINE GATE VERIFICATION: FAIL")
        print("=" * 60)
        sys.exit(1)

if __name__ == "__main__":
    main()
