# AegisMathLib Formal Approved Deviation Ledger

> [!IMPORTANT]
> **Document**: Formal Approved Deviation Ledger  
> **Document Version**: 1.0  
> **Status**: Authoritative Governance Document  
> **Baseline Commit**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) Sections 101 & 102

---

## 1. Overview & Governance

Per Section 101 and Section 102 of Engineering Standard V1, any intentional divergence from normative rules must be formally registered in this ledger. Silent deviations, informal waivers, or undocumented architectural exceptions are strictly prohibited.

Every registered deviation must include:
- A unique identifier (`AML-DEVIATION-###`)
- Formal scope, standard rule reference, and technical description
- Technical rationale and risk evaluation
- Mitigations and verification test evidence
- Formal review triggers and long-term removal / migration plans

---

## 2. Active Deviation Index

| Deviation ID | Title | Module | Standard Rule | Status | Finding Cross-Reference |
| :--- | :--- | :--- | :--- | :---: | :---: |
| **AML-DEVIATION-001** | Left-to-Right Frame Transformation Pipeline Composition | `Geometry` | Sec 22, 24 | **APPROVED** | `AML-MED-005` |
| **AML-DEVIATION-002** | Public Mutable Coordinate Data Members for Standard-Layout ABI | `Geometry` | Sec 9, 29 | **APPROVED** | `AML-LOW-002` |
| **AML-DEVIATION-003** | Project-Specific Non-Negative Domain Clamping for `Core::Math::sqrt` | `Core` | Sec 14, 48 | **APPROVED** | `AML-MED-004` (Follow-up) |
| **AML-DEVIATION-004** | PascalCase Root Namespace `AegisMath` | Global | Sec 8 | **APPROVED** | `AML-LOW-001` |

---

## 3. Formal Deviation Records

### AML-DEVIATION-001: Left-to-Right Frame Transformation Pipeline Composition

- **Title**: Left-to-Right Frame Transformation Pipeline Composition
- **Status**: **APPROVED**
- **Applicable Version**: 1.0+
- **Module**: `Geometry`
- **Standard Rule**: Section 22 / Section 24 (Mathematical composition of rotation operators $R_{\text{net}} = R_2 R_1$)
- **Scope**: `RotationMatrix3::operator*`, `Quaternion::operator*`
- **Description**:
  The rotation composition operator `operator*` evaluates frame transformations from left to right as a sequential pipeline:
  $$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \implies \mathbf{R}_{A \to C}$$
  Under standard matrix multiplication, $\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$. Therefore, the underlying direction cosine matrix implementation multiplies `rhs.ToMatrix() * dcm_` ($\mathbf{M}_{BC} \mathbf{M}_{AB}$).
- **Rationale**:
  Left-to-right pipeline chaining (`R_world_body * R_body_sensor`) represents standard intuitive transformation modeling in aerospace, robotics, and graphics pipelines.
- **Risk**:
  Developers expecting raw matrix multiplication order ($M_1 M_2$) might assume `R1 * R2` multiplies matrices in lexical order.
- **Mitigation**:
  Compile-time frame tags (`FromFrame`, `ToFrame`) statically enforce frame chaining ($A \to B$ can only multiply $B \to C$; $B \to C$ multiplied by $A \to B$ is rejected at compile time).
- **Verification Evidence**:
  - `tests/Geometry/GeometryComparisonTest.cpp`: `RotationCompositionProperty` passes.
  - `tests/Geometry/AttitudeEngineTest.cpp`: `CascadingOrder_Q002_Fix` passes.
- **Review Trigger**: Major breaking release (v2.0).
- **Removal / Migration Plan**: Retained permanently in v1.x; maintain strict syntax across `RotationMatrix3`, `Quaternion`, and `Transform3`.

---

### AML-DEVIATION-002: Public Mutable Coordinate Data Members for Standard-Layout ABI

- **Title**: Public Mutable Coordinate Data Members for Standard-Layout ABI
- **Status**: **APPROVED**
- **Applicable Version**: 1.0+
- **Module**: `Geometry`
- **Standard Rule**: Section 9 / Section 29 (Private data member encapsulation; member naming conventions)
- **Scope**: `Vector3::x, y, z`, `Point3::x, y, z`, `Quaternion::w, x, y, z`
- **Description**:
  Spatial vectors, points, and quaternions expose their coordinate values as public, direct data members without private encapsulation or getter/setter methods.
- **Rationale**:
  Hard real-time embedded aerospace systems, hardware DMA channels, telemetry streaming, and SIMD registers require C-compatible standard-layout types (`std::is_standard_layout_v`, `std::is_trivially_copyable_v`) with zero-overhead direct component access.
- **Risk**:
  Direct modification of quaternion components (`q.w = 0.5;`) bypasses construction-time unit normalization and sign canonicalization.
- **Mitigation**:
  Documented in `docs/geometry.md` that canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant. Rotational equivalence testing uses `RotationEquivalent()`, which robustly handles both $\mathbf{q}$ and $-\mathbf{q}$ regardless of component mutability.
- **Verification Evidence**:
  - `tests/Dynamics/DynamicsABITest.cpp`: `StandardLayoutAndTriviality` passes.
  - `tests/Geometry/GeometryComparisonTest.cpp`: `QuaternionNearZeroWDeterminism` passes.
- **Review Trigger**: Major version revision (v2.0).
- **Removal / Migration Plan**: Retained for v1.x; evaluate encapsulated types with guaranteed standard-layout properties in v2.0.

---

### AML-DEVIATION-003: Project-Specific Non-Negative Domain Clamping for `Core::Math::sqrt`

- **Title**: Project-Specific Non-Negative Domain Clamping for `Core::Math::sqrt`
- **Status**: **APPROVED**
- **Applicable Version**: 1.0+
- **Module**: `Core`
- **Standard Rule**: Section 14 / Section 48 (IEEE-754 standard square root domain behavior returning NaN on negative finite inputs)
- **Scope**: `Core::Math::sqrt(T x)`
- **Description**:
  Negative finite floating-point values and $-\infty$ return $+0.0$ rather than generating `NaN` or triggering a numerical domain error.
- **Rationale**:
  Aerospace Kalman filters, state estimators, and spatial vector length calculations frequently produce tiny negative numbers (e.g. $-10^{-16}$) due to floating-point roundoff. Clamping to $+0.0$ prevents sudden catastrophic NaN propagation across long-term propagation loops.
- **Risk**:
  Could potentially mask large negative algorithmic errors if code mistakenly relies on `sqrt` failing on invalid physics inputs.
- **Mitigation**:
  - Strictly documented as the "AegisMath project-specific domain policy" in `docs/core.md`.
  - Constrained via `Concepts::SupportedSqrtScalar` to `float` and `double`.
  - Hard loop bound `kMaxIterations = 64` enforced at compile time.
  - Runtime delegates to standard-library `std::sqrt` for non-negative values.
- **Verification Evidence**:
  - `tests/Core/MathFunctionsTest.cpp`: `CoreSqrtTest.SignedZeroAndNegativeDomainPolicy` and `CoreSqrtTest.NegativeInfinityClampedToZero` pass.
- **Review Trigger**: Evaluation of error-returning `CheckedSqrt` in v2.0.
- **Removal / Migration Plan**: Retain non-negative clamping for `Core::Math::sqrt` in v1.x; introduce `TrySqrt` returning `Result<T, MathError>` in v2.0.

---

### AML-DEVIATION-004: PascalCase Root Namespace `AegisMath`

- **Title**: PascalCase Root Namespace `AegisMath`
- **Status**: **APPROVED**
- **Applicable Version**: 1.0+
- **Module**: Global (`Core`, `Units`, `Geometry`, `Dynamics`)
- **Standard Rule**: Section 8 (Namespaces must be lowercase snake_case `aegis::math`)
- **Scope**: Root namespace `AegisMath`
- **Description**:
  The repository organizes all production code under `namespace AegisMath` rather than `namespace aegis::math`.
- **Rationale**:
  Pre-existing architectural baseline across all public headers, test suites, and downstream integrations.
- **Risk**:
  Non-conformance with Engineering Standard Section 8 naming convention.
- **Mitigation**:
  Internal namespaces follow strict hierarchy (`AegisMath::Core`, `AegisMath::Units`, `AegisMath::Geometry`, `AegisMath::Dynamics`).
- **Verification Evidence**:
  All header isolation compilation tests and unit test suites compile cleanly with 0 warnings.
- **Review Trigger**: Major version release (v2.0).
- **Removal / Migration Plan**: Add `namespace aegis::math = AegisMath;` as non-breaking alias in v1.1, migrate canonical namespace in v2.0.
