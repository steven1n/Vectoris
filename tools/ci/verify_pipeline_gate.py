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

def workflow_run_blocks(content):
    """Use YAML structure, including inherited shells; no indentation/EOF scanner."""
    import yaml

    class UniqueLoader(yaml.SafeLoader):
        pass

    def unique_mapping(loader, node):
        result = {}
        for key_node, value_node in node.value:
            key = loader.construct_object(key_node)
            if key in result:
                raise ValueError(f"duplicate YAML key: {key}")
            result[key] = loader.construct_object(value_node)
        return result

    UniqueLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, unique_mapping)
    workflow = yaml.load(content, Loader=UniqueLoader)
    blocks = []
    for job_name, job in workflow["jobs"].items():
        if job.get("continue-on-error", False) is not False:
            raise ValueError("qualification jobs may not continue on error")
        inherited = job.get("defaults", workflow.get("defaults", {})).get("run", {}).get("shell")
        for index, step in enumerate(job.get("steps", [])):
            if "run" not in step:
                continue
            if step.get("continue-on-error", False) is not False:
                raise ValueError("qualification run steps may not continue on error")
            shell = step.get("shell", inherited)
            if shell is None:
                runner = job.get("runs-on", "")
                if not isinstance(runner, str) or "${{" in runner:
                    raise ValueError("run block requires an explicit shell for dynamic runner")
                shell = "pwsh" if "windows" in runner.lower() else "bash"
            if shell not in ("bash", "pwsh") or not isinstance(step["run"], str):
                raise ValueError(f"unsupported shell/run structure: {shell}")
            blocks.append((f"{job_name}/{index}:{step.get('name', 'run')}", shell, step["run"]))
    if not blocks:
        raise ValueError("no run blocks found")
    return blocks


def audit_bash(script):
    # Accepted qualification dialect: explicit fail-fast prologue, never disabled.
    # Quotes/comments are tokenized, so prose mentioning pipefail cannot pass.
    import shlex
    lines = [line for line in script.splitlines() if line.strip() and not line.lstrip().startswith("#")]
    first = shlex.split(lines[0], comments=True) if lines else []
    if first not in (["set", "-euo", "pipefail"], ["set", "-eo", "pipefail"]):
        raise ValueError("bash requires active set -e[u]o pipefail before commands")
    lexer = shlex.shlex(script, posix=True, punctuation_chars=";&|\n")
    lexer.whitespace = " \t\r"
    lexer.whitespace_split = True
    tokens = []
    for token in lexer:
        # shlex groups punctuation runs (including '&\n'). Keep shell control
        # operators separate from line boundaries so the final command is audited.
        tokens.extend(re.findall(r"\n|&&|\|\||[;&|]", token)
                      if re.fullmatch(r"[;&|\n]+", token) else [token])
    for index, token in enumerate(tokens):
        if token == "set" and index + 1 < len(tokens) and tokens[index + 1].startswith("+"):
            raise ValueError("bash failure policy may not be disabled")
        fd_redirect = (token == "&" and index > 0 and index + 1 < len(tokens) and
                       re.fullmatch(r"\d*>", tokens[index - 1]) and tokens[index + 1].isdigit())
        if (token in ("||", "&&") or (token == "&" and not fd_redirect) or
                any(part in token for part in ("$(", "`", "(", ")")) or
                token in ("eval", "source", "bash", "sh", "pwsh", "if", "while", "until", "!",
                          "trap", "function", "for", "case", "coproc", "builtin", "command") or
                (token == "." and (index == 0 or tokens[index - 1] in (";", "|", "\n")))):
            raise ValueError("unsupported dynamic or failure-masking bash construct: " + token)


def audit_pwsh_metadata(line):
    # The workflow's here-strings may interpolate data and these read-only
    # expressions. Arbitrary subexpressions could launch an unchecked native tool.
    pure_expressions = (
        r"\$\(\[System\.Environment\]::(?:OSVersion\.ToString\(\)|Is64BitProcess)\)",
        r"\$\(\(Get-Item \(Get-Command cl\.exe\)\.Source\)\.VersionInfo\.FileVersion\)",
        r"\$\(\$\w+ -replace '[^']*', '[^']*'\)",
    )
    for expression in pure_expressions:
        line = re.sub(expression, "", line)
    if "$(" in line or "`" in line:
        raise ValueError("unsupported executable metadata interpolation")


def audit_pwsh_auxiliary(command, check):
    # Deliberately bounded workflow dialect. Unrecognized statements are rejected,
    # not presumed to be cmdlets; an extension must add a reviewed positive control.
    if command == check:
        return
    if any(token in command for token in ("&", ";", "$(", "`")):
        raise ValueError("embedded command execution is outside the audited dialect")
    if re.fullmatch(r'if \(\[string\]::IsNullOrWhiteSpace\(\$\w+\)\) \{ throw "[^"\n]*" \}', command):
        return
    segments = command.split("|")
    first = segments[0].strip()
    if not re.match(r"^(?:New-Item|Write-Host|Add-Content)(?:\s|$)", first):
        if not re.fullmatch(r"\$\w+\s*=\s*Get-Content\s+[\w./-]+", first):
            if not re.fullmatch(r"\$\w+(?:\s*=\s*\$\w+)?", first):
                raise ValueError("unsupported PowerShell statement: " + command)
    for segment in segments[1:]:
        segment = segment.strip()
        if re.fullmatch(r"Where-Object \{ \$_ -match '[^']*' \}", segment):
            continue
        if re.fullmatch(r"Select-Object -First \d+", segment):
            continue
        if re.match(r"^(?:Out-File|Tee-Object|Out-String)(?:\s|$)", segment):
            continue
        raise ValueError("unsupported PowerShell auxiliary pipeline: " + segment)


def audit_pwsh(script):
    # One policy: every native invocation is a standalone & command followed
    # immediately by the tested exit-code check. No delayed/shared checks.
    check = "if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }"
    if re.search(r"\b(?:Invoke-Expression|iex|Start-Process|Invoke-Command)\b", script, re.I):
        raise ValueError("dynamic PowerShell process execution is outside the audited dialect")
    logical = []
    pending = ""
    in_here = False
    for raw in script.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if in_here:
            if line in ('"@', "'@"):
                in_here = False
            else:
                audit_pwsh_metadata(line)
            continue
        if line.endswith(('@"', "@'")):
            in_here = True
            continue
        if line.endswith("`"):
            pending += line[:-1] + " "
            continue
        logical.append(pending + line)
        pending = ""
    if pending or in_here:
        raise ValueError("unterminated PowerShell continuation/here-string")
    for index, command in enumerate(logical):
        if command.startswith("& "):
            if ";" in command or "&&" in command or "||" in command or "$(" in command:
                raise ValueError("native invocations must be separate commands")
            syntax = re.sub(r"\"[^\"\n]*\"|'(?:''|[^'])*'", "", command)
            syntax = re.sub(r"\*?>?&\d+", "", syntax[1:])
            if any(token in syntax for token in ("&", "(", ")", "{", "}")):
                raise ValueError("native arguments may not contain embedded executable expressions")
            # Only output cmdlets may follow a native command in a pipeline.
            # A second native process could overwrite LASTEXITCODE even within one line.
            for segment in command.split("|")[1:]:
                if not re.match(r"^\s*(?:Tee-Object|Out-String|Out-File)(?:\s|$)", segment, re.I):
                    raise ValueError("native pipeline may contain only approved output cmdlets")
            if index + 1 == len(logical) or logical[index + 1] != check:
                raise ValueError("native command lacks an immediate LASTEXITCODE check: " + command)
        else:
            audit_pwsh_auxiliary(command, check)


def audit_workflow_file(workflow_path):
    print(f"Auditing Workflow: {workflow_path}")
    try:
        with open(workflow_path, encoding="utf-8") as source:
            blocks = workflow_run_blocks(source.read())
        failures = []
        for name, shell, script in blocks:
            try:
                (audit_pwsh if shell == "pwsh" else audit_bash)(script)
            except (ValueError, IndexError) as exc:
                failures.append(f"{name}: {exc}")
        for failure in failures:
            print("ERROR: " + failure)
        print(f"Run blocks audited: {len(blocks)}; failed: {len(failures)}")
        print("Workflow Audit: " + ("FAIL" if failures else "PASS"))
        return not failures
    except Exception as exc:
        print(f"ERROR: workflow audit could not complete: {type(exc).__name__}: {exc}")
        return False

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    wf_path = os.path.join(repo_root, ".github", "workflows", "cross-compiler-qualification.yml")

    selftests = subprocess.run([sys.executable, os.path.join(repo_root, "tools", "ci", "test_pipeline_contract.py")])
    linux_ok = test_linux_bash_pipefail()
    audit_ok = audit_workflow_file(wf_path)

    if selftests.returncode == 0 and linux_ok and audit_ok:
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
