# VectorisNumerics 1.0.0 formal closure and owner release decision

Date: 2026-10-03
Base: C15 `3e7132df0b7e6f1cd0c62f33a2a433ce97670e4b`
CI: [37109622562](https://github.com/steven1n/Vectoris/actions/runs/37109622562)

## Authority and preserved history

Formal closure and release proceed under the release owner's explicit
authorization. This record is not a new independent audit or requalification.
C10 CI PASS / independent audit FAIL; C11 CI FAIL; C12 CI FAIL; C13 local + CI PASS /
independent audit FAIL; C14 local + CI PASS / independent re-audit FAIL; C15 local +
CI PASS / independent re-audit FAIL remain historical facts. C15 independent
result is **FAIL: 0 BLOCKER, 2 MAJOR, 0 MINOR, 0 OBSERVATION**.

Release Smoke Attempt #1 remains ABORTED due to invalid external AlmostEqual
probe authoring; no C15 production defect established. Attempt #2 PASS is
separate evidence and does not rewrite Attempt #1.

## Formal VRT closure

**VRT-01 THROUGH VRT-19: CLOSED.** Closure reason for every row:
**Substantive remediation evidence independently established and preserved
through Candidate #15.** This closes the original scoped findings; it does not
assert that all qualification tooling is defect-free.

Evidence chain: original VRT-01–19 remediation/test evidence in the historical
[ledger](https://github.com/steven1n/Vectoris/blob/v1.0.0/docs/audits/Vectoris_Red_Team_Remediation_2026-09-20.md); independent C13
release-audit §9 substantive repair table; C14 targeted review; C15 production
preservation; exact-SHA C15 CI; fresh Attempt #2 regression execution. External
audit reports retain their original FAIL verdicts.

All 69 public production headers (58 Numerics + 11 Dynamics) are byte-identical
C14→C15. This release changes only metadata/closure records; no numerical
implementation, tests, qualification parser, workflow or warning policy changes.

| Finding | Formal state | Substantive repair / regression evidence |
| --- | --- | --- |
| VRT-01 | CLOSED | Unit quaternion normalization/conversion extreme/signed/nonfinite contracts; Fraction campaign and fresh tests |
| VRT-02 | CLOSED | UnitVector scale-safe normalization/preconditions; independent edge sweep and fresh tests |
| VRT-03 | CLOSED | Result factory forwarding/state construction; fresh Result tests/source review |
| VRT-04 | CLOSED | Matrix3 × Vector3 public instantiation and frame-preserving multiplication; original Step 4 and Matrix3PublicMultiplicationTest regressions. C13 numerical inverse review is additional evidence, not the original finding identity. |
| VRT-05 | CLOSED | Real sanitizer diagnostics plus nonzero required; fresh negatives and native three sanitizer configurations |
| VRT-06 | CLOSED | Pipeline failure propagation; own Bash/Pwsh attacks, actual workflow |
| VRT-07 | CLOSED | Transactional Dynamics invalid-input/output rejection; actual46 Dynamics test bodies and source review |
| VRT-08 | CLOSED | LDLT solve-not-invert/scaled SPD policy; independent extreme diagonal sweep and fresh solver regressions |
| VRT-09 | CLOSED | AlmostEqual opposite-sign/tiny/invalid-tolerance semantics; new edge sweep and fresh tolerance tests |
| VRT-10 | CLOSED | API surface exact header/positive/negative matrix and body equality; independent fail-closed attacks |
| VRT-11 | CLOSED | clang-tidy exact TU/diagnostic accountability; independently derived117 set and mandatory controls |
| VRT-12 | CLOSED | Canonical/compatibility namespaces/selective aliases; namespace tests and standalone/order poison compilation |
| VRT-13 | CLOSED | Result no third/valueless state, transactional replacement/error observation; fresh Result tests and source contract review |
| VRT-14 | CLOSED | Supported sqrt scalar constraints and canonical/compatibility semantics; new1024 constexpr and400,000 runtime comparisons |
| VRT-15 | CLOSED | Matrix representation/layout policy scoped to actual traits, no unproven portable ABI guarantee; fresh trait/layout tests/source review |
| VRT-16 | CLOSED | Angle units / RotationalCross dimensions and frames; full typed Dynamics/compile-time API tests |
| VRT-17 | CLOSED | Identity factory cannot spoof cross-frame class tags; negative API compile probes/fresh tests |
| VRT-18 | CLOSED | C++20 interface propagation with BUILD_TESTING off; independent Numerics/Dynamics consumers |
| VRT-19 | CLOSED | Declared coverage/API scope and raw thresholds; fresh exports, denominator reconstruction, public-template execution and gates |

## AFA disposition — no fictitious zero-open-findings claim

| Finding / explicitly bounded scope | Disposition |
| --- | --- |
| AFA-001–007 historical substantive repairs | VERIFIED REMEDIATED, preserved through later independent review |
| AFA-008 English MSVC qualification locale, VSLANG=1033 | ACCEPTED SCOPE |
| AFA2-001 typed rotation; 002 generic Geometry exceptions; 003 executed API identity gate; 004 shell failure auditing; 005 relative MSVC ownership | VERIFIED REMEDIATED |
| C10 AFA2-006 historical fixture integrity | INDEPENDENTLY VERIFIED REMEDIATED (C14 review) |
| C10 AFA2-011 README | INDEPENDENTLY VERIFIED REMEDIATED (C14 review); release-facing status subsequently updated as metadata |
| C10 AFA2-012 timing wording | INDEPENDENTLY VERIFIED REMEDIATED (C14 review) |
| AFA2-009 RotationMatrix finite cancellation | INDEPENDENTLY VERIFIED REMEDIATED; preserved |
| AFA2-010 Matrix3 integer inverse / INT_MIN domain | INDEPENDENTLY VERIFIED REMEDIATED; preserved |
| C13 AFA2-011 nonfinite propagation | INDEPENDENTLY VERIFIED REMEDIATED; preserved |
| AFA2-007 tiny component product loss; AFA2-008 performance/subnormal timing; AFA3-002 stored-coefficient path divergence; AFA3-003 custom Core abs boundary | ACCEPTED SCOPE; no expansion of guarantees |
| AFA3-001 actual Core include contract correction (Option B), canonical docs and working consumers | VERIFIED REMEDIATED for this bounded repair; full qualification-oracle assurance withheld by AFA5 |
| AFA4-001 original two-space duplicate counterexample / parser hardening | VERIFIED REMEDIATED for this bounded counterexample; no claim of comprehensive zero-MAJOR oracle |
| AFA5-001 consumer API execution proof can fail open | **OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT**; post-1.0 qualification hardening |
| AFA5-002 indented Markdown code block can appear as Public Headers table | **OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT**; post-1.0 qualification hardening |

Historical repaired findings retain their existing independent disposition.
No blanket independently verified status is assigned to the unresolved AFA5
findings or to the complete AFA3/AFA4 qualification oracle.

## VectorisNumerics 1.0 release policy override

**RELEASE DECISION: OWNER ACCEPTED RISK.**

- Runtime / numerical release BLOCKER: 0.
- Runtime / numerical MAJOR: 0.
- Known unresolved qualification-tooling MAJOR debt: 2 (AFA5-001, AFA5-002).
- Owner decision: accepted for v1.0.0, deferred to post-1.0 tooling hardening.
- Reason: qualification-tooling robustness defects, not demonstrated runtime
  numerical defects. This classification does not erase their MAJOR severity.
- Original independent audit verdict remains FAIL. No claim is made that C15
  passed its targeted independent re-audit or the original zero-MAJOR policy.

## Evidence custody

External reports reviewed (not copied into the release tree):

- `Vectoris_Candidate_13_Independent_Release_Audit.md`, SHA-256 `80bbd8a449d2f5ba1923299611e66de6c45b39cfab31dec4cd922fd424b2e747`.
- `Vectoris_Candidate_14_Targeted_Independent_Release_ReAudit.md`, SHA-256 `07b83cbf75a6f3fbd80c371dce555c0783d21ac619a7d525e2667b25d5231a27`.
- `Vectoris_Candidate_15_Targeted_Independent_Release_ReAudit.md`, SHA-256 `d9aa5a201747df674f1f19066cdef21343f365a50121d3d5e3ec9ddeff4d1fbd`.

Original ledger entries and audit verdicts are retained unchanged. The
Attempt #2 report outside Git records raw tests, probe sources, header hashes,
publication SHA/tag and provenance. The release-metadata commit is distinct
from C15; old exact-SHA CI is not relabeled as CI on the release commit.

Numerics 1.0 disposition and Dynamics module maturity remain separate. There
is no external certification, MC/DC/WCET qualification or formal verification
claim. AFA5 remediation and any new module require separate authorization.
