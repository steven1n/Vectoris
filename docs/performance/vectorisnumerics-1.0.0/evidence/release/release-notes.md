# VectorisNumerics 1.0.0

Date: 2026-10-03
Base: Candidate #15, `3e7132df0b7e6f1cd0c62f33a2a433ce97670e4b`
Release identity: annotated `v1.0.0` tag; the tag identifies the release metadata
commit. Production headers are byte-identical to C15.

## Highlights

- ISO C++20 numerical infrastructure with typed units and coordinate frames.
- Matrix3, Quaternion, RotationMatrix3 and related geometry.
- Numerical reliability hardening and preserved historical repairs.
- GCC, Linux Clang and Windows MSVC cross-platform qualification.
- ASan, UBSan, clang-tidy, raw LLVM coverage and HeaderIsolation evidence.

## Qualification and smoke

Production numerical/runtime qualification is accepted for 1.0 by the owner.
C15 local validation and exact-SHA CI run
[37109622562](https://github.com/steven1n/Vectoris/actions/runs/37109622562) passed.
Historical numerical release blockers were independently remediated and
preserved. C15 CI measured emitted functions 229/229, lines 1249/1290 and RAW
branches 454/484. These are C15 CI metrics, not a fresh release-commit coverage
run or proof of every template/API instantiation.

Release Smoke Attempt #1: ABORTED, invalid external consumer probe (two-argument
AlmostEqual invocation). C15 production defect: NOT ESTABLISHED. The failed
probe and original report are retained outside Git.

Release Smoke Attempt #2: PASS. Fresh Release 425/425, Debug 427/427, no
failures/skips; both HeaderIsolation targets; 36 focused regressions; executed
BUILD_TESTING=OFF Numerics, Dynamics, MathFunctions and AlmostEqual consumers.
AlmostEqual uses required absolute and relative tolerances, both double
`1.0e-12`. A separate staged-header consumer uses only an external include
prefix. No production implementation was changed for release. The release
report separately records post-metadata smoke and published SHA/tag identity.

## Owner-accepted qualification debt — still open

Candidate #15 targeted independent release re-audit **did NOT pass** under the
original zero-MAJOR policy: FAIL, 0 BLOCKER, 2 MAJOR, 0 MINOR, 0 OBSERVATION.

- **AFA5-001: OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT.** Consumer API
  execution proof in qualification tooling can fail open.
- **AFA5-002: OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT.** An indented Markdown
  code block can be misidentified as the Public Headers table.

Both findings concern qualification-tooling fail-closed robustness, not a
demonstrated VectorisNumerics runtime numerical defect. They remain MAJOR
qualification debt, targeted for post-1.0 qualification hardening. They are
not described as fixed, closed or independently verified.

**RELEASE DECISION: OWNER ACCEPTED RISK.** Runtime/numerical release BLOCKER: 0;
runtime/numerical MAJOR: 0; known accepted qualification-tooling MAJOR debt: 2.
This is an explicit release-policy override, not a reversal of the independent
audit FAIL and not evidence of zero known MAJOR findings.

## Consumption and supported scope

C++20 headers and source-tree CMake targets `Vectoris::Numerics` and downstream
`Vectoris::Dynamics` are supported. BUILD_TESTING=OFF consumers do not require
GoogleTest or qualification Python. No CMake install rules, exported config
package, or `find_package(Vectoris)` support is provided in this version.
Staging copies of public headers is distinguished from an installed package.
VectorisDynamics is smoke-tested downstream; it is not released as Dynamics 1.0.

Use `Core/MathFunctions.h` for `core::Math` wrappers. `Core/Math.h` provides
primitives; it does not promise `Math::sin`. AlmostEqual requires four arguments:
`AlmostEqual(a, b, absoluteTolerance, relativeTolerance)`.

Determinism is bounded to a fixed implementation, toolchain, floating-point
environment, configuration, algorithm and seed. Cross-toolchain/hardware
bitwise identity is not guaranteed. `double` is recommended for aerospace,
orbital and estimation work unless an explicit error budget justifies `float`.
There is no external safety certification, formal verification, MC/DC
qualification or demonstrated WCET claim.

See [formal closure](formal-closure.md). Accepted AFA5 debt is not
part of the closed VRT numerical-remediation body.
