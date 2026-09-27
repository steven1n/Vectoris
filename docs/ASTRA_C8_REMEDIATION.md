# Candidate #8 — Astra finding remediation

Authorized base: `065d9bbf50dbba2cfcda1cf75a83f63c039d899c` (Candidate #7).
C7's historical **FINAL REQUALIFICATION PASS / INDEPENDENT AUDIT FAILED** remains.
The independent audit recorded 0 BLOCKER, 5 MAJOR, 2 MINOR and 1 OBSERVATION.
VRT-01 through VRT-19 remain **NOT CLOSED**; Numerics 1.0 is not eligible for freeze.

| Finding | Scope | Local state |
|---|---|---|
| AFA-001 | Bounded and compensated Quaternion rotation; analytic/extreme/Rodrigues/boundary controls | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-002 | Truthful conditional Vector3 noexcept; throwing-scalar controls | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-003 | Exact required/source/registered/executed/passed API identities | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-004 | Structured YAML run blocks and per-native-call propagation policy | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-005 | Explicit fresh build/tool inputs; actual mandatory self-test accounting | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-006 | Probe-start + expected sanitizer family/violation + nonzero exit | FIXED + LOCALLY VERIFIED; pending independent verification |
| AFA-007 | External C7 erratum; source/log report checklist | CORRECTED + VERIFIED; pending independent verification |
| AFA-008 | English VSLANG=1033 diagnostic grammar scope | Documented; pending independent confirmation |

See [qualification evidence checklist](qualification-evidence-checklist.md) for
gate commands and evidence reporting boundaries. No finding is closed by this
candidate. Final requalification and independent audit #2 are separate stages.

Local evidence: Debug 311/311 (C7 299, +12), Release 309/309 (C7 297, +12),
ASan/UBSan/combined 311/311 each. HeaderIsolation 60 Numerics / 13 Dynamics TUs.
API required/source/registered/executed/passed identities: 19 exact matches.
clang-tidy 22: expected=eligible=analyzed=117, failed=0, diagnostics=0; mandatory
self-tests executed=24, passed=24, failed=0, skipped=0. Local LLVM21 raw coverage:
functions 210/210, lines 1111/1150, branches 412/453 (unadjusted denominators).
API gate controls: 17 legacy + 9 identity controls; pipeline: 16 controls / 31 audited
run blocks; sanitizer classifier: 11 controls plus real native probes; warning
parser: 103 controls. These Python controls are separate from CTest.
External BUILD_TESTING=OFF Numerics-only and Dynamics consumers passed, CTest=0,
no GoogleTest materialization. No warning policy, coverage scope or threshold was
changed. Exact-SHA cross-platform CI is pending at candidate creation.

Informational ordinary Quaternion×Vector3 measurement on the same local Intel
Core i9-9980HK / Apple Clang21 -O3 host: C7 median 3.570 ns/op; C8 26.146 ns/op
(200000 iterations/sample, 9 samples, identical seed/checksum). This is a measured
correctness cost (~7.3x), not a performance improvement or a qualification threshold.
The existing Matrix3 benchmark was also rerun. Future performance work requires
separate authorization and must preserve the extreme-scale correctness contract.
