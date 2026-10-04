# Public claim audit

Decision rule: YES approves the scoped wording shown here. NO identifies rejected alternatives, included for review only; they are not approved marketing copy. All published headline claims must be YES. Measurements, interpretation and usage recommendations are kept distinct. Numerical comparisons below use rounded labels; chart data uses full precision.

Source shorthand: [primary report](evidence/primary/VectorisNumerics_1.0.0_Performance_Report.md), [focused report](evidence/focused/VectorisNumerics_1.0.0_Quaternion_Rotation_Performance_Investigation.md), [derived data](data/provenance.json) and [verification](data/verification.json). Complete native data and assembly are available in the evidence bundles.

| Public claim | Evidence source | Exact data | Scope | Caveat | Approved |
| --- | --- | --- | --- | --- | --- |
| Competitive fixed-size matrix performance in measured macOS Intel workloads. | primary report: Results; headline-results.csv | Matrix-vector 10.743; matrix-matrix 10.888 ns/op for Vectoris | Clang O3 / double / dependency chain / wall-time | Matrix-vector trails all references here; not a universal win. | YES |
| Cached RotationMatrix closely tracks Raw. | primary report: Results; headline-results.csv | 9.492 vs 9.538 ns/op | macOS Intel / Clang O3 / double / wall-time | Below 3%; comparable, not a definitive lead. | YES |
| Direct quaternion rotation has substantial additional cost relative to minimal arithmetic paths. | both reports; headline and decomposition CSV | Original Vectoris 39.672 vs Raw 12.433; focused Public 45.247 vs Minimal 12.657 | Separate original wall-time and focused CPU-time harnesses | Safety semantics differ; cannot isolate an individual mechanism. | YES |
| Caching can amortize quickly for repeated orientation on this system. | focused report: batch amortization; rotation-amortization.csv | First median win: Clang O2/O3 N=1; GCC O2 N=4; GCC O3 N=2 | Measured N={1,2,4,8,16,32,64,128}, CPU-time | GCC O3 p10–p90 separation N=4; no universal threshold. | YES |
| Measurements were paired with independent reference and boundary checks. | primary raw validation/reliability; verification.json | 6,528 reference-result and 79 supported boundary checks per configuration | Four measured configurations | Finite sampled checks, not full-domain proof. | YES |
| 24,480 primary measurements. | primary raw.csv + native JSON; verification.json | 816 × 30 = 24,480 | Selected primary repetitions | 720 Core replacements replace original rows; no double counting. | YES |
| 3,840 focused measurements. | focused raw.csv + native JSON; verification.json | 128 × 30 = 3,840 | Focused-primary repetitions | Controls, warmup and retests are separate. | YES |
| Maximum observed scaled relative error was approximately 6.42e-16 in the tested dataset. | primary accuracy.csv and native validation JSONL | 6.419863297741686e-16 | Observed Vectoris outputs across four configurations | Component error / payload scale; near-zero relative component errors can be masked. | YES |
| Original quaternion value remains 39.672; retests 39.889 and 40.102. | primary report plus focused reproduction/controls JSON | 39.672 wall-time; 39.889 / 40.102 CPU-time | Original workload and separate retests | Not interchangeable statistics; no overwrite. | YES |
| Focused public-path median is 45.247. | focused raw.csv; quaternion-decomposition.csv | 45.247 CPU ns/op | Focused Clang O3 / double / dependency chain | Higher timing not independently isolated, despite consistent kernel instruction sequence. | YES |
| No per-call factory normalization or sqrt in built-in double direct rotation. | focused report: actual source path and assembly | Factory setup separate; squared-norm correction and nine divisions remain | Source semantics: valid unit quaternion + finite vector | Does not imply zero norm correction or arbitrary-invalid-input validation. | YES |
| Clang generated calls, register pressure and stack traffic. | focused report: assembly inspection; original assembly in ZIP | Out-of-line public body; temporaries and stack references | Inspected Clang executable only | Not a portable instruction/cycle guarantee or proven cause of timing delta. | YES |
| Quaternion controls passed O0, contraction-off and ASan+UBSan. | focused analysis-checks/final verification + logs in ZIP | 884 numerical checks/config; O0 and combined control failed=0 | Recorded focused correctness controls | Not a fresh independent audit in this publication. | YES |
| Linux performance was not measured. | primary hardware manifest and both reports | Linux NOT RUN | This publication | Cross-platform qualification CI is separate evidence. | YES |
| Release is unchanged and qualification debt remains deferred. | frozen release notes/closure; Git identity check | v1.0.0 be678b17…; AFA5-001/002 OPEN-DEFERRED | Release context only | C15 independent re-audit FAIL preserved. | YES |
| Vectoris quaternion rotation is faster than Eigen. | headline-results.csv | 39.672 vs 11.902 ns/op | Clang O3 / double | Contradicted by this measurement; not used. | NO |
| Vectoris has a maximum numerical error of 6.42e-16. | accuracy.csv | Observed sample maximum only | Tested dataset | Replace with maximum observed component-scaled error in tested dataset. | NO |
| Vectoris spends exactly 27 ns on safety. | quaternion-decomposition.csv | Aggregate path medians only | Focused harness | Full-domain reference equivalence and isolated check costs not established. | NO |
| p99 is WCET. | raw repetitions and methodology | p99 of 30 repetition means | Local observed distribution | Not single-call tails or worst-case execution time. | NO |
| All independent audits passed / zero known findings. | release notes/closure | C15 FAIL; two accepted-deferred MAJOR qualification findings | Release history | False; explicitly excluded from publication claims. | NO |

## Statistical and ownership boundaries

Under-three-percent differences are described as comparable. Percentages in the derived comparison CSV are computed from full-precision medians using the specified formula; they are descriptive and not claims of statistically definitive superiority. No global performance score, radar chart or universal library ranking is created.

Recorded numerical/reference controls are evidence of tested inputs. They do not establish formal verification, certification or a universal error bound. Original publication, retest and focused figures retain distinct timing clocks and harness identities.

v1.0.0 was released by owner acceptance of two deferred qualification-tooling findings; this publication does not reverse C15's independent audit FAIL or close AFA5 debt. This post-release documentation integration does not alter that historical disposition.
