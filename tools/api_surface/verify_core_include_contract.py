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


# Qualification-side role grammar, not a copy of the document's prose. Only
# these two entrances have architecture-specific export/dependency obligations.
EXPECTED_ROLES = {
    'Math.h': {'kind': 'primitive', 'exports': {'core::sqrt', 'core::abs'},
               'explicit_entrance': 'MathFunctions.h'},
    'MathFunctions.h': {'kind': 'wrappers', 'exports': {'abs', 'sin', 'cos', 'acos'},
                        'compatibility': 'sqrt'},
}


def markdown_cells(line):
    """Supported tables have outer pipes, two cells and no embedded pipes.

    Arbitrary horizontal cell padding is harmless. Escaped/embedded pipes and
    multiline cells are outside this small contract grammar and fail closed.
    """
    row = line.strip()
    if not row.startswith('|') or not row.endswith('|'):
        raise ValueError('unrecognized public-header table row: ' + line)
    cells = [cell.strip() for cell in row[1:-1].split('|')]
    if len(cells) != 2 or not all(cells):
        raise ValueError('unrecognized public-header table row: ' + line)
    return cells



def outside_fences(lines):
    """Preserve positions while excluding explicitly fenced display examples."""
    visible = []
    fence = None
    for line in lines:
        if fence is not None:
            closing = re.fullmatch(r'\s*(' + re.escape(fence[0]) + r'{'
                                   + str(fence[1]) + r',})\s*', line)
            if closing:
                fence = None
            visible.append('')
        else:
            opening = re.fullmatch(r'\s*(`{3,}|~{3,})[^`]*', line)
            if opening:
                fence = (opening[1][0], len(opening[1]))
                visible.append('')
            else:
                visible.append(line)
    if fence is not None:
        raise ValueError('unclosed fence in Public Headers section')
    return visible


def public_header_rows(text):
    lines = text.splitlines()
    starts = [i for i, line in enumerate(lines)
              if re.fullmatch(r'##\s+4\.\s+Public Headers\s*', line)]
    if len(starts) != 1:
        raise ValueError('missing or duplicate Public Headers section')
    start = starts[0] + 1
    end = next((i for i in range(start, len(lines))
                if lines[i].startswith('## ')), len(lines))
    section = outside_fences(lines[start:end])
    first = next((i for i, line in enumerate(section) if line.lstrip().startswith('|')), None)
    if first is None or markdown_cells(section[first]) != ['Header', 'Description']:
        raise ValueError('missing Public Headers table heading')
    if first + 1 >= len(section) or not all(
            re.fullmatch(r':?-{3,}:?', cell) for cell in markdown_cells(section[first + 1])):
        raise ValueError('invalid Public Headers table separator')
    rows = []
    cursor = first + 2
    while cursor < len(section) and section[cursor].strip():
        if section[cursor].startswith('#'):
            break
        identity, role = markdown_cells(section[cursor])
        link = re.fullmatch(r'\[([^\[\]]+)\]\(([^\s()]+)\)', identity)
        if not link:
            raise ValueError('unrecognized public-header table row: ' + section[cursor])
        label = link[1].strip()
        # Plain, inline-code and emphasis around the link label denote the same
        # header. Nested/other Markdown constructs are intentionally unsupported.
        for marker in ('**', '*', '`'):
            if label.startswith(marker) and label.endswith(marker):
                label = label[len(marker):-len(marker)].strip()
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*\.h', label):
            raise ValueError('unrecognized public-header table row: ' + section[cursor])
        if link[2] != '../modules/VectorisNumerics/include/Vectoris/Numerics/Core/' + label:
            raise ValueError('public-header link identity/target mismatch: ' + label)
        rows.append((label, role))
        cursor += 1
    # Fenced examples were excluded before selecting the table. Every other
    # separated data-like row/second table remains visible and fails closed.
    for line in section[cursor:]:
        if line.lstrip().startswith('|'):
            raise ValueError('unrecognized public-header table row outside table: ' + line)
    return rows


def validate_role(name, role):
    """Recognize positive architecture assertions with flexible padding/markup.

    This is a deliberately bounded English role grammar. Unrecognized assertions
    fail closed rather than trying to infer truth from arbitrary natural language.
    Generic descriptions remain prose; the two entrance roles encode dependency
    direction and allowed exports explicitly in EXPECTED_ROLES.
    """
    expected = EXPECTED_ROLES.get(name)
    if expected is None:
        return
    normalized = ' '.join(role.replace('`', '').split()).rstrip('.')
    if expected['kind'] == 'primitive':
        parts = normalized.split(';')
        exports = re.fullmatch(
            r'(?:narrow )?primitive (?:entry|header):\s*(?:canonical )?'
            r'(core::[a-z]+)\s+(?:and|,)\s+(?:existing )?(core::[a-z]+)', parts[0], re.IGNORECASE)
        dependency = re.fullmatch(
            r'\s*include ([a-z]+\.h) (?:explicitly for wrappers|for wrappers explicitly)',
            parts[1], re.IGNORECASE) if len(parts) == 2 else None
        if (not exports or set(exports.groups()) != expected['exports'] or not dependency
                or dependency[1] != expected['explicit_entrance']):
            raise ValueError('Math.h must document the narrow primitive entry and explicit wrapper entrance')
    else:
        wrappers = re.fullmatch(
            r'(?i:wrapper(?:/trigonometric)? (?:entry|entrance):\s*)?'
            r'core::Math::\{([a-z,\s]+)\}\s+(?i:and\s+compatibility)\s+([a-z]+)', normalized)
        symbols = [symbol.strip() for symbol in wrappers[1].split(',')] if wrappers else []
        if (not wrappers or set(symbols) != expected['exports'] or len(symbols) != len(set(symbols))
                or wrappers[2] != expected['compatibility']):
            raise ValueError('MathFunctions.h must document wrapper/trigonometric exports and compatibility sqrt')


def read_contracts(root, document=None, manifest=None):
    root = Path(root)
    text = Path(document or root / 'docs/core.md').read_text(encoding='utf-8')
    spec = json.loads(Path(manifest or root / 'tools/api_surface/public_api_manifest.json').read_text(encoding='utf-8'))
    declared = spec.get('documented_core_include_contracts')
    expected = {key: value[0] for key, value in ENTRANCES.items()}
    if declared != expected:
        raise ValueError('documented Core manifest entrance identities/headers mismatch')
    rows = public_header_rows(text)
    names = [name for name, _ in rows]
    on_disk = {path.name for path in (root / 'modules/VectorisNumerics/include/Vectoris/Numerics/Core').glob('*.h')}
    if len(names) != len(set(names)) or set(names) != on_disk or not names:
        raise ValueError('Core public-header table is duplicate, incomplete or unexpected')
    for name, role in rows:
        validate_role(name, role)
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
