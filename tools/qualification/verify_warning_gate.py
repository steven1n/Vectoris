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

    def test_first_party_linker_warning(self):
        self.expect_failure(LINKER, "FIRST_PARTY_LINKER_WARNING")

    def test_candidate5_real_librarian_fixture(self):
        self.expect_failure(LIBRARIAN, "FIRST_PARTY_LIBRARIAN_WARNING")

    def test_third_party_warning_separate(self):
        result = self.audit(EXTERNAL)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["counts"]["THIRD_PARTY_WARNING"], 1)

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


if __name__ == "__main__":
    unittest.main(verbosity=2)
