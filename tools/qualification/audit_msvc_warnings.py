#!/usr/bin/env python3
"""Fail-closed qualification audit of unmodified MSVC build logs.

Native /WX is necessary but is not a log-completeness or diagnostic-ownership
check. This gate requires every build phase, retains each diagnostic verbatim,
and rejects unknown formats/ownership, errors, and any first-party warning.
Only the explicitly identified GoogleTest build is external. Project ownership
takes precedence over a dependency mentioned in a diagnostic message.

Recognition is syntactic: MSVC/MSBuild use origin : severity code : message;
CMake has its own severity/location header. None means NOT_DIAGNOSTIC, not an
ignored diagnostic. A location, tool, coded severity, annotation, or diagnostic
summary can establish diagnostic shape even when the complete grammar fails.
Such incomplete/unknown records still fail closed. Prose keywords alone cannot
establish diagnostic shape; no prose exception list is used.
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
    r"(?:Command line\s+)?"
    r"(?P<severity>fatal error|error|warning)\s+"
    r"(?P<code>C\d{4}|D\d{4}|LNK\d{4}|MSB\d{4})\s*:\s*"
    r"(?P<message>.+?)(?:\s+\[(?P<project>[^\]]+\.vcxproj)\])?\s*$",
    re.IGNORECASE,
)
CMAKE_DIAGNOSTIC = re.compile(
    r"^\s*CMake (?P<severity>Error|(?:Deprecation )?Warning)"
    r"(?:\s*\([^)]+\))?(?:\s+(?:at|in)\s+(?P<location>.+?))?"
    r"\s*:(?=\s|$)\s*(?P<message>.*)$", re.I,
)
SUMMARY = re.compile(r"^\s*(\d+)\s+(?:warnings?|errors?)(?:\(s\))?"
                     r"(?:\s+generated\.)?\s*$", re.I)
PREFIX = r"^\s*(?:\d+>)?\s*"
SEVERITY = r"(?:fatal\s+error|error|warning)"
CODE = r"[A-Z]+[0-9][A-Z0-9]*"
ORIGIN_SHAPE = (
    r"(?:.+?\(\s*\d+(?:\s*,\s*\d+)*\s*\)"
    r"|.+?\.(?:cpp|cc|cxx|c|hpp|hxx|h|obj|lib|exe|vcxproj|targets|props)\b"
    r"|(?:cl|link|lib|msbuild)(?:\.exe)?)"
)
# These recognize structure, including damaged separators or unknown code
# families. They do not search for unqualified words inside arbitrary prose.
DIAGNOSTIC_SHAPES = tuple(re.compile(pattern, re.I) for pattern in (
    rf"{PREFIX}.+?\s*:\s*(?:Command line\s+)?{SEVERITY}\s+{CODE}\b",
    rf"{PREFIX}{ORIGIN_SHAPE}\s*:\s*(?:Command line\s+)?{SEVERITY}\b",
    rf"{PREFIX}{ORIGIN_SHAPE}\s+(?:Command line\s+)?{SEVERITY}\s+{CODE}\b",
    rf"{PREFIX}{ORIGIN_SHAPE}\s*:\s*{CODE}\b",
    rf"{PREFIX}{SEVERITY}\s*(?::|\s+{CODE}\b)",
    rf"{PREFIX}{CODE}\s*:",
    r"^\s*CMake (?:Deprecation )?(?:Error|Warning)\b"
    r"(?:\s*\([^)]+\))?(?:\s+(?:at|in)\b|\s*:|\s*$)",
    r"^\s*(?:::(?:warning|error)(?: [^:]*)?::|##\[(?:warning|error)\])",
))


def normalized(path):
    return posixpath.normpath(str(path).replace("\\", "/")).casefold()


def beneath(path, root):
    return path == root or path.startswith(root + "/")


def origin_path(origin, source_root):
    # Compiler coordinates and MSBuild project-number prefixes are not path parts.
    path = re.sub(r"\(\s*\d+(?:\s*,\s*\d+)*\s*\)$", "", origin.strip()).strip().strip('"')
    path = normalized(path)
    if not path.startswith("/") and not re.match(r"^[a-z]:/", path):
        path = normalized(source_root + "/" + path)
    return path


def ownership(origin, project, source_root, build_root):
    """No classification may be inferred from the free-form message text."""
    source = origin_path(origin, source_root)
    project_path = normalized(project) if project else ""
    first_source = source == source_root + "/cmakelists.txt" or any(
        beneath(source, source_root + "/" + part)
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


def linker_role(origin, project):
    producer = posixpath.basename(origin.replace("\\", "/")).casefold().strip()
    if producer in ("link", "link.exe"):
        return "LINKER"
    if producer in ("lib", "lib.exe"):
        return "LIBRARIAN"
    project_name = posixpath.basename((project or "").replace("\\", "/"))[:-8]
    return PROJECT_TO_TOOL.get(project_name)


def classify_msvc(data, line, source_root, build_root):
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
        else:
            tool = linker_role(data["origin"], data["project"])
            if tool:
                category = "FIRST_PARTY_" + tool + "_WARNING"
    if "error" in data["severity"].lower():
        category = "BUILD_ERROR"
    return {"category": category, "severity": data["severity"].lower(),
            "code": code, "raw": line}


def classify_cmake(data, line, source_root, build_root):
    location = re.sub(r"\s+\([^)]*\)$", "", data["location"] or "")
    location = re.sub(r":\d+$", "", location)
    owner = ownership(location, None, source_root, build_root)
    category = {"FIRST_PARTY": "FIRST_PARTY_BUILD_SYSTEM_WARNING",
                "THIRD_PARTY": "THIRD_PARTY_WARNING"}.get(owner, "UNKNOWN_DIAGNOSTIC")
    severity = "error" if data["severity"].casefold() == "error" else "warning"
    if severity == "error":
        category = "BUILD_ERROR"
    return {"category": category, "severity": severity, "code": "CMAKE", "raw": line}


def classify(line, source_root, build_root):
    summary = SUMMARY.fullmatch(line)
    if summary and int(summary[1]) == 0:
        return None
    if platform_annotation(line):
        return {"category": "PLATFORM_ANNOTATION", "severity": "annotation", "raw": line}
    match = DIAGNOSTIC.fullmatch(line)
    if match:
        return classify_msvc(match.groupdict(), line, source_root, build_root)
    match = CMAKE_DIAGNOSTIC.fullmatch(line)
    if match:
        return classify_cmake(match.groupdict(), line, source_root, build_root)
    if summary or any(shape.match(line) for shape in DIAGNOSTIC_SHAPES):
        return {"category": "UNKNOWN_DIAGNOSTIC", "severity": "unknown", "raw": line}
    return None


def completed_target(text, target):
    escaped = re.escape(target)
    return bool(re.search(
        rf"(?:Built target {escaped}\b|{escaped}\.vcxproj\s*->|"
        rf"VECTORIS_HEADER_ISOLATION_COMPLETE {escaped} [1-9]\d*)", text))


def validate_completion(filename, text):
    if not text.strip() or re.search(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]", text):
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
