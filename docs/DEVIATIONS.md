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

Per Section 101 and Section 102 of Engineering Standard V1, any intentional divergence from normative rules must be formally registered in this ledger. Silent deviations, informal waivers, or undocumented architectural exceptions are strictly prohibited.

Every registered deviation must include:
- A unique identifier (`AML-DEVIATION-###`)
- Affected rule and location
- Technical rationale and risk evaluation
- Mitigations and verification test evidence
- Reviewer, status, and long-term removal / migration plans

---

## 2. Active Deviation Index

| Deviation ID | Title | Module | Standard Rule | Status | Finding Cross-Reference |
| :--- | :--- | :--- | :--- | :---: | :---: |
| **AML-DEVIATION-001** | Left-to-Right Frame Transformation Pipeline Composition | `Geometry` | Sec 22, 24 | **REGISTERED / ACCEPTED FOR V1** | `AML-MED-005` |
| **AML-DEVIATION-002** | Public Mutable Coordinate Data Members | `Geometry` | Sec 9 | **REGISTERED / ACCEPTED FOR V1** | `AML-LOW-002` |
| **AML-DEVIATION-003** | PascalCase Root Namespace `AegisMath` | Global | Sec 8 | **REGISTERED / ACCEPTED FOR V1** | `AML-LOW-001` |

> [!NOTE]
> `Core::Math::sqrt` non-negative domain clamping (returning `+0.0` for negative values to avoid filter NaN corruption) is a documented module-specific numerical domain policy within `docs/core.md`, rather than an Engineering Standard rule violation, and is therefore tracked as module policy rather than a standard deviation.

---

## 3. Formal Deviation Records

### AML-DEVIATION-001: Left-to-Right Frame Transformation Pipeline Composition

- **Deviation ID**: `AML-DEVIATION-001`
- **Title**: Left-to-Right Frame Transformation Pipeline Composition
- **Status**: **REGISTERED / ACCEPTED FOR V1**
- **Reviewer**: AegisMath Governance & Maintainers
- **Recorded Date**: 2026-09-15
- **Applicable Version**: 1.0+
- **Module**: `Geometry`
- **Affected Rule**: Section 22 / Section 24 (Mathematical composition of rotation operators $R_{\text{net}} = R_2 R_1$)
- **Location**: `include/AegisMath/Geometry/RotationMatrix3.h:92-98`, `include/AegisMath/Geometry/Quaternion.h:73-83`
- **Description**:
  The rotation composition operator `operator*` evaluates frame transformations from left to right as a sequential pipeline:
  $$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \implies \mathbf{R}_{A \to C}$$
  Under standard matrix multiplication, $\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$. Therefore, the underlying direction cosine matrix implementation multiplies `rhs.ToMatrix() * dcm_` ($\mathbf{M}_{BC} \mathbf{M}_{AB}$), and quaternions compute the reverse Hamilton product ($q_{rhs} \otimes q_{this}$).
- **Reason**:
  Left-to-right pipeline chaining (`R_world_body * R_body_sensor`) represents standard intuitive transformation modeling in aerospace, robotics, and graphics pipelines.
- **Risk**:
  Developers expecting raw matrix multiplication order ($M_1 M_2$) might assume `R1 * R2` multiplies matrices in lexical order.
- **Mitigation**:
  Compile-time frame tags (`FromFrame`, `ToFrame`) statically enforce frame chaining ($A \to B$ can only multiply $B \to C$; $B \to C$ multiplied by $A \to B$ is rejected at compile time).
- **Verification**:
  - `tests/Geometry/GeometryComparisonTest.cpp`: `RotationCompositionProperty` passes.
  - `tests/Geometry/AttitudeEngineTest.cpp`: `CascadingOrder_Q002_Fix` passes.
- **Review Trigger**: Major breaking release (v2.0).
- **Removal / Migration Plan**: Retained permanently in v1.x; maintain strict syntax across `RotationMatrix3`, `Quaternion`, and `Transform3`.

---

### AML-DEVIATION-002: Public Mutable Coordinate Data Members

- **Deviation ID**: `AML-DEVIATION-002`
- **Title**: Public Mutable Coordinate Data Members
- **Status**: **REGISTERED / ACCEPTED FOR V1**
- **Reviewer**: AegisMath Governance & Maintainers
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
- **Status**: **REGISTERED / ACCEPTED FOR V1**
- **Reviewer**: AegisMath Governance & Maintainers
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
