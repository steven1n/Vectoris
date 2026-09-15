# AegisMathLib Formal Approved Deviation Ledger

> [!IMPORTANT]
> **Document**: Formal Approved Deviation Ledger  
> **Document Version**: 1.0  
> **Status**: Authoritative Governance Document  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) Sections 101 & 102

---

## 1. Overview & Governance

### 1.1 Documentation Authority Model & Scoped Precedence
- **Default Normative Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) is the single source of truth (SSOT) for all normative engineering, mathematical, and architectural requirements.
- **Authorized Scoped Exception Mechanism**: This document (`docs/DEVIATIONS.md`) is the Engineering Standard's formally authorized scoped exception mechanism governed by Sections 101 & 102.
- **Precedence**:
  - Within an explicitly registered deviation scope, the applicable registered deviation modifies or overrides **only** the cited normative rule for the stated component, scope, and version lifetime.
  - Outside that explicitly registered scope, the Engineering Standard remains fully and strictly authoritative.
  - Project conventions ([`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md)), module specifications ([`docs/core.md`](core.md), [`docs/units.md`](units.md), [`docs/geometry.md`](geometry.md), [`docs/dynamics.md`](dynamics.md)), and repository overviews ([`README.md`](../README.md)) must strictly conform to the **Engineering Standard plus applicable registered deviations**.
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
| :--- | :--- | :--- | :--- | :---: | :---: |
| **AML-DEVIATION-001** | Left-to-Right Frame Transformation Pipeline Composition | `Geometry` | Sec 22, 24 | **WITHDRAWN / RECLASSIFIED (NOT A DEVIATION)** | `AML-MED-005` (Remediated) |
| **AML-DEVIATION-002** | Public Mutable Coordinate Data Members | `Geometry` | Sec 9 | **REGISTERED / MAINTAINER-ACCEPTED FOR V1** | `AML-LOW-002` |
| **AML-DEVIATION-003** | PascalCase Root Namespace `AegisMath` | Global | Sec 8 | **REGISTERED / MAINTAINER-ACCEPTED FOR V1** | `AML-LOW-001` |

> [!NOTE]
> - **Active Legitimate Deviations**: Exactly 2 active deviations are accepted for v1 (`AML-DEVIATION-002` and `AML-DEVIATION-003`).
> - **AML-DEVIATION-001 Reclassification**: Detailed in Section 3.1 below. The left-to-right transformation pipeline composition satisfies all normative requirements of Sections 22–24 and is tracked as an API/mathematical convention rather than a standard deviation.
> - **`Core::Math::sqrt` Domain Clamping**: Non-negative domain clamping (returning `+0.0` for negative values to prevent Kalman filter NaN corruption) is a documented module numerical policy within [`docs/core.md`](core.md), fully permitted under Section 16, and is tracked as module policy rather than a standard deviation.

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
- **Location**: `include/AegisMath/Geometry/Vector3.h:17-19`, `include/AegisMath/Geometry/Point3.h:16-18`, `include/AegisMath/Geometry/Quaternion.h:21-24`
- **Description**:
  Spatial vectors, points, and quaternions expose their coordinate values as public data members (`x, y, z` and `w, x, y, z`) without private encapsulation or getter/setter methods.
- **Reason**:
  Preserves existing v1 API compatibility, zero-overhead aggregate initialization ergonomics, direct element access in performance-critical inner numerical kernels, and legacy C-compatible layout interoperability.
- **Risk**:
  Direct external mutation of quaternion components (`q.w = 0.5;`) bypasses construction-time unit normalization and sign canonicalization.
- **Mitigation**:
  Documented in `docs/geometry.md` that canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant. Rotational equivalence testing uses `RotationEquivalent()`, which robustly handles both $\mathbf{q}$ and $-\mathbf{q}$ regardless of component mutability.
- **Verification**:
  - `tests/Dynamics/DynamicsABITest.cpp`: `StandardLayoutAndTriviality` passes.
  - `tests/Geometry/GeometryComparisonTest.cpp`: `QuaternionNearZeroWDeterminism` passes.
- **Review Trigger**: Major version revision (v2.0).
- **Removal / Migration Plan**: Retained for v1.x; evaluate encapsulated types with accessors in v2.0.

---

### AML-DEVIATION-003: PascalCase Root Namespace `AegisMath`

- **Deviation ID**: `AML-DEVIATION-003`
- **Title**: PascalCase Root Namespace `AegisMath`
- **Status**: **REGISTERED / MAINTAINER-ACCEPTED FOR V1**
- **Reviewer**: Repository Maintainer
- **Recorded Date**: 2026-09-15
- **Applicable Version**: 1.0+
- **Module**: Global (`Core`, `Units`, `Geometry`, `Dynamics`)
- **Affected Rule**: Section 8 (Namespaces must be lowercase snake_case `aegis::math`)
- **Location**: Global codebase (`include/AegisMath/**`)
- **Description**:
  The repository organizes all production code under `namespace AegisMath` rather than `namespace aegis::math`.
- **Reason**:
  Pre-existing architectural baseline across all public headers, test suites, and downstream integrations.
- **Risk**:
  Non-conformance with Engineering Standard Section 8 naming convention.
- **Mitigation**:
  Internal namespaces follow strict hierarchy (`AegisMath::Core`, `AegisMath::Units`, `AegisMath::Geometry`, `AegisMath::Dynamics`).
- **Verification**:
  All header isolation compilation tests and unit test suites compile cleanly with 0 warnings.
- **Review Trigger**: Major version release (v2.0).
- **Removal / Migration Plan**: Introduce standard namespace alias in non-breaking update:
  ```cpp
  namespace aegis {
      namespace math = ::AegisMath;
  }
  ```
  and complete full migration in v2.0.
