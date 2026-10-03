#!/usr/bin/env python3
"""Compile and execute the exact Core consumer entrances published in docs/core.md."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ENTRANCES = {
    'primitives': ('Math.h', ('core::sqrt(', 'core::abs(')),
    'wrappers': ('MathFunctions.h', ('Math::sin(', 'Math::cos(', 'Math::sqrt(')),
    'traits': ('NumericTraits.h', ('NumericTraits<double>::epsilon()', 'core::AlmostEqual(')),
    'constants': ('Constants.h', ('Constants::Pi<double>',)),
}


def read_contracts(root, document=None, manifest=None):
    root = Path(root)
    text = Path(document or root / 'docs/core.md').read_text(encoding='utf-8')
    spec = json.loads(Path(manifest or root / 'tools/api_surface/public_api_manifest.json').read_text(encoding='utf-8'))
    declared = spec.get('documented_core_include_contracts')
    expected = {key: value[0] for key, value in ENTRANCES.items()}
    if declared != expected:
        raise ValueError('documented Core manifest entrance identities/headers mismatch')
    table = text.split('## 4. Public Headers', 1)[1].split('## 5.', 1)[0]
    rows = re.findall(r'^\| \[`([^`]+\.h)`\]\([^\n]+?\) \| (.+) \|$', table, re.MULTILINE)
    names = [name for name, _ in rows]
    on_disk = {path.name for path in (root / 'modules/VectorisNumerics/include/Vectoris/Numerics/Core').glob('*.h')}
    if len(names) != len(set(names)) or set(names) != on_disk or not names:
        raise ValueError('Core public-header table is duplicate, incomplete or unexpected')
    role = dict(rows)['Math.h']
    if not role.startswith('Primitive entry:') or 'umbrella' in role.lower():
        raise ValueError('Math.h must document the narrow primitive entry, not an umbrella')
    blocks = re.findall(r'<!-- vectoris-core-contract: ([a-z]+) -->\s*```cpp\n(.*?)\n```', text, re.DOTALL)
    ids = [key for key, _ in blocks]
    if len(ids) != len(set(ids)) or set(ids) != set(ENTRANCES):
        raise ValueError('documented Core source identities are missing, duplicate or unexpected')
    sources = dict(blocks)
    for key, (header, symbols) in ENTRANCES.items():
        source = sources[key]
        includes = re.findall(r'^\s*#\s*include\s*[<"](Vectoris/[^>"\n]+)[>"]', source, re.MULTILINE)
        if includes != ['Vectoris/Numerics/Core/' + header]:
            raise ValueError(f'{key}: wrong or unrelated Vectoris include entrance')
        if 'int main()' not in source or any(symbol not in source for symbol in symbols):
            raise ValueError(f'{key}: consumer body/export assertions missing')
    return sources


def compile_contracts(root, compiler, compiler_id, sources):
    include = str(Path(root) / 'modules/VectorisNumerics/include')
    completed = []
    with tempfile.TemporaryDirectory(prefix='vectoris-core-contract-') as temporary:
        directory = Path(temporary)
        for key in sorted(sources):
            source = directory / (key + '.cpp')
            source.write_text(sources[key], encoding='utf-8')
            executable = directory / (key + ('.exe' if compiler_id == 'MSVC' else ''))
            if compiler_id == 'MSVC':
                flags = ['/nologo', '/std:c++20', '/W4', '/WX', '/permissive-', '/utf-8', '/EHsc', '/I' + include,
                         str(source), '/Fe:' + str(executable), '/Fo:' + str(directory / (key + '.obj'))]
            elif compiler_id in ('GNU', 'Clang', 'AppleClang'):
                flags = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror',
                         '-I' + include, str(source), '-o', str(executable)]
            else:
                raise ValueError('unsupported Core contract compiler: ' + compiler_id)
            command = [compiler] + flags
            result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=120)
            print('COMMAND=' + json.dumps(command), flush=True)
            if result.returncode:
                raise ValueError(f'{key}: compile exit {result.returncode}\n{result.stdout}\n{result.stderr}')
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            if result.returncode:
                raise ValueError(f'{key}: consumer exit {result.returncode}\n{result.stdout}\n{result.stderr}')
            completed.append(key)
            print(f'CORE_DOCUMENTED_CONSUMER {key}: compiled/executed PASS', flush=True)
    if set(completed) != set(ENTRANCES) or len(completed) != len(ENTRANCES):
        raise ValueError('Core consumer execution identities mismatch')
    print('REQUIRED=SOURCE_DEFINED=COMPILED=EXECUTED=PASSED=' + json.dumps(sorted(completed)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', default=str(Path(__file__).resolve().parents[2]))
    parser.add_argument('--document', help='External document override for controlled negative tests')
    parser.add_argument('--manifest', help='External manifest override for controlled negative tests')
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--compiler-id', choices=['GNU', 'Clang', 'AppleClang', 'MSVC'], required=True)
    args = parser.parse_args()
    try:
        sources = read_contracts(args.repo_root, args.document, args.manifest)
        compile_contracts(args.repo_root, args.cxx, args.compiler_id, sources)
    except (OSError, ValueError, IndexError, subprocess.SubprocessError) as error:
        print('ERROR: Core documented include contract: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
