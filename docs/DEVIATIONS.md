# Vectoris Formal Approved Deviation Ledger

> [!IMPORTANT]
> **Document**: Formal Approved Deviation Ledger  
> **Document Version**: 1.2
> **Status**: Authoritative Governance Document  
> **Reviewed Starting HEAD**: `cfbecc6390fb30d857f10f2516f39bb2ef75f996` (working tree contains uncommitted remediations)
> **Last Updated**: 2026-09-23
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) Sections 101 & 102

> **Current Project Status**: NOT REQUALIFIED / Experimental. This ledger records scoped deviations; it is not a qualification report.

---

## 1. Overview & Governance

### 1.1 Documentation Authority Model & Scoped Precedence
- **Default Normative Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) is the single source of truth (SSOT) for all normative engineering, mathematical, and architectural requirements.
- **Authorized Scoped Exception Mechanism**: This document (`docs/DEVIATIONS.md`) is the Engineering Standard's formally authorized scoped exception mechanism governed by Sections 101 & 102.
- **Precedence**:
  - Within an explicitly registered deviation scope, the applicable registered deviation modifies or overrides **only** the cited normative rule for the stated component, scope, and version lifetime.
  - Outside that explicitly registered scope, the Engineering Standard remains fully and strictly authoritative.
  - Project conventions ([`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md)), module specifications ([`docs/core.md`](core.md), [`docs/units.md`](units.md), [`docs/geometry.md`](geometry.md), [`modules/VectorisDynamics/docs/dynamics.md`](../modules/VectorisDynamics/docs/dynamics.md)), and repository overviews ([`README.md`](../README.md)) must strictly conform to the **Engineering Standard plus applicable registered deviations**.
  - Audit documents ([`docs/audits/`](audits/)) serve as historical evidence and qualification ledgers only; they carry zero normative authority to define or alter rules.

### 1.2 ID Stability & Registration Requirements
- **Immutable Deviation IDs**: Deviation IDs (`AML-DEVIATION-###`) are strictly immutable once registered. If a deviation is withdrawn, resolved, or reclassified as an API convention, its ID is retired with its historical rationale recorded and is never reused.
- Every registered deviation record must formally provide:
  - `Deviation ID` and `Title`
  - `Affected Rule` (exact section citation from Engineering Standard V1)
  - `Location` (source files and line ranges)
  - `Reason` (engineering and mathematical justification)
  - `Risk` (architectural or operational implications)
  - `Mitigation` (type safety guards, static assertions, or documentation)
  - `Verification` (unit/regression test evidence)
  - `Reviewer` (`Repository Maintainer`)
  - `Status` and `Removal / Migration Plan`

---

## 2. Deviation Index

| Deviation ID | Title | Module | Standard Rule | Status | Finding Cross-Reference |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **AML-DEVIATION-001** | Left-to-Right Frame Transformation Pipeline Composition | `Geometry` | Sec 22, 24 | **WITHDRAWN / RECLASSIFIED (NOT A DEVIATION)** | `AML-MED-005` (Remediated) |
| **AML-DEVIATION-002** | Public Mutable Coordinate Data Members | `Geometry` | Sec 9 | **REGISTERED / MAINTAINER-ACCEPTED FOR V1** | `AML-LOW-002` |
| **AML-DEVIATION-003** | PascalCase Root Namespace `AegisMath` | Global | Sec 8 | **RESOLVED / CLOSED BY VECTORIS MIGRATION** | `AML-LOW-001` |
| **AML-DEVIATION-004** | Pure MathLib Nominal Branch Coverage Threshold Calibration | Global | Sec 86 | **WITHDRAWN / REVOKED (RAW LLVM BRANCH GATE; NO DENOMINATOR ADJUSTMENT)** | `P3-SCOPE` / `P3.1` |
| **AML-DEVIATION-005** | Matrix3 Public Row-Major Scalar Storage | `Geometry` | Sec 9, 21 | **REGISTERED / VRT-15 LOCAL REVIEW; MAINTAINER REVIEW PENDING** | `VRT-15` |

> [!NOTE]
> - **Active Legitimate Deviations**: `AML-DEVIATION-002` covers public mutable spatial components; `AML-DEVIATION-005` is the separately scoped Matrix3 storage exception. AML-005 remains pending independent/maintainer review.
> - **AML-DEVIATION-001 Reclassification**: Detailed in Section 3.1 below. The left-to-right transformation pipeline composition satisfies all normative requirements of Sections 22–24 and is tracked as an API/mathematical convention rather than a standard deviation.
> - **AML-DEVIATION-003 Resolution**: Detailed in Section 3.3 below. Fully resolved and closed by the Vectoris Global Rename Migration (`namespace vectoris::numerics` and `namespace vectoris::dynamics`).
> - **AML-DEVIATION-004 Revocation**: Detailed in Section 3.4 below. No deviation or reachable-only accounting changes the raw LLVM branch denominator. The normative gate uses raw covered branches divided by raw total branches, with no exclusions. This internal engineering practice is not external certification or compliance.
> - **sqrt policy corrected by VRT-14**: `core::sqrt` now preserves signed zero and returns NaN for negative nonzero radicands, including -Inf; `core::Math::sqrt` forwards to it. The historical +0 clamping policy is superseded by [SSOT §15](ENGINEERING_STANDARD_V1.md) and [the canonical sqrt contract](core.md#10-canonical-coresqrt-contract-vrt-14). Section 16 governs comparisons and did not justify suppressing invalid radicands.

---

## 3. Formal Deviation Records

### AML-DEVIATION-001: Left-to-Right Frame Transformation Pipeline Composition

- **Deviation ID**: `AML-DEVIATION-001`
- **Title**: Left-to-Right Frame Transformation Pipeline Composition
- **Status**: **WITHDRAWN / RECLASSIFIED (NOT A DEVIATION)**
- **Reviewer**: Repository Maintainer
- **Recorded Date**: 2026-09-15
- **Applicable Version**: 1.0+
- **Module**: `Geometry`
- **Affected Rule Citation**: Section 22 / Section 24 (Coordinate frames & rotation matrices)
- **Location**: `include/AegisMath/Geometry/RotationMatrix3.h:92-98`, `include/AegisMath/Geometry/Quaternion.h:73-83`
- **Reclassification Rationale**:
  - The rotation composition operator `operator*` evaluates frame transformations from left to right as a sequential pipeline:
    $$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \implies \mathbf{R}_{A \to C}$$
  - Under standard linear algebra, transforming a vector from frame $A$ to frame $C$ evaluates:
    $$\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$$
  - The underlying Direction Cosine Matrix implementation multiplies `rhs.ToMatrix() * dcm_` ($\mathbf{M}_{BC} \mathbf{M}_{AB}$), and quaternions compute the reverse Hamilton product ($q_{rhs} \otimes q_{this}$).
  - **Normative Rule Analysis**: Engineering Standard Sections 22, 23, and 24 mandate explicit coordinate frames, active rotation conventions, unified multiplication conventions, and orthogonal matrix properties. They do **not** mandate surface syntax operator lexical ordering. The mathematical transformation $\mathbf{v}_C = \mathbf{R}_{BC} (\mathbf{R}_{AB} \mathbf{v}_A)$ is mathematically exact and frame-safe.
  - **Conclusion**: Current behavior violates zero normative rules of the Engineering Standard. This item is reclassified as a **Documented Mathematical and API Convention** in [`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md) and [`docs/geometry.md`](geometry.md). Finding `AML-MED-005` is closed as **REMEDIATED**.
- **Verification**:
  - `tests/Geometry/GeometryComparisonTest.cpp`: `RotationCompositionProperty` passes.
  - `tests/Geometry/AttitudeEngineTest.cpp`: `CascadingOrder_Q002_Fix` passes.

---

### AML-DEVIATION-002: Public Mutable Coordinate Data Members

- **Deviation ID**: `AML-DEVIATION-002`
- **Title**: Public Mutable Coordinate Data Members
- **Status**: **REGISTERED / MAINTAINER-ACCEPTED FOR V1**
- **Reviewer**: Repository Maintainer
- **Recorded Date**: 2026-09-15
- **Applicable Version**: 1.0+
- **Module**: `Geometry`
- **Affected Rule**: Section 9 (Member variables use trailing underscore `name_` and private encapsulation)
- **Location**: `modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/{Vector3,Point3,Quaternion}.h`; Dynamics value members are under `modules/VectorisDynamics/include/Vectoris/Dynamics/`
- **Description**:
  Spatial vectors, points, and quaternions expose their coordinate values as public data members (`x, y, z` and `w, x, y, z`) without private encapsulation or getter/setter methods.
- **Reason**:
  Preserves existing v1 API compatibility, zero-overhead aggregate initialization ergonomics, and direct element access in performance-critical inner numerical kernels. C++ layout traits alone do not guarantee cross-language ABI interoperability.
- **Risk**:
  Direct external mutation of quaternion components (`q.w = 0.5;`) bypasses construction-time unit normalization and sign canonicalization.
- **Mitigation**:
  Documented in `docs/geometry.md` that canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant. Rotational equivalence testing uses `RotationEquivalent()`, which robustly handles both $\mathbf{q}$ and $-\mathbf{q}$ regardless of component mutability.
- **Verification**:
  - `tests/Geometry/GeometryComparisonTest.cpp`: `QuaternionNearZeroWDeterminism` passes.
  - `modules/VectorisDynamics/tests/DynamicsABITest.cpp`: `StandardLayoutAndTriviality` passes.
- **Review Trigger**: Major version revision (v2.0).
- **Removal / Migration Plan**: Retained for v1.x; evaluate encapsulated types with accessors in v2.0.

---

### AML-DEVIATION-003: PascalCase Root Namespace `AegisMath`

- **Deviation ID**: `AML-DEVIATION-003`
- **Title**: PascalCase Root Namespace `AegisMath`
- **Status**: **RESOLVED / CLOSED BY VECTORIS MIGRATION**
- **Reviewer**: Repository Maintainer
- **Recorded Date**: 2026-09-15
- **Resolved Date**: 2026-09-17 (Vectoris Global Rename Migration)
- **Applicable Version**: None (Closed; Section 8 lowercase hierarchy is in full effect)
- **Module**: Global (`Core`, `Units`, `Geometry`)
- **Affected Rule Citation**: Section 8 (Namespaces shall be lowercase snake_case, root namespace `vectoris::numerics`)
- **Location**: Historically in `include/AegisMath/**`; resolved across `include/Vectoris/Numerics/**` and `include/Vectoris/Dynamics/**`
- **Resolution Rationale**:
  - Under the Vectoris Global Rename Migration, the repository completed a global architectural namespace realignment.
  - The pure mathematics and numerical core migrated from legacy `AegisMath` to the fully standard-conforming lowercase hierarchical namespace `namespace vectoris::numerics`, while the domain physics module migrated to `namespace vectoris::dynamics`.
  - All public headers now reside under `<Vectoris/Numerics/...>` and `<Vectoris/Dynamics/...>`.
  - The legacy `AegisMath` root was removed. Under VRT-12, lowercase `core`, `units`, and `geometry` are canonical public spellings; `Core`, `Units`, and `Geometry` remain nondeprecated compatibility namespaces for this release.
  - `AML-DEVIATION-003` is formally marked **RESOLVED / CLOSED**.
- **Verification**:
  Standalone-header and include-order checks plus fully qualified lowercase and PascalCase API probes are recorded in the incremental remediation ledger. Historical migration test counts are not current metrics.

---

### AML-DEVIATION-004: Pure MathLib Nominal Branch Coverage Threshold Calibration

- **Deviation ID**: `AML-DEVIATION-004`
- **Title**: Pure MathLib Nominal Branch Coverage Threshold Calibration
- **Status**: **WITHDRAWN / REVOKED; NO ACTIVE COVERAGE EXCEPTION**
- **Reviewer**: Repository Maintainer
- **Recorded / Revoked Date**: 2026-09-17
- **Applicable Version**: None
- **Module**: Global (`Core`, `Geometry`, `Units`)
- **Affected Rule Citation**: Engineering Standard §86 (`>=90%` branch coverage)
- **Location**: `tools/coverage/verify_coverage.py` and the raw LLVM coverage report
- **Revocation Rationale**:
  - A proposed reduction of the branch threshold was withdrawn. The normative thresholds remain 100% functions, at least 95% lines, and at least 90% branches.
  - The branch numerator and denominator are the raw LLVM covered-branch and total-branch counters. No unreachable, defensive, third-party, or otherwise unexecuted branch may be subtracted from the denominator to satisfy the gate.
  - Older P3/P3.1 coverage counts and classifications are historical snapshots and are not current qualification evidence. Current metrics belong in the dated remediation/qualification report with its source baseline and build configuration.
  - No active deviation lowers or adjusts any coverage threshold.
- **Verification**:
  The current coverage gate and its self-tests are part of the VRT-10 qualification tooling. See the current step-specific report for raw counts; this deviation record makes no current pass claim.

### AML-DEVIATION-005: Matrix3 Public Row-Major Scalar Storage

- **Deviation ID**: AML-DEVIATION-005
- **Title**: Matrix3 Public Row-Major Scalar Storage
- **Status**: **REGISTERED / VRT-15 LOCAL REVIEW; MAINTAINER AND INDEPENDENT REVIEW PENDING**
- **Reviewer**: Repository Maintainer (review pending)
- **Recorded Date**: 2026-09-23
- **Next Review Date**: 2027-09-23 and before any Stable qualification or major API release
- **Applicable Version**: Vectoris v1.x; reassess before v2.0
- **Module**: Geometry
- **Affected Rule Citation**: Engineering Standard §§9 and 21 (private member encapsulation; internal matrix storage is not public)
- **Location**: modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Matrix3.h, public member Matrix3<T>::m[9]
- **Description**:
  Matrix3<T> retains a public built-in array containing exactly nine scalar entries. Its specified order is row-major, with index row * 3 + col. Existing source access includes direct reads and writes, as well as contiguous iteration.
- **Technical Reason**:
  Matrix3 is a general unframed mathematical value whose intended state is any nine T entries. It has no finite-value, orthogonality, rotation, or positive-definite lifetime invariant. Public writes therefore cannot violate a stronger Matrix3 invariant that does not exist. The exact nine-element row-major representation is established API and is directly consumed by fixed-size kernels. Hiding it in v1 would break existing direct-field call sites and downstream source that relies on the public member, without increasing Matrix3 state validity. Frame-tagged rotations and checked numerical consumers remain separate abstractions.
- **Representation Contract**:
  The nine scalar array and row-major index mapping are source-level guarantees. Matrix3 provides mutable and const operator()(row, col) references; indexing is unchecked with precondition row < 3 and col < 3. The built-in array is contiguous. There is no data() member. Tested standard-layout/trivially-copyable traits and sizeof observations do not promise a C ABI, cross-build binary compatibility, DMA mapping, persistent format, or wire format. alignof observations are target-specific.
- **Risk**:
  Callers may store NaN/Inf or values that fail another operation's preconditions, and external code can depend on this layout. Matrix3 itself remains a general matrix. RotationMatrix3::TryCreate, the symmetric-positive-definite solver, and TryInverse must retain their own documented validation. A future Matrix3-specific invariant would invalidate the rationale and trigger reconsideration.
- **Mitigation**:
  The Matrix3 contract now names its representation, mutability, access precondition and limits. Tests verify public mutation, const/mutable accessor types, row-major mapping, contiguity, standard layout, trivial copyability, sizeof and alignment observations. Consumer validation paths remain independently tested. A Release microbenchmark baseline is recorded for multiplication, matrix-vector multiplication, determinant, TryInverse and element access; it establishes no universal timing or CI threshold.
- **Verification**:
  modules/VectorisNumerics/tests/Geometry/Matrix3RepresentationTest.cpp and PublicApiSurfaceTest.GeometryMatrix3RepresentationContract; benchmarks/Matrix3Benchmark.cpp checks independent deterministic oracles before timing. Results and build identity are recorded in the VRT-15 evidence archive.
- **Removal / Migration Plan**:
  Retain for v1.x. Before v2.0, audit downstream direct field users and consider a separately versioned encapsulated Matrix3 with operator()/data() APIs. This deviation does not authorize public storage on any other type.
