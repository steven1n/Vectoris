# Qualification evidence checklist (AFA-007 / AFA-008)

A report is a summary of evidence, not a substitute for it. Record the candidate
commit/tree, compiler versions, commands and raw logs; distinguish a local working
copy run from an exact-SHA CI run. Preserve superseded reports and issue a dated
erratum with links to both the original claim and its corrected source.

Before reporting a gate as complete:

- Cross-check each algorithm description against the candidate implementation and
  its authoritative API documentation. In particular, Core/Math.h float/double
  constexpr sqrt is bounded integer digit-by-digit/restoring square root with
  24/53 iterations, not Newton iteration (ENGINEERING_STANDARD_V1.md §15).
- Count actual executed/passed/failed/skipped controls, not a success banner. For
  API tests retain REQUIRED, SOURCE_DEFINED, REGISTERED, EXECUTED and PASSED
  identities. For clang-tidy retain expected/eligible/analyzed identities, tool
  version, failed TUs and diagnostics. Mandatory missing fixtures are failures.
- Aggregate raw diagnostics across **every job and configuration**, including
  sanitizers and coverage. Retain per-job counts and source/target attribution;
  compiler option diagnostics emitted while building GoogleTest remain disclosed
  dependency diagnostics. Parallel log adjacency alone does not prove ownership.
- Keep first-party compiler/linker/librarian, dependency, platform/runner and
  unknown diagnostics separate. Review build logs in addition to process exits.
- Copy raw LLVM function/line/branch numerators and denominators without changing
  scope or subtracting branches. Emitted functions do not prove API completeness.
- Distinguish remediation, final requalification, independent review, closure and
  release. CI green does not by itself close findings or qualify a release.

## Executable gate commands

Install the small, pinned workflow-parser dependency with
`python3 -m pip install -r tools/ci/requirements.txt`.
Run `python3 tools/ci/verify_pipeline_gate.py`; this runs the AFA004 controls too.
The supported shell policy is explicit Bash `set -e[u]o pipefail` before commands,
never disabled; each PowerShell native call uses a standalone `&` invocation
immediately followed by `if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }`.
Native PowerShell pipelines may use output cmdlets only; another native command
cannot overwrite the recorded exit status. Nested shells, sourced/dynamic scripts
and failure-conditional Bash constructs are outside the audited run-block dialect.
Background commands, command substitutions, error traps and continue-on-error are
rejected. Auxiliary PowerShell statements are limited to the reviewed metadata
cmdlets and read-only expressions used by this workflow; unknown statements fail.
Unsupported/dynamic shell constructs are outside that audit dialect and must be
rejected or explicitly added with controls, not silently assumed safe.

Run the API gate with `--build-dir <actual-build>`, followed by
`tools/api_surface/verify_api_surface_gate.py --build-dir <actual-build>`. `--skip-build-exec` is a partial
source/compile control mode and must never be reported as full qualification.

Run `tools/static_analysis/verify_clang_tidy_gate.py --build-dir <actual-fresh-static-build>
--clang-tidy <clang-tidy-22>`. The build database and all mandatory controls are
required; counts are derived from actual executions, with zero silent skips.
Sanitizer gate `tools/sanitizers/verify_sanitizers_gate.py --cxx <compiler>` requires
probe-start evidence, expected violation/family, nonzero exit and a clean control.
Python gate controls run outside CTest; report their counts separately.

## MSVC diagnostic locale contract

Verified MSVC diagnostic locale: **English / VSLANG=1033**.
The qualification workflow preserves that setting. Other localized MSVC diagnostic
grammars are **outside current qualification scope**. The parser does not claim
support for all MSVC locales. Changing this environment requires new real native
log fixtures and independent grammar verification. Ordinary prose is not a
structured diagnostic; unknown structured diagnostics remain fail-closed.
