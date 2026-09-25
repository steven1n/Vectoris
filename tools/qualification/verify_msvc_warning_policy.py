#!/usr/bin/env python3
"""Real Windows compiler/linker/librarian controls; never simulated by fixtures.

The warning-producing sources live only in the requested evidence directory.
Each native tool must emit its expected warning with exit 0, then reject that
same input with /WX. A separate CMake experiment reproduces the OBJECT-library
aggregation and exercises the actual compile-only HeaderIsolation helper.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
XML = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


class Experiments:
    def __init__(self, output):
        self.output = output.resolve()
        self.output.mkdir(parents=True, exist_ok=True)
        self.commands = []
        self.results = {}

    def run(self, name, command):
        result = subprocess.run([str(x) for x in command], cwd=self.output,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        (self.output / (name + ".log")).write_bytes(result.stdout)
        self.commands.append({"name": name, "command": [str(x) for x in command],
                              "exit_code": result.returncode})
        (self.output / "commands.json").write_text(json.dumps(self.commands, indent=2),
                                                 encoding="utf-8")
        return result.returncode, result.stdout.decode("utf-8", errors="strict")

    def success(self, name, command):
        code, text = self.run(name, command)
        require(code == 0, f"{name}: expected successful tool exit, got {code}")
        require(not re.search(r"\b(?:warning|fatal error|error)\s+[A-Z]+\d+", text, re.I),
                f"{name}: unexpected native diagnostic")
        return text

    def warning_pair(self, name, command, warning):
        code, text = self.run(name + "-control", command)
        require(code == 0 and re.search(r"\bwarning\s+" + warning, text, re.I),
                f"{name}: warning-producing exit-0 control not reproduced")
        strict_code, strict_text = self.run(name + "-wx", command + ["/WX"])
        require(strict_code != 0 and warning in strict_text,
                f"{name}: /WX did not reject the same warning")
        require("LNK4044" not in strict_text and "D9002" not in strict_text,
                f"{name}: warning-as-error option was not recognized")
        self.results[name] = {"warning": warning, "control_exit": code,
                              "wx_exit": strict_code}

    def native_tools(self):
        for tool in ("cl.exe", "link.exe", "lib.exe", "cmake.exe"):
            require(shutil.which(tool), f"Required real MSVC tool missing: {tool}")
        (self.output / "compiler.cpp").write_text(
            "int compiler_probe() { int unused; return 0; }\n")
        self.warning_pair("compiler", [
            "cl.exe", "/nologo", "/std:c++20", "/W4", "/c", "compiler.cpp",
            "/Focompiler.obj"], "C4101")
        for name, value in (("duplicate_a", 1), ("duplicate_b", 2)):
            (self.output / (name + ".cpp")).write_text(
                f"int collision() {{ return {value}; }}\n")
            self.success(name, ["cl.exe", "/nologo", "/std:c++20", "/W4", "/WX",
                                "/c", name + ".cpp", "/Fo" + name + ".obj"])
        self.warning_pair("librarian", [
            "lib.exe", "/nologo", "/out:duplicates.lib", "duplicate_a.obj",
            "duplicate_b.obj"], "LNK4006")
        (self.output / "entry.cpp").write_text('extern "C" void entry() {}\n')
        self.success("entry", ["cl.exe", "/nologo", "/std:c++20", "/W4", "/WX",
                               "/c", "entry.cpp", "/Foentry.obj"])
        self.warning_pair("linker", [
            "link.exe", "/nologo", "/entry:entry", "/subsystem:console", "/nodefaultlib",
            "/machine:x64", "/incremental", "/opt:ref", "/out:entry.exe", "entry.obj"],
            "LNK4075")

    def cmake_aggregation(self):
        source = self.output / "aggregation-source"
        source.mkdir(exist_ok=True)
        for name in ("first", "second"):
            (source / (name + ".cpp")).write_text("int main() { return 0; }\n")
        (source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.14)\nproject(NativeAggregation LANGUAGES CXX)\n"
            "set(CMAKE_CXX_STANDARD 20)\nset(CMAKE_CXX_EXTENSIONS OFF)\n"
            "add_library(Vectoris_ArchiveProbe OBJECT first.cpp second.cpp)\n")
        control = self.output / "aggregation-control"
        self.success("aggregate-configure", ["cmake.exe", "-S", source, "-B", control])
        code, text = self.run("aggregate-build", [
            "cmake.exe", "--build", control, "--config", "Debug",
            "--target", "Vectoris_ArchiveProbe"])
        require(code == 0 and "warning LNK4006" in text,
                "Visual Studio OBJECT aggregation control did not reproduce LNK4006")
        strict = self.output / "aggregation-wx"
        self.success("aggregate-wx-configure", [
            "cmake.exe", "-S", source, "-B", strict, "-DCMAKE_STATIC_LINKER_FLAGS=/WX"])
        strict_code, strict_text = self.run("aggregate-wx-build", [
            "cmake.exe", "--build", strict, "--config", "Debug",
            "--target", "Vectoris_ArchiveProbe"])
        require(strict_code != 0 and "LNK4006" in strict_text,
                "CMake /WX librarian policy did not fail the deliberate duplicate")
        self.results["msbuild_object_aggregation"] = {
            "control_exit": code, "wx_exit": strict_code, "warning": "LNK4006"}

    def compile_only_control(self):
        source = self.output / "compile-only-source"
        source.mkdir(exist_ok=True)
        (source / "first.cpp").write_text("static_assert(sizeof(char) == 1);\n")
        (source / "second.cpp").write_text(
            "namespace { [[maybe_unused]] void statement_probe() {\n"
            "  [[maybe_unused]] const int value = 42;\n} }\n")
        helper = (REPO / "cmake/HeaderIsolationTarget.cmake").as_posix()
        (source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.14)\nproject(CompileOnly LANGUAGES CXX)\n"
            "set(VECTORIS_STRICT_WARNINGS /W4 /WX /permissive-)\n"
            "add_library(ProbeContract INTERFACE)\n"
            f'include("{helper}")\n'
            "vectoris_add_header_isolation_target(Vectoris_NativeHeaderProbe\n"
            " SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/first.cpp\n"
            " ${CMAKE_CURRENT_SOURCE_DIR}/second.cpp LIBRARIES ProbeContract)\n")
        build = self.output / "compile-only-build"
        self.success("compile-only-configure", [
            "cmake.exe", "-S", source, "-B", build, "-DCMAKE_STATIC_LINKER_FLAGS=/WX"])
        text = self.success("compile-only-build", [
            "cmake.exe", "--build", build, "--config", "Debug"])
        require("VECTORIS_HEADER_ISOLATION_COMPLETE Vectoris_NativeHeaderProbe 2" in text,
                "The compile-only target did not finish")
        require(not list(build.rglob("Vectoris_NativeHeaderProbe_Objects.lib")),
                "Compile-only probe unexpectedly produced an archive")
        objects = [p for p in build.rglob("*.obj")
                   if "Vectoris_NativeHeaderProbe_Objects.dir" in p.parts]
        require(len(objects) == 2,
                "Compile-only control did not compile both translation units")
        self.results["compile_only_control"] = {"compiled_objects": 2, "archives": 0}


def inspect_projects(build, output):
    """Preserve exact generated metadata and verify Debug/Release native flags."""
    summary = {}
    for module in ("Numerics", "Dynamics"):
        for suffix in ("Tests", "HeaderIsolation_Objects"):
            name = f"Vectoris{module}_{suffix}"
            project = build / f"modules/Vectoris{module}/{name}.vcxproj"
            root = ET.parse(project).getroot()
            shutil.copyfile(project, output / project.name)
            configurations = {}
            for group in root.findall("m:ItemDefinitionGroup", XML):
                condition = group.get("Condition", "")
                for config in ("Debug", "Release"):
                    if not re.search(rf"'{config}\|", condition):
                        continue
                    cl = group.find("m:ClCompile", XML)
                    require(cl is not None, f"{name}/{config}: missing compiler metadata")
                    require(cl.findtext("m:TreatWarningAsError", namespaces=XML) == "true",
                            f"{name}/{config}: compiler /WX absent")
                    require(cl.findtext("m:WarningLevel", namespaces=XML) == "Level4",
                            f"{name}/{config}: compiler /W4 absent")
                    stage, field = (("Link", "TreatLinkerWarningAsErrors") if suffix == "Tests"
                                    else ("Lib", "TreatLibWarningAsErrors"))
                    tool = group.find("m:" + stage, XML)
                    require(tool is not None and
                            tool.findtext("m:" + field, namespaces=XML) == "true",
                            f"{name}/{config}: {stage} /WX absent")
                    configurations[config] = {"compiler": "/W4 /WX", stage: "/WX"}
            require(set(configurations) == {"Debug", "Release"},
                    f"{name}: missing expected configurations")
            if suffix != "Tests":
                require(not list(build.rglob(name + ".lib")),
                        f"{name}: unexpected HeaderIsolation archive")
            summary[name] = configurations
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--inspect-build", type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    report = {"status": "FAIL"}
    try:
        require(sys.platform == "win32", "Native MSVC evidence requires a Windows host")
        os.environ["VSLANG"] = "1033"
        if args.inspect_build:
            report["generated_project_policy"] = inspect_projects(
                args.inspect_build.resolve(), args.output_dir.resolve())
        else:
            experiments = Experiments(args.output_dir)
            experiments.native_tools()
            experiments.cmake_aggregation()
            experiments.compile_only_control()
            report["experiments"] = experiments.results
        report["status"] = "PASS"
    except Exception as error:
        report["failure"] = f"{type(error).__name__}: {error}"
    (args.output_dir / "result.json").write_text(json.dumps(report, indent=2) + "\n",
                                              encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
