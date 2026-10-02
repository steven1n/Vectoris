# Candidate #8 independent-audit MAJOR remediation

## Scope and acceptance contract

This uncommitted follow-up starts at Candidate #8
`147731efa0ff6ea14de52ceba456e54c384cd219`. Candidate #8's independent-audit
failure remains historical fact. This work addresses AFA2-001 through AFA2-005
only; it does not close findings, qualify a new candidate, or authorize a freeze.

- **AFA2-001:** dimensioned quaternion rotation must use the same overflow-safe
  floating rotation kernel as Geometry, preserving quantity type and source /
  destination frames. Analytic half-turn results at finite maximum magnitudes
  must remain finite; downstream Euler integration must commit representable
  finite state updates. No separate expanded cross-product rotation formula.
- **AFA2-002:** `ScalarArithmetic` does not imply non-throwing construction,
  comparison, arithmetic or copying. Composite Geometry APIs guarantee
  `noexcept` for built-in arithmetic scalar operands; custom scalar composites
  conservatively allow exceptions, including from result construction. Existing
  Vector3 expression-based specifications remain. Reference-only observers and
  floating-only helpers remain non-throwing. Implicit special members retain
  compiler-inferred exception specifications. By-value component constructors
  use the existing Vector3 reference-preserving / move-if-nothrow transfer model;
  potentially throwing result copying remains part of the composite policy.
  This preserves custom scalar
  support and layouts, but changes exception traits for custom specializations.
- **AFA2-003:** required, source-defined, registered, actually executed and passed
  identities must agree. CTest exit status / registration labels are insufficient.
  Each positive probe must produce a fresh GoogleTest execution report containing
  the exact required body, run and completed without failure or skip. The same
  rule covers declared Dynamics positive probes. Empty commands, zero-test filters,
  missing/malformed reports and wrong identities fail closed.
- **AFA2-004:** the audited workflow uses a bounded shell dialect. Bash must retain
  its initial fail-fast options: later `set`, `shopt`, or option-changing wrappers
  are rejected. PowerShell here-string openers must be standalone simple variable
  assignments; native commands cannot be hidden on their discarded opener line.
  Real workflow positive controls and masking negative controls must execute.
- **AFA2-005:** diagnostic project provenance must resolve source-relative and
  build-relative paths before ownership classification. First-party project
  context takes precedence over dependency header origin. Unresolved project
  context fails closed instead of falling back to allowed dependency ownership.
  English MSVC diagnostic grammar / VSLANG=1033 scope remains unchanged.

No numerical algorithms outside the typed rotation adapter, warning flags,
coverage denominators, namespace/frame/unit contracts or historical conclusions
are changed. Local execution evidence is reported separately from pending
cross-platform CI and independent review.

## Historical pre-C9 local validation snapshot (2026-10-02)

AFA2-001 through AFA2-005: **IMPLEMENTED LOCALLY; local validation PASS;
PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.

- 64 new registered C++ regressions: 58 generic-Geometry tests and 6 typed-rotation /
  Euler tests. Debug 375/375; Release 373/373; ASan, UBSan and combined each 375/375.
- Both HeaderIsolation targets and namespace/include-order regressions pass.
- API gate: 19 exact positive bodies executed and passed; 12 negative compile
  probes; 17 legacy gate controls and 12 actual GoogleTest identity controls pass.
- Workflow auditor: 23 self-tests and 31 actual workflow run blocks pass; masking
  policy mutations and here-string opener attacks are rejected.
- MSVC diagnostic gate: 106 self-tests pass, including relative-project full CLI
  attacks. These are local parser fixtures, not native Windows CI qualification.
- clang-tidy 22: expected = eligible = analyzed = 117; failed TUs = 0;
  first-party diagnostics = 0. Gate self-tests: 24 executed / 24 passed / 0 skipped.
- Fresh raw LLVM coverage: functions 219/219; lines 1146/1188; branches 412/453.
  Coverage scope, thresholds and denominators are unchanged. Emitted-function
  coverage does not establish completeness of every public template instantiation.
- Strict local builds have zero warnings; sanitizer negative controls and
  BUILD_TESTING=OFF external consumers pass. `git diff --check` passes.

Frozen Candidate #8 and its audit-failure history are preserved. No new candidate
has been committed or pushed; updated exact-SHA GCC / Linux Clang / MSVC CI and
independent review remain pending. AFA2-006 MINOR and the audit observations are
outside this patch. Overall: **NOT FINALIZED / Experimental**.

## Candidate #9 freeze scope

Candidate #9 inherits the frozen C8 SHA above. The local snapshot above records
the uncommitted post-C8 stage; it is not the final C9 qualification verdict.
The C9 qualification report outside Git binds rerun evidence and new CI to the
exact committed candidate SHA. No independent review, finding closure or
release authorization is part of this freeze. README and pure-math scope
clarifications distinguish historical audit failure, the new candidate, scoped
determinism, scalar guidance and absence of external certification.

The six new rotation/Euler regressions cover extremes and ordinary scales, but
do not form a systematic registered precision campaign spanning 1e-9 to 1e12.
Operational-domain precision therefore remains **RELEASE FOLLOW-UP / NUMERICAL
CAMPAIGN REQUIRED**; external spot checks, if recorded, do not replace it.
