# Evidence index and publication provenance

All numbers refer to frozen VectorisNumerics v1.0.0, release SHA `be678b17a9ba58c9be5bca5ab59112fe74d9a83b`. Links below are repository-relative. Complete original archives are not hosted here; see archive availability below.

## Primary performance dataset

- [Full report](evidence/primary/VectorisNumerics_1.0.0_Performance_Report.md)
- [Original offline explorer](evidence/primary/performance.html)
- [Selected raw CSV](evidence/primary/raw.csv) / [reported summary](evidence/primary/summary.csv)
- [Input/output examples](evidence/primary/input-output-examples.csv), 306 records, three scales across 102 operation/library pairs
- [Accuracy records](evidence/primary/accuracy.csv), 26,112 checks across four configurations
- [Operation availability manifest](evidence/primary/operation_manifest.json)
- [Complete original evidence ZIP](methodology-and-evidence.md#complete-archive-availability)

Primary ZIP SHA-256: `96a3e59994e7156fb3568a617ab456bd7d1476585b7f46612945f08a3178bb7e`.

Selected dataset: 816 scenarios × 30 = 24,480 records. Primary statistic: median **wall-time** ns/op. Native JSON is preserved in the offline original archives under `raw/<configuration>/timing.json`; those native files are not included in this public tree. The six Core latency cases per configuration select `core-dependency-controls/<configuration>-timing.json` instead. Those 720 replacements are not added to superseded records. Preliminary experiments and controls are retained in the ZIP, outside the selected denominator.

## Quaternion investigation dataset

- [Focused report](evidence/focused/VectorisNumerics_1.0.0_Quaternion_Rotation_Performance_Investigation.md)
- [Selected raw CSV](evidence/focused/raw.csv) / [reported summary](evidence/focused/summary.csv)
- [Recorded break-even](evidence/focused/break-even.json)
- [Correctness controls](evidence/focused/analysis-checks.json) / [final verification record](evidence/focused/final-verification.json)
- [Complete original evidence ZIP](methodology-and-evidence.md#complete-archive-availability)

Focused ZIP SHA-256: `fc65d3c6c05bdc1c484b7c7d3959c44ee9f23879b173b98def15223dd524606a`.

Selected dataset: 128 scenarios × 30 = 3,840 records. Primary statistic: median **CPU-time** ns/op; batch records are total CPU ns/batch. The isolated public median is 45.247. The original headline 39.672 is retained in the primary dataset. Reproduction/control JSON is preserved separately in the offline focused archive; it is not merged into primary repetitions. Public retest summaries are in [history controls](data/quaternion-history-controls.csv).

## Environment and method

- [Primary hardware manifest](evidence/primary/hardware.json)
- [Primary software/dependency versions](evidence/primary/software.json)
- [Primary build records](evidence/primary/builds.json)
- [Primary timing provenance](evidence/primary/timing-provenance.json)
- [Focused environment manifest](evidence/focused/environment.json)
- [Focused timing provenance](evidence/focused/timing-provenance.json)
- [Frozen Engineering Standard](evidence/release/ENGINEERING_STANDARD_V1.md), especially reproducible-research / benchmark / floating-point sections

The full ZIPs preserve measurement logs, source snapshots, assembly and environment observations. Copied reports retain their original wording and charts. This package adds a separate public interpretation; it does not revise those reports.

## Derived publication data

- [Headline results](data/headline-results.csv), Clang O3 / double / dependency chain / wall-time / four libraries
- [Focused path comparison](data/quaternion-decomposition.csv), Clang O3 / double / dependency chain / CPU-time
- [Measured batch amortization](data/rotation-amortization.csv), four compiler configurations, N=1…128 / CPU-time
- [Compiler comparison](data/compiler-results.csv), four operations, primary wall-time / Vectoris only
- [Latency distribution](data/latency-distribution.csv), primary wall-time / Vectoris / median, p90, p99
- [Quaternion history controls](data/quaternion-history-controls.csv), original-workload CPU-time retests, separate records
- [Programmatic descriptive percentages](data/descriptive-comparisons.csv), `(reference − Vectoris) / reference × 100%`; negative means higher Vectoris latency
- [Verification](data/verification.json) / [per-chart provenance](data/provenance.json)

Percentiles use linear interpolation over thirty repetition means. Raw timings, not rounded headline values, feed every chart. Values are rounded only for labels. Code-generation observations do not establish isolated per-check time costs.

## Release and accepted debt

- [Frozen release notes](evidence/release/release-notes.md)
- [Frozen formal closure record](evidence/release/formal-closure.md)

The release records preserve Release 425/425, Debug 427/427 and 36 focused historical regressions PASS. VRT-01 through VRT-19 are CLOSED in those records. No release tests or independent audit were rerun for this publication.

v1.0.0 was released with **two explicitly accepted, deferred qualification-tooling findings: AFA5-001 and AFA5-002**. Both remain OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT. C15's targeted independent release re-audit remains FAIL under the original zero-MAJOR policy. Performance publication does not alter qualification, findings or tag identity.

## Offline reproduction

The [chart generator](generate_publication_charts.py) reads only the published derived CSV/JSON. With Python 3, Matplotlib 3.11.2 and NumPy 2.5.3, copy this directory into a separate scratch directory and run `python3 generate_publication_charts.py` there. It has no network access. Do not overwrite the historical publication assets. Versions are recorded in [provenance](data/provenance.json); rendering across different dependency versions is not promised bit-identical.

The public selected raw CSVs preserve all 24,480 primary and 3,840 focused repetition records without changing any field. They allow independent recalculation of the scoped medians and percentiles. Native JSON, benchmark source snapshots, assembly and full measurement logs remain in the original offline archives. Full campaign reconstruction requires those archives and a new output directory; this public subset is not a complete rebuildable harness distribution.

## Complete archive availability

The approved publication archive is **not uploaded** in this integration: it contains private local paths and host metadata. The original files remain unchanged. No public download URL is claimed for either complete original evidence archive. The archive links from the approved offline package resolve here to explain that availability boundary.

| Preserved original archive | SHA-256 | Public hosting |
| --- | --- | --- |
| Primary benchmark evidence | `96a3e59994e7156fb3568a617ab456bd7d1476585b7f46612945f08a3178bb7e` | Not uploaded |
| Quaternion investigation evidence | `fc65d3c6c05bdc1c484b7c7d3959c44ee9f23879b173b98def15223dd524606a` | Not uploaded |
| Approved complete publication package | `5577c58812268892d0aaefb908c510ebc93bcfc1b95f3bb31ac9c41fadb31770` | Not uploaded |

In the public report copies, local worktree/image links are replaced by repository-relative paths or the frozen v1.0.0 source URL. The hardware manifest omits the personal hostname; build and timing command paths use `$BENCHMARK_ROOT`. Numerical records, timestamps, versions, flags and measured statistics are unchanged. Repository copies normalize CSV line endings to LF and remove nonsemantic SVG trailing whitespace for Git compatibility; the original files are preserved unchanged. [Integration file provenance](data/integration-copy-manifest.json) records original and public hashes and these replacements. Source-hash snapshot and native-file references in the original chart provenance identify offline archive records, not additional files claimed to be hosted here.

## Light and dark chart variants

- [00-benchmark-at-a-glance light](charts/00-benchmark-at-a-glance.svg) / [dark](charts/00-benchmark-at-a-glance-dark.svg)
- [01-fixed-size-latency light](charts/01-fixed-size-latency.svg) / [dark](charts/01-fixed-size-latency-dark.svg)
- [02-matrix-hot-path light](charts/02-matrix-hot-path.svg) / [dark](charts/02-matrix-hot-path-dark.svg)
- [03-quaternion-cost-decomposition light](charts/03-quaternion-cost-decomposition.svg) / [dark](charts/03-quaternion-cost-decomposition-dark.svg)
- [04-direct-vs-cached-rotation light](charts/04-direct-vs-cached-rotation.svg) / [dark](charts/04-direct-vs-cached-rotation-dark.svg)
- [05-amortized-rotation-cost light](charts/05-amortized-rotation-cost.svg) / [dark](charts/05-amortized-rotation-cost-dark.svg)
- [06-compiler-optimization-comparison light](charts/06-compiler-optimization-comparison.svg) / [dark](charts/06-compiler-optimization-comparison-dark.svg)
- [07-latency-distribution light](charts/07-latency-distribution.svg) / [dark](charts/07-latency-distribution-dark.svg)
- [08-correctness-evidence light](charts/08-correctness-evidence.svg) / [dark](charts/08-correctness-evidence-dark.svg)
- [09-social-performance-summary light](charts/09-social-performance-summary.svg) / [dark](charts/09-social-performance-summary-dark.svg)

Benchmark results are workload-, compiler-, hardware-, and configuration-dependent and should not be interpreted as universal performance rankings. Performance benchmarks do not substitute for numerical correctness or robustness validation. Percentile latency is not WCET.
