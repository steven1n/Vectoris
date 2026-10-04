# VectorisNumerics 1.0 Performance: Opening the Quaternion Trade-off

**Engineering math without surrendering performance.**

This article explains measurements of the frozen v1.0.0 release, SHA `be678b17a9ba58c9be5bca5ab59112fe74d9a83b`. It is a performance publication, not an optimization, new qualification campaign or independent correctness audit. Facts below come from the preserved [primary report](evidence/primary/VectorisNumerics_1.0.0_Performance_Report.md) and [focused quaternion investigation](evidence/focused/VectorisNumerics_1.0.0_Quaternion_Rotation_Performance_Investigation.md).

> Linux x86-64 was not measured in this publication. CPU core, clock frequency and thermal state were not locked. These measurements characterize workloads on the tested macOS Intel system. Percentile latency is not WCET.

## Why benchmark Vectoris?

Engineering mathematics asks for more than a short arithmetic expression. A vector needs a coordinate frame. A physical quantity needs a unit. A normalization operation needs a meaningful failure policy. Those requirements can affect implementation choices, and their performance consequences deserve measurement rather than slogans.

VectorisNumerics targets small, fixed-size engineering mathematics where numerical behavior, explicit semantics and predictable data structures matter alongside raw speed. This campaign considers the concrete three-dimensional and 3×3 workloads that motivated that design. It does not rank every API or establish how a large dynamic linear-algebra workload would behave.

The publication has four themes: fixed-size performance, numerical reliability, explicit engineering semantics and transparent performance trade-offs. The fourth matters because a benchmark that only displays favorable results tells users very little about choosing an implementation path. The quaternion result is therefore central to the story, despite being substantially slower than the reference paths.

## Methodology

The primary campaign used an Intel Core i9-9980HK with eight physical and sixteen logical cores, 32 GiB RAM and macOS 26.7. Compilers were Apple Clang 21.0.0 and Homebrew GCC 16.2.0. Each compiler ran O2 and O3 configurations. The tested scalar was double; Eigen 5.0.1, GLM 1.0.1 and Google Benchmark 1.9.1 were recorded dependencies.

Matching settings across libraries included C++20, `-DNDEBUG`, portable `-march=x86-64`, no fast-math, disabled FP contraction and no LTO. The machine remained a shared desktop host. Core affinity, clock frequency and thermal conditions were not locked. Matching flags improve comparison discipline, but they do not eliminate host-state uncertainty or compiler-specific code generation.

The primary suite contains 816 scenarios with thirty repetitions each: 24,480 selected measurement records. It includes dependency-chain latency and an eight-lane independent workload. The headline figures select the dependency-chain mode. Inputs are loaded at runtime, results are observed, and timing includes those costs. These are complete workload measurements, not isolated CPU instruction latencies.

Sixty-four inputs cover small, ordinary and large scales, approximately 1e-9, 1 and 1e12. A fixed seed identifies the dataset. Warm-up, controls and aggregate summaries are not counted as additional primary repetitions. A documented Core dependency control replaced 720 abs/AlmostEqual records; the original records remain in the evidence, rather than being merged into sixty repetitions per case.

The focused quaternion campaign is separate: 128 scenarios and 3,840 primary repetitions. A crucial distinction is that the original publication uses wall-time medians while the focused campaign uses CPU-time medians. We retained those definitions. Before generating these charts, every selected CSV record was cross-checked against native JSON, and all 944 scenario statistics were recalculated against the summaries. Headline rounding agrees with the reports.

## The fixed-size math results

![All four operations, including direct quaternion rotation](charts/01-fixed-size-latency.svg)

Under Clang O3, double and the dependency-chain workload, Vectoris Matrix3 × Vector3 measured 10.743 ns/op. Eigen measured 8.691, GLM 8.642 and Raw 9.338. Vectoris is slower in this particular comparison. That result remains visible, without a truncated axis or a selected subset that suggests otherwise.

For Matrix3 × Matrix3, Vectoris measured 10.888 ns/op, between Eigen's 9.480 and GLM's 12.655, while Raw measured 8.413. This supports the scoped statement that Vectoris delivers competitive fixed-size matrix performance in measured workloads. It does not support saying that Vectoris is faster than Eigen or GLM in general.

Cached RotationMatrix × Vector3 measured 9.492 ns/op for Vectoris, 9.329 for Eigen, 10.229 for GLM and 9.538 for Raw. Vectoris and Raw are comparable here: the difference is below three percent, and the uncontrolled desktop environment gives no basis for advertising a definitive tiny lead. The cached result is useful because it offers a practical alternative to direct quaternion rotation.

## The quaternion surprise

The most interesting result was not a win. Direct quaternion rotation measured 39.672 ns/op for Vectoris in the original publication workload, compared with 11.902 for Eigen, 12.979 for GLM and 12.433 for Raw. The gap was large enough to justify examining the actual public path rather than explaining it away as noise.

Separate retests of the original workload produced CPU-time medians of 39.889 and 40.102 ns/op. The focused campaign then measured the public path at 45.247 ns/op. That higher number does not replace the original 39.672 wall-time result. Retests and focused measurements have their own identities, timing clocks and harness context; all datasets remain preserved.

The investigation compared the public kernel instruction sequences in the original and focused executables. They were consistent after normalizing addresses and constant displacements. This strengthens the evidence that the same kernel was being inspected, but it does not independently isolate why the measured medians differ. We do not assign the difference to alignment, scheduling, cache state or any other unproven cause.

## Opening the 40 ns black box

![Focused quaternion measurements](charts/03-quaternion-cost-decomposition.svg)

The focused Clang O3 CPU-time medians were 45.247 ns/op for the public API, 39.225 for an independent checked reference, 12.657 for a minimal unchecked kernel, 9.664 for a cached RotationMatrix and 36.083 for converting on every call and then rotating. These comparisons distinguish execution paths; they are not a literal bill of nanoseconds for individual checks.

The checked reference preserves relevant finite-input protections, but full-domain semantic equivalence to production has not been established. Its boundary strategy and adapter behavior matter. It is therefore invalid to subtract the unchecked median and claim that an exact amount of time is “the safety cost.” The evidence supports a substantial aggregate cost associated with the protected direct path, including its implementation and generated code.

The timed ordinary inputs are within the shared comparison domain. Extreme controls demonstrate that the paths do not have identical broader guarantees. For the analytic case q = (0,1,0,0) and v = (0,DBL_MAX,0), the protected public, checked and cached paths produced the representable result (0,-DBL_MAX,0). The minimal formulas produced nonfinite intermediate-derived outputs. These untimed examples explain why replacing the protected path with a short formula would change its contract. They are not advertised as competitor defects: the input exceeds the relevant unchecked intermediate-value domain.

## What the direct path actually does

For the built-in double path, direct rotation does not invoke factory normalization or sqrt per call. Factory setup and checked conversion are different operations. The direct path recomputes a squared norm and uses it to correct nine homogeneous rotation coefficients, clamps coefficients, and accumulates products in an overflow-aware order. Special boundary handling is available when ordinary accumulation could overflow.

This is source-level behavior. The inspected Clang code also exhibited an out-of-line call, temporaries, register pressure and stack traffic. Those are compiler-specific observations; GCC generated a different shape. Static instruction sites, stack references and division instructions are evidence for further investigation, not direct models of executed cycles. Cold fallback code must not be mistaken for work performed on every ordinary input.

The direct operation assumes a valid unit quaternion and finite input vector. It is not a general factory that validates arbitrary mutated quaternion objects on every call. Explaining its numerical protection must preserve those preconditions rather than implying an unconditional checked interface.

The publication adopts the investigation's Recommendation B: retain the protected direct path, disclose its measured cost, and explain the existing cached-matrix option. No production optimization has been performed for this publication.

## Why cached matrices matter

If orientation stays fixed while many vectors are rotated, repeatedly forming and protecting the direct coefficients is avoidable at the usage level. A checked conversion can be performed once, its Result handled, and the matrix reused under its own public contract. The relevant question is how quickly that conversion cost amortizes in the user's workload.

![Measured batch totals](charts/04-direct-vs-cached-rotation.svg)

The investigation measured batches of 1, 2, 4, 8, 16, 32, 64 and 128 vectors sharing one orientation. Every cached batch includes one checked conversion. It then measures the rotations and result observation. The plotted totals are measured, and the amortized plot divides each batch median by its vector count. Neither chart extrapolates from 39.672 and 9.492.

![Measured cost per vector](charts/05-amortized-rotation-cost.svg)

For Clang O2 and O3, the cached path first won at the first tested point, one vector. GCC O2 first won at four vectors. GCC O3 first won on the median at two, with separation between cached p90 and direct p10 evident by four. These are first tested points, not interpolated thresholds. Distribution separation is descriptive, not a claim of formal statistical confidence.

The guidance is to benchmark caching when rotating one or more vectors repeatedly with the same orientation. The measurements suggest very fast amortization on this system. They do not impose “always convert to Matrix3,” especially when orientation changes every call, the workload has different dependencies, or a different compiler generates a different trade-off.

## Correctness alongside speed

Each primary build configuration recorded 6,528 independent reference-result checks and 79 supported boundary checks, all passing. These counts are independently checked from the evidence. The focused investigation also recorded passing correctness controls under O0, disabled FP contraction and ASan plus UBSan. Those controls complement timing; they are not new external certification.

Reference calculations use long double, with a pivoted Gauss–Jordan inverse reference rather than copying the tested adjugate path. Rotation references include an independent matrix formula and analytic 180-degree cases. The input/output examples preserve actual results and reference values so that readers can inspect more than a success flag.

The maximum observed Vectoris component-scaled relative error was approximately 6.42 × 10⁻¹⁶ across the tested dataset. The denominator uses a reference payload scale, with a floor. This is useful for comparing mixed-magnitude outputs, but it can permit a large relative error in an individual component close to zero. Absolute error, component scale and rounded-reference ULP distance must be interpreted together. No theoretical maximum error for the entire library is claimed.

## What the results do not prove

These measurements do not establish Linux x86-64 or Windows performance. They do not characterize every scalar, API, dimension, CPU, subnormal workload or floating-point environment. The benchmark spans three input scales; it does not supply independent latency distributions for every engineering magnitude. Unavailable APIs remain unsupported benchmark entries rather than being fabricated as zero-time results.

The p90 and p99 charts summarize thirty repetition averages. They are not individual-call tail measurements and cannot provide WCET. Within-process repetitions are not established statistically independent trials. Even a bootstrap interval would not remove systematic host drift, thermal uncertainty or differences between compiler configurations. Small differences therefore receive restrained language.

The release history also remains unchanged. v1.0.0 was released with two explicitly accepted, deferred qualification-tooling findings: AFA5-001 and AFA5-002. Candidate #15's targeted independent re-audit remained FAIL under the original zero-MAJOR policy. Owner acceptance of that debt is different from all audits passing. VRT-01 through VRT-19 are recorded closed, but this publication performs no new closure action and changes no tag or production implementation.

## What's next

Users can first select the path that fits their semantics: typed matrices for fixed-size products, or a checked cached RotationMatrix when orientation reuse warrants it. Then measure that choice with realistic inputs, dependencies and an explicit error budget. For aerospace, orbital and estimation workloads, double is recommended unless such a budget justifies float; supporting float is not a blanket suitability claim.

The publication package contains editable light and dark SVGs, the full evidence bundles, copied environment manifests, input/output examples, derived CSVs and an offline generator. Future measurements on Linux or other hardware should produce separate datasets and new environment records. Potential implementation optimization needs its own authorization, correctness evidence and measurement campaign. None is started here.

[Reproduction instructions](methodology-and-evidence.md#offline-reproduction), [evidence index](methodology-and-evidence.md), [claim audit](claim-audit.md) and the [main performance page](README.md) provide the next level of detail. The aim is useful engineering evidence: competitive matrix workloads, a visible quaternion trade-off and a practical cached alternative, with the limits left in view.
