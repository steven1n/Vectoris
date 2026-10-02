"""AFA-003: exact positive probe identities, independently obtained at each stage."""
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path
import xml.etree.ElementTree as ET

PREFIX = "PublicApiSurfaceTest."

def require_equal(required, actual, stage):
    """Keep lists until duplicates have been rejected; equal counts are insufficient."""
    if not required or not isinstance(actual, list) or any(not isinstance(x, str) for x in actual):
        raise ValueError(f"{stage}: invalid or empty identity contract")
    if len(actual) != len(set(actual)):
        raise ValueError(f"{stage}: duplicate identities")
    missing, unexpected = set(required) - set(actual), set(actual) - set(required)
    if missing or unexpected:
        raise ValueError(f"{stage}: missing={sorted(missing)}, unexpected={sorted(unexpected)}")

def source_identities(source):
    # GTest declarations, not comments. All public API probes use plain TEST.
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    return [PREFIX + name for name in re.findall(r'TEST\s*\(\s*PublicApiSurfaceTest\s*,\s*(\w+)\s*\)', source)]

def verify_source(required, source):
    require_equal(required, required, "REQUIRED")
    found = source_identities(source)
    require_equal(required, found, "SOURCE_DEFINED")
    return found

def gtest_execution(report, required):
    """Read executed body identities, never CTest wrapper labels or counts alone."""
    if not report.is_file():
        raise ValueError("EXECUTED: missing fresh GoogleTest body report")
    try:
        root = ET.parse(report).getroot()
    except ET.ParseError as exc:
        raise ValueError("EXECUTED: malformed GoogleTest report") from exc
    if root.tag != "testsuites":
        raise ValueError("EXECUTED: invalid GoogleTest report root")
    tests = root.findall(".//testcase")
    defined = [test.attrib.get("classname", "") + "." + test.attrib.get("name", "")
               for test in tests]
    require_equal(required, defined, "GTEST_DEFINED")
    executed = [identity for identity, test in zip(defined, tests)
                if test.attrib.get("status") == "run"
                and test.attrib.get("result") == "completed"
                and test.find("skipped") is None]
    passed = [identity for identity, test in zip(defined, tests)
              if identity in executed and test.find("failure") is None
              and test.find("error") is None]
    require_equal(required, executed, "EXECUTED")
    require_equal(required, passed, "PASSED")
    return executed, passed


def verify_ctest(build_dir, required, config=None, selection=r"^PublicApiSurfaceTest\."):
    """Require CTest success AND fresh, exact GoogleTest body execution evidence."""
    command = ["ctest", "--test-dir", str(build_dir)]
    if config:
        command += ["-C", config]
    query = subprocess.run(command + ["-R", selection, "--show-only=json-v1"],
                           capture_output=True, text=True)
    if query.returncode:
        raise ValueError("REGISTERED: CTest query failed: " + query.stderr)
    registrations = json.loads(query.stdout)["tests"]
    registered = [test["name"] for test in registrations]
    require_equal(required, registered, "REGISTERED")
    executed, passed = [], []
    with tempfile.TemporaryDirectory(prefix="vectoris-api-results-") as tmp:
        for index, test in enumerate(registrations):
            name = test["name"]
            if any(arg.startswith("--gtest_output") for arg in test.get("command", [])):
                raise ValueError("EXECUTED: registration overrides fresh GoogleTest report")
            body_report = Path(tmp) / f"body-{index}.xml"
            junit = Path(tmp) / f"ctest-{index}.xml"
            env = dict(os.environ, GTEST_OUTPUT="xml:" + str(body_report))
            run = subprocess.run(command + ["-R", "^" + re.escape(name) + "$",
                                 "--output-on-failure", "--output-junit", str(junit)],
                                 capture_output=True, text=True, env=env)
            print(run.stdout, end="")
            if run.returncode:
                raise ValueError("EXECUTED: CTest failure: " + run.stderr)
            if not junit.is_file():
                raise ValueError("EXECUTED: missing CTest JUnit report")
            try:
                wrappers = ET.parse(junit).getroot().findall(".//testcase")
            except ET.ParseError as exc:
                raise ValueError("EXECUTED: malformed CTest report") from exc
            completed = [case.attrib.get("name") for case in wrappers
                         if case.find("skipped") is None and case.find("failure") is None
                         and case.find("error") is None
                         and case.attrib.get("status") in (None, "run")]
            require_equal([name], completed, "EXECUTED")
            actual, successful = gtest_execution(body_report, [name])
            executed.extend(actual)
            passed.extend(successful)
    require_equal(required, executed, "EXECUTED")
    require_equal(required, passed, "PASSED")
    return {"REGISTERED": sorted(registered), "EXECUTED": sorted(executed), "PASSED": sorted(passed)}
