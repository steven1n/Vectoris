"""AFA-003: exact positive probe identities, independently obtained at each stage."""
import json
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

def verify_ctest(build_dir, required, config=None):
    """Query CTest JSON registration, then compare its real JUnit execution records."""
    command = ["ctest", "--test-dir", str(build_dir)]
    if config:
        command += ["-C", config]
    selection = ["-R", r"^PublicApiSurfaceTest\."]
    query = subprocess.run(command + selection + ["--show-only=json-v1"], capture_output=True, text=True)
    if query.returncode:
        raise ValueError("REGISTERED: CTest query failed: " + query.stderr)
    registered = [test["name"] for test in json.loads(query.stdout)["tests"]]
    require_equal(required, registered, "REGISTERED")
    with tempfile.TemporaryDirectory(prefix="vectoris-api-results-") as tmp:
        junit = Path(tmp) / "execution.xml"
        run = subprocess.run(command + selection + ["--output-on-failure", "--output-junit", str(junit)],
                             capture_output=True, text=True)
        print(run.stdout, end="")
        if run.returncode:
            raise ValueError("EXECUTED: CTest failure: " + run.stderr)
        if not junit.is_file():
            raise ValueError("EXECUTED: missing CTest JUnit report")
        tests = ET.parse(junit).getroot().findall(".//testcase")
        executed = [test.attrib["name"] for test in tests
                    if test.find("skipped") is None and test.attrib.get("status") in (None, "run")]
        passed = [test.attrib["name"] for test in tests
                  if test.attrib["name"] in executed and test.find("failure") is None
                  and test.find("error") is None and test.attrib.get("status") in (None, "run")]
        require_equal(required, executed, "EXECUTED")
        require_equal(required, passed, "PASSED")
    return {"REGISTERED": sorted(registered), "EXECUTED": sorted(executed), "PASSED": sorted(passed)}
