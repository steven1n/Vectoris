"""AFA004 controls hit the real workflow parser and command policy."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from verify_pipeline_gate import audit_workflow_file, workflow_run_blocks

SAFE='set -euo pipefail\nfalse | tee /dev/null'
CHECK='if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }'
class AFA004PipelineContract(unittest.TestCase):
    def audit(self, scripts):
        with tempfile.TemporaryDirectory() as tmp:
            text='jobs:\n  job:\n    runs-on: ubuntu-latest\n    steps:\n'
            for shell, script in scripts:
                text+='      - shell: '+shell+'\n        run: |\n'+''.join('          '+s+'\n' for s in script.splitlines())
            path=Path(tmp)/'workflow.yml';path.write_text(text)
            self.assertEqual(len(workflow_run_blocks(text)),len(scripts))
            with contextlib.redirect_stdout(io.StringIO()):
                return audit_workflow_file(path)
    def test_first_middle_last_blocks(self):
        for index in range(3):
            scripts=[('bash',SAFE)]*3;scripts[index]=('bash','false | tee /dev/null')
            with self.subTest(index=index): self.assertFalse(self.audit(scripts))
    def test_safe_bash(self): self.assertTrue(self.audit([('bash',SAFE)]))
    def test_safe_prologue_after_comments_and_blank_lines(self):
        self.assertTrue(self.audit([('bash','\n# qualification metadata\n\n'+SAFE)]))
    def test_prose_is_not_prologue(self):
        self.assertFalse(self.audit([('bash','# set -euo pipefail\nfalse | tee /dev/null')]))
        self.assertFalse(self.audit([('bash','echo "set -euo pipefail"\nfalse | tee /dev/null')]))
    def test_disabled_policy(self): self.assertFalse(self.audit([('bash',SAFE+'\nset +o pipefail\nfalse | tee log')]))
    def test_pwsh_masking(self): self.assertFalse(self.audit([('pwsh','& cmake.exe --build bad\n& cmd.exe /c exit 0\n'+CHECK)]))
    def test_pwsh_immediate(self): self.assertTrue(self.audit([('pwsh','& cmake.exe --build good\n'+CHECK)]))
    def test_pwsh_missing(self): self.assertFalse(self.audit([('pwsh','& cmake.exe --build bad')]))
    def test_pwsh_multiline(self): self.assertTrue(self.audit([('pwsh','& cmake.exe `\n-S . `\n-B build\n'+CHECK)]))
    def test_pwsh_pipeline_cannot_mask_native_status(self):
        self.assertFalse(self.audit([('pwsh','& cmake.exe --build bad | & cmd.exe /c exit 0\n'+CHECK)]))
        self.assertFalse(self.audit([('pwsh','& cmake.exe --build bad | cmd.exe /c exit 0\n'+CHECK)]))
    def test_nested_shell_and_sourced_policy_rejected(self):
        self.assertFalse(self.audit([('bash',SAFE+'\nbash -c "false | tee /dev/null"')]))
        self.assertFalse(self.audit([('bash',SAFE+'\nsource change-policy.sh')]))
    def test_bash_conditional_and_substitution_masking(self):
        for script in ('false && true\necho masked', 'echo "$(false)"', 'false &\necho masked',
                       "trap 'exit 0' ERR\nfalse"):
            with self.subTest(script=script):
                self.assertFalse(self.audit([('bash', 'set -euo pipefail\n'+script)]))
    def test_bash_subshell_function_and_dynamic_eval(self):
        for script in ('(set +e; false; true)', 'f() { false; true; }; f',
                       "command eval 'false && true; echo masked'"):
            with self.subTest(script=script):
                self.assertFalse(self.audit([('bash','set -euo pipefail\n'+script)]))
    def test_pwsh_hidden_native_and_unknown_statements(self):
        for script in ('$result = git.exe missing\n& cmd.exe /c exit 0\n'+CHECK,
                       'if ($true) { & failing.exe }', 'unknown-native.exe\nWrite-Host done',
                       '& cmake.exe "$(git.exe missing)"\n'+CHECK,
                       '& cmake.exe @(unknown-native.exe)\n'+CHECK,
                       '$metadata = @"\n$(unknown-native.exe)\n"@'):
            with self.subTest(script=script): self.assertFalse(self.audit([('pwsh', script)]))
    def test_continue_on_error_rejected(self):
        for field in ('    continue-on-error: true\n', '    continue-on-error: ${{ inputs.ignore }}\n'):
            text='jobs:\n  job:\n'+field+'    runs-on: ubuntu-latest\n    steps:\n      - run: echo test\n'
            with self.assertRaisesRegex(ValueError, 'continue on error'): workflow_run_blocks(text)
        text='jobs:\n  job:\n    runs-on: ubuntu-latest\n    steps:\n      - run: echo test\n        continue-on-error: true\n'
        with self.assertRaisesRegex(ValueError, 'continue on error'): workflow_run_blocks(text)
    def test_combined_set_and_shopt_masking(self):
        for mutation in ('set -e +o pipefail', 'set -u +e', 'shopt -uo pipefail',
                         'shopt -uo errexit', 'builtin set +e', 'command shopt -uo pipefail'):
            for position in range(3):
                scripts=[('bash', SAFE)]*3
                scripts[position]=('bash', 'set -euo pipefail\n'+mutation+'\nfalse | tee /dev/null\necho masked')
                with self.subTest(mutation=mutation, position=position):
                    self.assertFalse(self.audit(scripts))
    def test_native_command_on_here_string_opener(self):
        for opener in ('& cmd /c exit 7; $meta = @"', '& failing.exe @"',
                       '$meta = & failing.exe @"', '& failing.exe `\n$meta = @"'):
            script=opener+'\ntext\n"@\n& cmd /c exit 0\n'+CHECK
            with self.subTest(opener=opener): self.assertFalse(self.audit([('pwsh',script)]))
    def test_standalone_metadata_here_string(self):
        self.assertTrue(self.audit([('pwsh','$meta = @"\nordinary metadata\n"@\nWrite-Host $meta')]))
    def test_option_changes_cannot_be_hidden_in_wrappers(self):
        for command in ("env -S 'bash -c false'", "/usr/bin/env -S 'bash -c false'",
                        "exec /bin/bash -c 'false; true'", "exit 0", "return 0"):
            with self.subTest(command=command):
                self.assertFalse(self.audit([('bash','set -euo pipefail\n'+command)]))
    def test_indirect_policy_mutator_rejected(self):
        for command in ('policy=set\n$policy -e +o pipefail',
                        '${POLICY} -e +o pipefail', '"$POLICY" -e +o pipefail',
                        'FOO=1 set -e +o pipefail'):
            with self.subTest(command=command):
                script='set -euo pipefail\n'+command+'\nfalse | tee /dev/null\necho masked'
                self.assertFalse(self.audit([('bash',script)]))
    def test_literal_compiler_prefix_preserves_command_audit(self):
        self.assertTrue(self.audit([('bash','set -euo pipefail\nCC=clang-22 CXX=clang++-22 cmake -S .')]))
        self.assertFalse(self.audit([('bash','set -euo pipefail\nCC=clang-22 set -e +o pipefail')]))
        self.assertFalse(self.audit([('bash','set -euo pipefail\nCC=clang-22 $POLICY +e')]))
    def test_astra_originals(self):
        for path in (Path(__file__).parent/'fixtures').glob('*.yml'):
            with self.subTest(path=path.name),contextlib.redirect_stdout(io.StringIO()):
                self.assertFalse(audit_workflow_file(path))
    def test_missing_malformed_duplicate(self):
        with self.assertRaises(Exception): workflow_run_blocks('jobs: [unterminated')
        with self.assertRaisesRegex(ValueError,'duplicate YAML key'): workflow_run_blocks('jobs: {}\njobs: {}')
        with contextlib.redirect_stdout(io.StringIO()): self.assertFalse(audit_workflow_file('/nonexistent/workflow.yml'))
if __name__=='__main__': unittest.main(verbosity=2)
