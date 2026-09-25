#!/usr/bin/env python3
"""Fail-closed qualification audit of unmodified MSVC build logs.

Native /WX is necessary but is not a log-completeness or diagnostic-ownership
check. This gate requires every build phase, retains each diagnostic verbatim,
and rejects unknown formats/ownership, errors, and any first-party warning.
Only the explicitly identified GoogleTest build is external. Project ownership
takes precedence over a dependency mentioned in a diagnostic message.
"""

import argparse
import json
import posixpath
import re
import sys
from collections import Counter
from pathlib import Path


EXPECTED_LOGS = (
    "configure.log", "build-debug.log", "header-isolation.log", "build-release.log"
)
PROJECT_TO_TOOL = {
    "VectorisNumerics_Tests": "LINKER",
    "VectorisDynamics_Tests": "LINKER",
    "VectorisNumerics_HeaderIsolation": "LIBRARIAN",
    "VectorisDynamics_HeaderIsolation": "LIBRARIAN",
    "VectorisNumerics_HeaderIsolation_Objects": "LIBRARIAN",
    "VectorisDynamics_HeaderIsolation_Objects": "LIBRARIAN",
}
CATEGORIES = (
    "FIRST_PARTY_COMPILER_WARNING", "FIRST_PARTY_LINKER_WARNING",
    "FIRST_PARTY_LIBRARIAN_WARNING", "FIRST_PARTY_BUILD_SYSTEM_WARNING",
    "THIRD_PARTY_WARNING", "PLATFORM_ANNOTATION", "UNKNOWN_DIAGNOSTIC", "BUILD_ERROR"
)
DIAGNOSTIC = re.compile(
    r"^\s*(?:\d+>)?(?P<origin>.+?)\s*:\s*"
    r"(?P<severity>fatal error|error|warning)\s+"
    r"(?P<code>C\d{4}|D\d{4}|LNK\d{4}|MSB\d{4})\s*:\s*"
    r"(?P<message>.+?)(?:\s+\[(?P<project>[^\]]+\.vcxproj)\])?\s*$",
    re.IGNORECASE,
)
SIGNAL = re.compile(r"\b(?:warnings?|errors?)\b|\b(?:LNK|MSB|C)\d{4}\s*:", re.I)


def normalized(path):
    return posixpath.normpath(str(path).replace("\\", "/")).casefold()


def beneath(path, root):
    return path == root or path.startswith(root + "/")


def origin_path(origin, source_root):
    # Compiler coordinates and MSBuild project-number prefixes are not path parts.
    path = re.sub(r"\(\d+(?:,\d+)*\)$", "", origin.strip())
    path = normalized(path)
    if not path.startswith("/") and not re.match(r"^[a-z]:/", path):
        path = normalized(source_root + "/" + path)
    return path


def ownership(origin, project, source_root, build_root):
    """No classification may be inferred from the free-form message text."""
    source = origin_path(origin, source_root)
    project_path = normalized(project) if project else ""
    first_source = any(beneath(source, source_root + "/" + part)
                       for part in ("modules", "cmake", "tools", "benchmarks"))
    first_project = (
        beneath(project_path, build_root + "/modules/vectorisnumerics") or
        beneath(project_path, build_root + "/modules/vectorisdynamics")
    )
    if first_source or first_project:
        return "FIRST_PARTY"
    dependency_roots = (
        build_root + "/_deps/googletest-src",
        build_root + "/_deps/googletest-build",
    )
    if any(beneath(source, d) or beneath(project_path, d) for d in dependency_roots):
        return "THIRD_PARTY"
    return "UNKNOWN"


def platform_annotation(line):
    # A recognized transport alone must never relabel arbitrary build warnings.
    match = re.match(r"^(?:::warning(?: [^:]*)?::|##\[warning\])(.+)$", line)
    if not match:
        return False
    message = match[1]
    return (
        not re.search(r"Vectoris|\.vcxproj|\b(?:C|LNK|MSB)\d{4}", message, re.I)
        and (message.startswith("Node.js 20 is deprecated.")
             or message.startswith("The ubuntu-latest label will migrate"))
    )


def classify(line, source_root, build_root):
    if re.fullmatch(r"\s*0 (?:Warning|Error)\(s\)\s*", line, re.I):
        return None
    if platform_annotation(line):
        return {"category": "PLATFORM_ANNOTATION", "severity": "annotation", "raw": line}
    if not SIGNAL.search(line):
        return None
    match = DIAGNOSTIC.fullmatch(line)
    if not match:
        return {"category": "UNKNOWN_DIAGNOSTIC", "severity": "unknown", "raw": line}
    data = match.groupdict()
    owner = ownership(data["origin"], data["project"], source_root, build_root)
    code = data["code"].upper()
    category = "UNKNOWN_DIAGNOSTIC"
    if owner == "THIRD_PARTY":
        category = "THIRD_PARTY_WARNING"
    elif owner == "FIRST_PARTY":
        if code.startswith(("C", "D")):
            category = "FIRST_PARTY_COMPILER_WARNING"
        elif code.startswith("MSB"):
            category = "FIRST_PARTY_BUILD_SYSTEM_WARNING"
        elif data["project"]:
            project_name = posixpath.basename(data["project"].replace("\\", "/"))[:-8]
            tool = PROJECT_TO_TOOL.get(project_name)
            if tool:
                category = "FIRST_PARTY_" + tool + "_WARNING"
    if "error" in data["severity"].lower():
        category = "BUILD_ERROR"
    return {"category": category, "severity": data["severity"].lower(),
            "code": code, "raw": line}


def completed_target(text, target):
    escaped = re.escape(target)
    return bool(re.search(
        rf"(?:Built target {escaped}\b|{escaped}\.vcxproj\s*->|"
        rf"VECTORIS_HEADER_ISOLATION_COMPLETE {escaped} [1-9]\d*)", text))


def validate_completion(filename, text):
    if not text.strip() or "\0" in text:
        raise ValueError(f"{filename}: empty or malformed log")
    if filename == "configure.log":
        if "Build files have been written to:" not in text:
            raise ValueError("configure.log: missing completed configure evidence")
        return
    suffix = "HeaderIsolation" if filename == "header-isolation.log" else "Tests"
    for module in ("Numerics", "Dynamics"):
        target = f"Vectoris{module}_{suffix}"
        if not completed_target(text, target):
            raise ValueError(f"{filename}: missing completed target {target}")


def audit_directory(log_dir, source_root, build_root):
    source_root, build_root = normalized(source_root), normalized(build_root)
    if source_root in ("", ".") or build_root in ("", "."):
        raise ValueError("Explicit source/build roots are required")
    diagnostics = []
    for filename in EXPECTED_LOGS:
        text = (Path(log_dir) / filename).read_text(encoding="utf-8-sig", errors="strict")
        validate_completion(filename, text)
        for number, line in enumerate(text.splitlines(), 1):
            diagnostic = classify(line, source_root, build_root)
            if diagnostic:
                diagnostic.update({"log": filename, "line": number})
                diagnostics.append(diagnostic)
    counts = Counter(d["category"] for d in diagnostics)
    failed = any(
        d["category"].startswith("FIRST_PARTY_")
        or d["category"] == "UNKNOWN_DIAGNOSTIC"
        or "error" in d["severity"] for d in diagnostics
    )
    return {"status": "FAIL" if failed else "PASS", "logs": list(EXPECTED_LOGS),
            "counts": {name: counts[name] for name in CATEGORIES},
            "diagnostics": diagnostics}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log-dir", required=True, type=Path)
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--build-root", required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit_directory(args.log_dir, args.source_root, args.build_root)
    except Exception as error:
        report = {"status": "FAIL", "failure": "AUDIT_EXCEPTION",
                  "detail": f"{type(error).__name__}: {error}"}
    rendered = json.dumps(report, indent=2)
    print(rendered)
    if args.output:
        try:
            args.output.write_text(rendered + "\n", encoding="utf-8")
        except Exception as error:
            print(f"FAIL: cannot preserve audit report: {error}", file=sys.stderr)
            return 1
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
