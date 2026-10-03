#!/usr/bin/env python3
"""Record exact frozen-baseline/current nonfinite rows with independent IEEE classes.

The baseline is diagnostic-only. Current rows must satisfy the existing contract.
Probe source, copied public headers, binaries and logs are outside the source tree.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tarfile
import tempfile

BASELINE = "c553ad168f392b1ceb857329d275c3af5ffb076a"
PROBE = r'''
#include <Vectoris/Numerics/Geometry/Quaternion.h>
#include <Vectoris/Numerics/Units/BaseUnits/Length.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string_view>
namespace g=vectoris::numerics::geometry; namespace u=vectoris::numerics::units;
struct F{};
template<class T> char kind(T x){return std::isnan(x)?'N':std::isinf(x)?(std::signbit(x)?'M':'P'):x==T{0}?'Z':'F';}
template<class T> int run(const char* precision){
 const T n=std::numeric_limits<T>::quiet_NaN(),i=std::numeric_limits<T>::infinity(),m=std::numeric_limits<T>::max();
 const std::array<g::Vector3<T,F>,12> input{{{n,1,2},{1,n,2},{1,2,n},{n,n,n},{n,0,-T{0}},
     {n,m,-m},{i,1,2},{-i,1,2},{i,-i,n},{-T{0},T{0},-T{0}},{1,i,2},{1,2,-i}}};
 constexpr std::array<const char*,12> expected{{"NNN","NNN","NNN","NNN","NNN","NNN",
     "PNN","MNN","NNN","ZZZ","NPN","NNM"}};
 const auto q=g::Quaternion<T,F,F>::Identity();const auto rotation=q.ToRotationMatrix().Value();
 const auto direct=g::RotationMatrix3<T,F,F>::Identity();using Q=u::Quantity<T,u::MeterUnit>;
 int failures=0;
 for(std::size_t k=0;k<input.size();++k){
  const auto v=input[k];const auto a=direct*v,b=rotation*v;
  const auto quantity=rotation*g::Vector3<Q,F>{Q{v.x},Q{v.y},Q{v.z}};
  const std::array<g::Vector3<T,F>,3> outputs{{a,b,{quantity.x.value(),quantity.y.value(),quantity.z.value()}}};
  for(std::size_t path=0;path<outputs.size();++path){const auto out=outputs[path];
   const std::array<char,3> actual{{kind(out.x),kind(out.y),kind(out.z)}};
   const bool match=actual[0]==expected[k][0]&&actual[1]==expected[k][1]&&actual[2]==expected[k][2];
   failures+=match?0:1;
   std::printf("C13_CLASS,%s,case=%zu,path=%zu,input=%c%c%c,expected=%s,actual=%c%c%c,values=%a/%a/%a,match=%d\n",
       precision,k,path,kind(v.x),kind(v.y),kind(v.z),expected[k],actual[0],actual[1],actual[2],
       static_cast<double>(out.x),static_cast<double>(out.y),static_cast<double>(out.z),match?1:0);
  }
 }
 return failures;
}
int main(int argc,char** argv){const int failures=run<float>("float")+run<double>("double");
 std::printf("C13_CLASS_MISMATCHES=%d\n",failures);
 return argc>1&&std::string_view(argv[1])=="strict"&&failures!=0?1:0;}
'''


def checked(command, *, cwd, log):
    print("COMMAND", shlex.join([str(x) for x in command]), flush=True)
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    text = result.stdout + result.stderr
    log.write_text(text, encoding="utf-8")
    print(text, end="", flush=True)
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}): {command[0]}")
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    compiler = shutil.which(os.environ.get("CXX", "cl.exe" if os.name == "nt" else "c++"))
    if not compiler:
        raise RuntimeError("Compiler missing")
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True).strip())
    header_hash = hashlib.sha256((root/"modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/RotationMatrix3.h").read_bytes()).hexdigest()
    if subprocess.run(["git", "cat-file", "-e", BASELINE], cwd=root, capture_output=True).returncode:
        checked(["git", "fetch", "--no-tags", "--depth=1", "origin", BASELINE], cwd=root, log=output/"fetch-baseline.log")
    archive = subprocess.check_output(["git", "archive", BASELINE, "modules/VectorisNumerics/include"], cwd=root)
    print(f"C13_NONFINITE_PROVENANCE baseline={BASELINE} current={sha} working_tree_dirty={dirty} header_sha256={header_hash} compiler={compiler}", flush=True)
    records = []
    with tempfile.TemporaryDirectory(prefix="vectoris-nonfinite-") as folder:
        temp = Path(folder)
        with tarfile.open(fileobj=io.BytesIO(archive)) as data:
            data.extractall(temp/"baseline", filter="data")
        source = temp/"probe.cpp"
        source.write_text(PROBE, encoding="utf-8")
        for version, include in [("baseline", temp/"baseline/modules/VectorisNumerics/include"),
                                 ("current", root/"modules/VectorisNumerics/include")]:
            for mode in ("debug", "release"):
                label = f"{version}-{mode}"
                binary = temp/(label + (".exe" if os.name == "nt" else ""))
                if os.name == "nt":
                    command = [compiler, "/nologo", "/std:c++20", "/W4", "/WX", "/EHsc", "/permissive-", "/fp:precise",
                               "/Od" if mode == "debug" else "/O2", "/I"+str(include), str(source), "/Fe"+str(binary), "/Fo"+str(temp/(label+".obj")), "/link", "/WX"]
                else:
                    command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror",
                               "-O0" if mode == "debug" else "-O3", "-ffp-contract=off", "-I"+str(include), str(source), "-o", str(binary)]
                checked(command, cwd=temp, log=output/(label+"-compile.log"))
                text = checked([str(binary), "strict" if version == "current" else "record"], cwd=temp, log=output/(label+"-rows.log"))
                if text.count("C13_CLASS,") != 72 or "C13_CLASS_MISMATCHES=" not in text:
                    raise RuntimeError("Missing nonfinite classification execution evidence")
                if version == "current" and "C13_CLASS_MISMATCHES=0\n" not in text:
                    raise RuntimeError("Current nonfinite classification mismatch")
                records.append({"version":version, "mode":mode, "baseline_sha":BASELINE, "current_sha":sha,
                                "working_tree_dirty":dirty, "current_header_sha256":header_hash,
                                "rows":text.count("C13_CLASS,"), "compile_command":command})
    (output/"provenance.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print("C13_NONFINITE_GATE PASS: current float/double rows satisfy independent IEEE classification.", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"C13_NONFINITE_GATE FAIL: {type(error).__name__}: {error}", flush=True)
        raise SystemExit(1) from error
