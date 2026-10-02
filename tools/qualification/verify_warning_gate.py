#!/usr/bin/env python3
"""Deterministic positive/negative controls for the zero-warning audit."""

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import audit_msvc_warnings as gate
import verify_msvc_warning_policy as native


SOURCE = "D:/a/Vectoris/Vectoris"
BUILD = SOURCE + "/build-msvc"
NUMERICS = BUILD + "/modules/VectorisNumerics/"
DEPS = BUILD + "/_deps/googletest-build/"
COMPILER = (
    SOURCE + "/modules/VectorisNumerics/tests/Probe.cpp(7,9): "
    "warning C4101: 'unused': unreferenced local variable "
    "[" + NUMERICS + "VectorisNumerics_Tests.vcxproj]"
)
LINKER = (
    "LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:REF' specification "
    "[" + NUMERICS + "VectorisNumerics_Tests.vcxproj]"
)
LIBRARIAN = (Path(__file__).parent / "fixtures/candidate5-lnk4006.txt").read_text().strip()
FIXTURES = Path(__file__).parent / "fixtures"
C4244 = (FIXTURES / "candidate2-c4244.txt").read_text().strip()
C2220 = (FIXTURES / "candidate2-c2220.txt").read_text().strip()
EXTERNAL = (
    BUILD + "/_deps/googletest-src/googletest/src/gtest-all.cc(1): "
    "warning C4101: unused [" + DEPS + "googletest/gtest.vcxproj]"
)


class WarningGateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.logs = Path(self.temp.name)
        self.clean = {
            "configure.log": "-- Build files have been written to: " + BUILD,
            "build-debug.log": (
                "VectorisNumerics_Tests.vcxproj -> tests.exe\n"
                "VectorisDynamics_Tests.vcxproj -> dynamics.exe"),
            "build-release.log": (
                "VectorisNumerics_Tests.vcxproj -> tests.exe\n"
                "VectorisDynamics_Tests.vcxproj -> dynamics.exe"),
            "header-isolation.log": (
                "VECTORIS_HEADER_ISOLATION_COMPLETE VectorisNumerics_HeaderIsolation 60\n"
                "VECTORIS_HEADER_ISOLATION_COMPLETE VectorisDynamics_HeaderIsolation 13"),
        }
        for name, text in self.clean.items():
            (self.logs / name).write_text(text + "\n", encoding="utf-8")

    def audit(self, diagnostic="", filename="build-debug.log"):
        with (self.logs / filename).open("a", encoding="utf-8") as stream:
            stream.write(diagnostic + "\n")
        return gate.audit_directory(self.logs, SOURCE, BUILD)

    def expect_failure(self, diagnostic, category):
        result = self.audit(diagnostic)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["counts"][category], 1)
        self.assertEqual(result["diagnostics"][0]["raw"], diagnostic)

    def test_clean_positive_control(self):
        self.assertEqual(self.audit()["status"], "PASS")

    def test_first_party_compiler_warning(self):
        self.expect_failure(COMPILER, "FIRST_PARTY_COMPILER_WARNING")

    def test_c12_rotation_relative_c4702_is_first_party(self):
        line = ("modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/RotationMatrix3.h(159,1): "
                "warning C4702: unreachable code [build-msvc/modules/VectorisNumerics/VectorisNumerics_Tests.vcxproj]")
        self.expect_failure(line, "FIRST_PARTY_COMPILER_WARNING")

    def test_c12_rotation_c2220_fails_closed(self):
        line = (SOURCE + "/modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/RotationMatrix3.h(159,1): "
                "error C2220: the following warning is treated as an error [" + NUMERICS + "VectorisNumerics_Tests.vcxproj]")
        self.expect_failure(line, "BUILD_ERROR")

    def test_first_party_linker_warning(self):
        self.expect_failure(LINKER, "FIRST_PARTY_LINKER_WARNING")

    def test_candidate5_real_librarian_fixture(self):
        self.expect_failure(LIBRARIAN, "FIRST_PARTY_LIBRARIAN_WARNING")

    def test_third_party_warning_separate(self):
        result = self.audit(EXTERNAL)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["counts"]["THIRD_PARTY_WARNING"], 1)

    def test_relative_first_party_projects_override_dependency_header(self):
        for project in ('build-msvc/modules/VectorisNumerics/VectorisNumerics_Tests.vcxproj',
                        r'build-msvc\modules\VectorisNumerics\VectorisNumerics_Tests.vcxproj',
                        'modules/VectorisNumerics/VectorisNumerics_Tests.vcxproj',
                        'modules/VectorisDynamics/VectorisDynamics_Tests.vcxproj'):
            with self.subTest(project=project):
                text=EXTERNAL.replace(DEPS+'googletest/gtest.vcxproj', project)
                (self.logs/'build-debug.log').write_text(self.clean['build-debug.log']+'\n'+text+'\n')
                result=gate.audit_directory(self.logs, SOURCE, BUILD)
                self.assertEqual(result['status'],'FAIL')
                self.assertEqual(result['counts']['FIRST_PARTY_COMPILER_WARNING'],1)
                output=self.logs/'result.json'
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertNotEqual(gate.main(['--log-dir',str(self.logs),'--source-root',SOURCE,
                                                  '--build-root',BUILD,'--output',str(output)]),0)
                self.assertEqual(json.loads(output.read_text())['counts']['THIRD_PARTY_WARNING'],0)
    def test_relative_dependency_project_positive(self):
        for project in ('_deps/googletest-build/googletest/gtest.vcxproj',
                        'build-msvc/_deps/googletest-build/googletest/gtest.vcxproj'):
            with self.subTest(project=project):
                text=EXTERNAL.replace(DEPS+'googletest/gtest.vcxproj', project)
                (self.logs/'build-debug.log').write_text(self.clean['build-debug.log']+'\n'+text+'\n')
                self.assertEqual(gate.audit_directory(self.logs, SOURCE, BUILD)['status'],'PASS')
    def test_unknown_project_does_not_fall_back_to_dependency_header(self):
        for project in ('unknown/project.vcxproj', 'D:modules/VectorisNumerics/Test.vcxproj',
                        '../escape/test.vcxproj', 'E:/unreviewed/test.vcxproj'):
            with self.subTest(project=project):
                text=EXTERNAL.replace(DEPS+'googletest/gtest.vcxproj', project)
                (self.logs/'build-debug.log').write_text(self.clean['build-debug.log']+'\n'+text+'\n')
                result=gate.audit_directory(self.logs, SOURCE, BUILD)
                self.assertEqual(result['status'],'FAIL')
                self.assertEqual(result['counts']['UNKNOWN_DIAGNOSTIC'],1)
    def test_unknown_diagnostic_fail_closed(self):
        self.expect_failure("warning: unexpected diagnostic", "UNKNOWN_DIAGNOSTIC")

    def test_malformed_diagnostic_fail_closed(self):
        self.expect_failure(COMPILER.replace("warning C4101:", "warning C4101"),
                            "UNKNOWN_DIAGNOSTIC")

    def test_unknown_code_fail_closed(self):
        self.expect_failure(COMPILER.replace("C4101", "XYZ9000"), "UNKNOWN_DIAGNOSTIC")

    def test_missing_message_fail_closed(self):
        self.expect_failure("file.cpp(1): warning C4101:", "UNKNOWN_DIAGNOSTIC")

    def test_unclassified_vectoris_warning_fail_closed(self):
        self.expect_failure(
            "thing.obj : warning LNK4006: duplicate "
            "[" + BUILD + "/modules/VectorisNumerics/Unknown.vcxproj]",
            "UNKNOWN_DIAGNOSTIC")

    def test_external_path_in_message_cannot_reclassify(self):
        self.expect_failure(COMPILER + " from /_deps/googletest-src/ignored",
                            "FIRST_PARTY_COMPILER_WARNING")

    def test_first_party_project_owns_dependency_header_warning(self):
        text = EXTERNAL.replace(DEPS + "googletest/gtest.vcxproj",
                                NUMERICS + "VectorisNumerics_Tests.vcxproj")
        self.expect_failure(text, "FIRST_PARTY_COMPILER_WARNING")

    def test_first_party_source_overrides_external_project(self):
        text = COMPILER.replace(NUMERICS + "VectorisNumerics_Tests.vcxproj",
                                DEPS + "googletest/gtest.vcxproj")
        self.expect_failure(text, "FIRST_PARTY_COMPILER_WARNING")

    def test_lookalike_dependency_path_is_not_external(self):
        self.expect_failure(EXTERNAL.replace("/_deps/", "/not_deps/"),
                            "UNKNOWN_DIAGNOSTIC")

    def test_normalized_traversal_is_not_external(self):
        self.expect_failure(EXTERNAL.replace(
            "/_deps/googletest-src/", "/_deps/googletest-src/../../escape/").replace(
            "/_deps/googletest-build/", "/_deps/googletest-build/../../escape/"),
            "UNKNOWN_DIAGNOSTIC")

    def test_msbuild_warning_fail_closed(self):
        self.expect_failure(COMPILER.replace("C4101", "MSB9001"),
                            "FIRST_PARTY_BUILD_SYSTEM_WARNING")

    def test_known_platform_annotation_separate(self):
        result = self.audit("##[warning]Node.js 20 is deprecated. Upgrade the action.")
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["counts"]["PLATFORM_ANNOTATION"], 1)

    def test_platform_transport_does_not_hide_project_warning(self):
        self.expect_failure("::warning::Node.js 20 is deprecated. Vectoris LNK4006",
                            "UNKNOWN_DIAGNOSTIC")

    def test_nonzero_summary_without_diagnostics_fails(self):
        self.expect_failure("    2 Warning(s)", "UNKNOWN_DIAGNOSTIC")

    def test_zero_warning_summary(self):
        self.assertEqual(self.audit("    0 Warning(s)")["status"], "PASS")

    def test_external_error_is_still_failure(self):
        self.assertEqual(self.audit(EXTERNAL.replace("warning", "error"))["status"], "FAIL")

    def test_relative_first_party_source(self):
        self.expect_failure(COMPILER.replace(SOURCE + "/", ""),
                            "FIRST_PARTY_COMPILER_WARNING")

    def test_every_expected_phase_is_required(self):
        for filename in gate.EXPECTED_LOGS:
            with self.subTest(filename=filename):
                path = self.logs / filename
                content = path.read_bytes()
                path.unlink()
                with self.assertRaises(FileNotFoundError):
                    gate.audit_directory(self.logs, SOURCE, BUILD)
                path.write_bytes(content)

    def test_empty_log_fails(self):
        (self.logs / "build-debug.log").write_text("")
        with self.assertRaises(ValueError):
            self.audit()

    def test_truncated_log_fails(self):
        (self.logs / "header-isolation.log").write_text("Compiling probes...\n")
        with self.assertRaises(ValueError):
            self.audit()

    def test_invalid_encoding_fails(self):
        (self.logs / "build-debug.log").write_bytes(b"\xff\xfeinvalid")
        with self.assertRaises(UnicodeError):
            self.audit()

    def test_null_byte_fails(self):
        with self.assertRaises(ValueError):
            self.audit("\0")

    def test_parser_exception_is_failed_cli_result(self):
        args = ["--log-dir", str(self.logs), "--source-root", SOURCE, "--build-root", BUILD]
        with patch.object(gate, "classify", side_effect=RuntimeError("controlled parser error")):
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                code = gate.main(args)
        self.assertNotEqual(code, 0)
        self.assertEqual(json.loads(output.getvalue())["failure"], "AUDIT_EXCEPTION")

    def test_missing_log_is_failed_cli_result(self):
        (self.logs / "configure.log").unlink()
        with contextlib.redirect_stdout(io.StringIO()):
            code = gate.main(["--log-dir", str(self.logs), "--source-root", SOURCE,
                              "--build-root", BUILD])
        self.assertNotEqual(code, 0)

    def test_output_failure_is_failed_cli_result(self):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            code = gate.main(["--log-dir", str(self.logs), "--source-root", SOURCE,
                              "--build-root", BUILD, "--output", str(self.logs / "missing/report")])
        self.assertNotEqual(code, 0)

    def cli(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return gate.main(["--log-dir", str(self.logs), "--source-root", SOURCE,
                              "--build-root", BUILD])

    def test_clean_cli_exit_zero(self):
        self.assertEqual(self.cli(), 0)

    def test_compiler_warning_cli_exit_nonzero(self):
        self.audit(COMPILER)
        self.assertNotEqual(self.cli(), 0)

    def test_linker_and_librarian_cli_exit_nonzero(self):
        self.audit(LINKER + "\n" + LIBRARIAN)
        self.assertNotEqual(self.cli(), 0)

    def test_real_c4244_fixture(self):
        self.expect_failure(C4244, "FIRST_PARTY_COMPILER_WARNING")

    def test_real_c2220_fixture(self):
        self.expect_failure(C2220, "BUILD_ERROR")

    def test_lnk4006_linker_context(self):
        # The historical archive remains LIBRARIAN; the same diagnostic from
        # an executable link must retain the LINKER classification.
        text = LIBRARIAN.replace("VectorisNumerics_HeaderIsolation.vcxproj",
                                 "VectorisNumerics_Tests.vcxproj")
        self.expect_failure(text, "FIRST_PARTY_LINKER_WARNING")

    def test_unlisted_compiler_number_is_not_ignored(self):
        self.expect_failure(COMPILER.replace("C4101", "C9999"),
                            "FIRST_PARTY_COMPILER_WARNING")

    def test_unlisted_linker_number_is_not_ignored(self):
        self.expect_failure(LINKER.replace("LNK4075", "LNK4999"),
                            "FIRST_PARTY_LINKER_WARNING")

    def test_unknown_compiler_code_family_fails_closed(self):
        self.expect_failure(COMPILER.replace("C4101", "CXX9999"), "UNKNOWN_DIAGNOSTIC")

    def test_unknown_linker_code_family_fails_closed(self):
        self.expect_failure(LINKER.replace("LNK4075", "XYZ4999"), "UNKNOWN_DIAGNOSTIC")

    def test_unknown_dependency_code_fails_closed(self):
        self.expect_failure(EXTERNAL.replace("C4101", "XYZ9000"), "UNKNOWN_DIAGNOSTIC")

    def test_compiler_location_forms(self):
        origins = [
            SOURCE + "/modules/VectorisNumerics/tests/Probe.cpp(42)",
            SOURCE + "/modules/VectorisNumerics/tests/Probe.cpp(42,13)",
            SOURCE + "/modules/VectorisNumerics/tests/Probe.cpp(42,13,43,4)",
            '"' + SOURCE + '/modules/VectorisNumerics/tests/Probe.cpp"(42, 13)',
            "1>" + SOURCE + "/modules/VectorisNumerics/tests/Probe.cpp(42)",
            "modules/VectorisNumerics/tests/Probe.cpp(42)",
        ]
        for origin in origins:
            with self.subTest(origin=origin):
                result = gate.classify(origin + ": warning C4244: conversion",
                                       gate.normalized(SOURCE), gate.normalized(BUILD))
                self.assertEqual(result["category"], "FIRST_PARTY_COMPILER_WARNING")

    def test_command_line_compiler_diagnostic(self):
        self.expect_failure(
            "cl : Command line warning D9025 : overriding an option "
            "[" + NUMERICS + "VectorisNumerics_Tests.vcxproj]",
            "FIRST_PARTY_COMPILER_WARNING")

    def test_fatal_linker_error(self):
        self.expect_failure(LINKER.replace("warning LNK4075", "fatal error LNK1104"),
                            "BUILD_ERROR")

    def test_msbuild_tool_diagnostic(self):
        self.expect_failure(
            "MSBUILD : warning MSB3270: architecture mismatch "
            "[" + NUMERICS + "VectorisNumerics_Tests.vcxproj]",
            "FIRST_PARTY_BUILD_SYSTEM_WARNING")

    def test_explicit_librarian_tool_identity(self):
        self.expect_failure(LINKER.replace("LINK :", "lib.exe :"),
                            "FIRST_PARTY_LIBRARIAN_WARNING")

    def test_explicit_linker_tool_identity(self):
        self.expect_failure(LIBRARIAN.replace("order_poison_forward.obj :", "LINK :"),
                            "FIRST_PARTY_LINKER_WARNING")

    def test_cmake_warning_at_first_party_location(self):
        self.expect_failure("CMake Warning at " + SOURCE + "/CMakeLists.txt:42 (message):",
                            "FIRST_PARTY_BUILD_SYSTEM_WARNING")

    def test_cmake_developer_warning(self):
        self.expect_failure("CMake Warning (dev) in modules/VectorisNumerics/CMakeLists.txt:",
                            "FIRST_PARTY_BUILD_SYSTEM_WARNING")

    def test_cmake_deprecation_warning(self):
        self.expect_failure("CMake Deprecation Warning at cmake/Probe.cmake:2 (message):",
                            "FIRST_PARTY_BUILD_SYSTEM_WARNING")

    def test_cmake_error(self):
        self.expect_failure("CMake Error at cmake/Probe.cmake:42 (message):", "BUILD_ERROR")

    def test_cmake_dependency_warning(self):
        result = self.audit("CMake Warning at " + BUILD +
                            "/_deps/googletest-src/CMakeLists.txt:42 (message):")
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["counts"]["THIRD_PARTY_WARNING"], 1)

    def test_cmake_warning_without_ownership_fails_closed(self):
        self.expect_failure("CMake Warning:", "UNKNOWN_DIAGNOSTIC")

    def test_malformed_cmake_diagnostic_fails_closed(self):
        self.expect_failure("CMake Warning at cmake/Probe.cmake:42 (message)",
                            "UNKNOWN_DIAGNOSTIC")

    def test_missing_diagnostic_separators_fails_closed(self):
        self.expect_failure("modules/VectorisNumerics/tests/Probe.cpp(12) warning C4244 conversion",
                            "UNKNOWN_DIAGNOSTIC")

    def test_missing_diagnostic_code_fails_closed(self):
        self.expect_failure("modules/VectorisNumerics/tests/Probe.cpp(12): warning: conversion",
                            "UNKNOWN_DIAGNOSTIC")

    def test_bare_code_with_colon_fails_closed(self):
        self.expect_failure("C4244: conversion", "UNKNOWN_DIAGNOSTIC")

    def test_unlocated_coded_warning_fails_closed(self):
        self.expect_failure("warning C4244: conversion", "UNKNOWN_DIAGNOSTIC")

    def test_prose_cannot_hide_following_diagnostic(self):
        result = self.audit("The error model describes warning policy.\n" + COMPILER)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["counts"]["FIRST_PARTY_COMPILER_WARNING"], 1)
        self.assertEqual(result["counts"]["UNKNOWN_DIAGNOSTIC"], 0)
        self.assertEqual(result["diagnostics"][0]["raw"], COMPILER)

    def test_unreadable_log_fails_cli(self):
        with patch.object(Path, "read_text", side_effect=PermissionError("controlled unreadable log")):
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertNotEqual(gate.main([
                    "--log-dir", str(self.logs), "--source-root", SOURCE,
                    "--build-root", BUILD]), 0)

    def test_directory_instead_of_log_fails_cli(self):
        path = self.logs / "configure.log"
        path.unlink()
        path.mkdir()
        self.assertNotEqual(self.cli(), 0)

    def test_control_character_cannot_hide_diagnostic(self):
        with self.assertRaises(ValueError):
            self.audit("\x1b[31m" + COMPILER)


class GeneratedProjectPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.output = self.root / "evidence"
        self.output.mkdir()
        self.projects = []
        for module in ("Numerics", "Dynamics"):
            folder = self.root / f"modules/Vectoris{module}"
            folder.mkdir(parents=True)
            for suffix in ("Tests", "HeaderIsolation_Objects"):
                path = folder / f"Vectoris{module}_{suffix}.vcxproj"
                self.projects.append(path)
                stage, field = (("Link", "TreatLinkerWarningAsErrors") if suffix == "Tests"
                                else ("Lib", "TreatLibWarningAsErrors"))
                groups = []
                for config in ("Debug", "Release"):
                    groups.append(
                        f'''<ItemDefinitionGroup Condition="'{config}|x64'">
                        <ClCompile><TreatWarningAsError>true</TreatWarningAsError>
                        <WarningLevel>Level4</WarningLevel></ClCompile>
                        <{stage}><{field}>true</{field}></{stage}></ItemDefinitionGroup>''')
                path.write_text('<Project xmlns="' + native.XML["m"] + '">'
                                + "".join(groups) + "</Project>")

    def corrupt(self, index, before, after):
        path = self.projects[index]
        path.write_text(path.read_text().replace(before, after))
        with self.assertRaises(RuntimeError):
            native.inspect_projects(self.root, self.output)

    def test_native_project_clean(self):
        self.assertEqual(len(native.inspect_projects(self.root, self.output)), 4)

    def test_compiler_wx_required(self):
        self.corrupt(0, ">true</TreatWarningAsError>", ">false</TreatWarningAsError>")

    def test_compiler_level_required(self):
        self.corrupt(0, "Level4", "Level3")

    def test_linker_wx_required(self):
        self.corrupt(0, ">true</TreatLinkerWarningAsErrors>",
                     ">false</TreatLinkerWarningAsErrors>")

    def test_librarian_wx_required(self):
        self.corrupt(1, ">true</TreatLibWarningAsErrors>", ">false</TreatLibWarningAsErrors>")

    def test_both_configurations_required(self):
        self.corrupt(0, "'Release|x64'", "'Other|x64'")

    def test_missing_project_fails(self):
        self.projects[0].unlink()
        with self.assertRaises(FileNotFoundError):
            native.inspect_projects(self.root, self.output)

    def test_unexpected_probe_archive_fails(self):
        (self.root / "VectorisNumerics_HeaderIsolation_Objects.lib").touch()
        with self.assertRaises(RuntimeError):
            native.inspect_projects(self.root, self.output)


def prose_case(text):
    def test(self):
        self.assertIsNone(gate.classify(text, gate.normalized(SOURCE), gate.normalized(BUILD)))
        self.assertEqual(self.audit(text)["status"], "PASS")
    return test


candidate6 = json.loads((FIXTURES / "candidate6-msbuild-prose.json").read_text())
assert len(candidate6["records"]) == 21, "Every original false-positive occurrence is required"
for index, record in enumerate(candidate6["records"], 1):
    case = prose_case(record["raw_line"])
    case.__doc__ = record["source_log"] + ":" + str(record["source_line"])
    setattr(WarningGateTests, f"test_candidate6_prose_{index:02d}", case)

for index, prose in enumerate((
    "error hierarchy", "error handling", "error category", "error model",
    "warning policy", "warning classification", "warning gate",
    "no errors detected", "zero warnings required",
    "Description: error handling", "Contract: warning policy",
    "This guide explains warning C4244: conversion diagnostics.",
), 1):
    setattr(WarningGateTests, f"test_ordinary_prose_{index:02d}", prose_case(prose))


if __name__ == "__main__":
    unittest.main(verbosity=2)
