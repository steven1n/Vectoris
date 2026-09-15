# AegisMathLib Geometry Module Specification

> [!IMPORTANT]
> **Document**: Geometry Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Geometry` module implements 3D spatial geometry, linear algebra, coordinate transformations, attitude representations, and frame safety. It enforces strict coordinate-frame typing at compile time, eliminating frame confusion errors in aerospace navigation and simulation algorithms.

---

## 2. Public Headers

The `Geometry` module exposes **15 public headers** under `include/AegisMath/Geometry/`:

| Header | Description |
| :--- | :--- |
| [`AlmostEqual.h`](../include/AegisMath/Geometry/AlmostEqual.h) | Tolerance-aware comparison helpers for spatial and attitude types. |
| [`Concepts.h`](../include/AegisMath/Geometry/Concepts.h) | Frame tag concepts (`IsFrameTag`, `SameFrame`, `ValidRotationFrame`). |
| [`CoordinateConvention.h`](../include/AegisMath/Geometry/CoordinateConvention.h) | Right-handed axis conventions, NED/ENU frame documentation. |
| [`Detail/ABI.h`](../include/AegisMath/Geometry/Detail/ABI.h) | Standard layout, alignment, and size validation helpers for geometry types. |
| [`Detail/RotationInvariant.h`](../include/AegisMath/Geometry/Detail/RotationInvariant.h) | Orthogonality verification helper using squared Frobenius norm. |
| [`FrameTags.h`](../include/AegisMath/Geometry/FrameTags.h) | Semantic coordinate frame tags (`WorldFrame`, `BodyFrame`, `ECEFFrame`, etc.). |
| [`Matrix3.h`](../include/AegisMath/Geometry/Matrix3.h) | Generic, frame-agnostic numerical $3 \times 3$ matrix container in row-major storage. |
| [`Point3.h`](../include/AegisMath/Geometry/Point3.h) | Frame-tagged affine point in $\mathbb{R}^3$. |
| [`Quaternion.h`](../include/AegisMath/Geometry/Quaternion.h) | Frame-tagged Hamilton unit quaternion for $SO(3)$ rotations. |
| [`RotationMatrix3.h`](../include/AegisMath/Geometry/RotationMatrix3.h) | Frame-tagged direction cosine matrix (DCM) in $SO(3)$. |
| [`SymmetricLinearSolver3.h`](../include/AegisMath/Geometry/SymmetricLinearSolver3.h) | Fixed $3 \times 3$ analytic $LDL^T$ SPD linear solver with conditioning bounds. |
| [`Traits.h`](../include/AegisMath/Geometry/Traits.h) | Geometry precision traits and tolerance defaults. |
| [`Transform3.h`](../include/AegisMath/Geometry/Transform3.h) | Frame-tagged $SE(3)$ homogeneous rigid-body transformation (rotation + translation). |
| [`UnitVector3.h`](../include/AegisMath/Geometry/UnitVector3.h) | Normalized unit direction vector in $S^2$. |
| [`Vector3.h`](../include/AegisMath/Geometry/Vector3.h) | Frame-tagged Euclidean vector in $\mathbb{R}^3$. |

---

## 3. `Matrix3<T>` Contract

`Matrix3<T>` is a **generic, frame-agnostic numerical $3 \times 3$ matrix**.
- **Role**: Pure numerical linear algebra container. It represents linear solver coefficient matrices, Jacobians, covariance blocks, or extracted inertia coefficients.
- **Frame Policy**: `Matrix3<T>` intentionally does NOT carry coordinate frame tags (`FromFrame`, `ToFrame`, or `FrameType`). It is deliberately unframed so that pure linear algebra algorithms (e.g. $LDL^T$ decomposition, determinants, matrix inversions) remain reusable across all domains without artificial frame coupling.
- **No Transformation Semantics**: `Matrix3<T>` does NOT imply coordinate-frame transformation semantics. Spatial transformations between frames are strictly mediated by dedicated semantic types (`RotationMatrix3`, `Transform3`, `Quaternion`).

---

## 4. Frame-Safe Geometry

AegisMathLib prevents accidental cross-frame operations at compile time:
- `Vector3<T, Frame>`: A vector rooted in a specific reference frame.
- `Point3<T, Frame>`: An absolute spatial location in a specific reference frame.
- `Quaternion<T, FromFrame, ToFrame>`: An active rotation transforming coordinates from `FromFrame` to `ToFrame`.
- `RotationMatrix3<T, FromFrame, ToFrame>`: A direction cosine matrix from `FromFrame` to `ToFrame`.
- `Transform3<T, FromFrame, ToFrame>`: A rigid body transformation from `FromFrame` to `ToFrame`.

### Compile-Time Frame Checks
- Adding two vectors with different frame tags is statically rejected: `v_world + v_body` produces a compile error.
- Adding two points (`p1 + p2`) is rejected; adding a vector to a point (`p + v`) produces a `Point3` in the same frame.
- Rotating a vector requires matching frame tags: `R_world_to_body * v_world` yields `v_body`; passing `v_body` produces a compile error.

---

## 5. Equality & Comparison Model

AegisMathLib explicitly separates three distinct tiers of floating-point comparison:

### 5.1 Exact Component-Wise Value Equality (`operator==`, `operator!=`)
- Direct component comparison using standard C++ floating-point `==` (`x == rhs.x && y == rhs.y && z == rhs.z`).
- **Exact Value Equality, Not Bitwise**: $+0.0 == -0.0$ evaluates to `true`; $\text{NaN} == \text{NaN}$ evaluates to `false`.
- **Usage Policy**: Reserved exclusively for exact identity initialization checks, sentinel states, serialization round-trips, and transactional rollbacks. **FORBIDDEN** for computed convergence or algorithm termination.

### 5.2 Tolerance-Aware Closeness (`AlmostEqual`)
- Available across all geometry types: `AlmostEqual(a, b, abs_tol, rel_tol)`.
- Combines absolute and relative tolerances:
  $$|a_i - b_i| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a_i|, |b_i|))$$

### 5.3 Rotational Equivalence (`RotationEquivalent`)
- Quaternions represent $SO(3)$ via a double cover ($\mathbb{S}^3 \to SO(3)$), where $\mathbf{q}$ and $-\mathbf{q}$ represent the identical physical orientation.
- `RotationEquivalent(q1, q2, abs_tol, rel_tol)` checks:
  $$\operatorname{AlmostEqual}(\mathbf{q}_1, \mathbf{q}_2) \lor \operatorname{AlmostEqual}(\mathbf{q}_1, -\mathbf{q}_2)$$
- Robust across all rotations, including $180^\circ$ turns where $w = 0$.

---

## 6. `Quaternion` Contract

- **Convention**: Hamilton quaternion $q = w + x\mathbf{i} + y\mathbf{j} + z\mathbf{k}$ with $\mathbf{i}^2 = \mathbf{j}^2 = \mathbf{k}^2 = \mathbf{i}\mathbf{j}\mathbf{k} = -1$.
- **Storage Order**: Standard `[w, x, y, z]` layout.
- **Active Rotation**: Transforms vectors from `FromFrame` to `ToFrame`.
- **Construction-Time Normalization**: `Quaternion::TryCreate(w, x, y, z)` normalizes the four components to $\|\mathbf{q}\| \approx 1$.
- **Construction-Time Canonicalization**: If $w < 0$, all components are negated to force $w \ge 0$. If $w == 0$, the sign of the vector part is preserved as supplied.
- **Mutability & Lifetime Invariant**:
  - Components `w, x, y, z` remain public mutable members to satisfy standard-layout and trivially copyable ABI constraints for telemetry and DMA buffers (see **AML-DEVIATION-002**).
  - Therefore, **canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant**.
- **180-Degree Degeneracy**: For $180^\circ$ rotations ($w = 0$), $[0, 1, 0, 0]$ and $[0, -1, 0, 0]$ represent the identical physical rotation. Canonicalization does not enforce lexicographical ordering on the vector part. Applications requiring equivalence testing MUST use `RotationEquivalent()`.

---

## 7. Rotation Composition Convention (Pipeline Syntax)

`RotationMatrix3` and `Quaternion` adopt a **left-to-right frame transformation pipeline syntax**:

$$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \to \mathbf{R}_{A \to C}$$

### Mathematical Semantics
When applied to a vector $\mathbf{v}_A$, the chained operator pipeline evaluates left-to-right:
$$(\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C}) * \mathbf{v}_A \approx \mathbf{R}_{B \to C} * (\mathbf{R}_{A \to B} * \mathbf{v}_A) = \mathbf{v}_C$$

In standard matrix linear algebra, the transformation is:
$$\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$$
Therefore, the underlying Direction Cosine Matrix multiplication is:
$$\mathbf{M}_{AC} = \mathbf{M}_{BC} \mathbf{M}_{AB} \quad (\text{i.e. } \texttt{rhs.ToMatrix() * dcm\_})$$

> [!NOTE]
> This pipeline composition convention is formally documented as a project convention in [`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md). It satisfies all normative requirements of Sections 22–24 and is tracked as an architectural convention (historically registered as AML-DEVIATION-001 before confirmation of full standard compliance).

---

## 8. `SymmetricLinearSolver3<T>`

`SymmetricLinearSolver3<T>` provides an analytic $LDL^T$ decomposition for $3 \times 3$ symmetric positive definite (SPD) linear systems $A x = b$:
- **Policy**: Strictly follows the **Solve-Not-Invert** rule (Section 17 of Engineering Standard V1). Never computes $A^{-1} b$.
- **Conditioning & Robustness Checks**:
  1. Validates all entries are finite (`MathError::non_finite_input`).
  2. Validates scale-aware symmetry ($|A_{ij} - A_{ji}| \le \text{scale} \cdot \epsilon \cdot 100$, returning `MathError::invalid_argument` on asymmetry).
  3. Validates positive pivot thresholds during $LDL^T$ factorization ($d_k \le \text{tol}_{\text{sing}}$ returns `MathError::singular_matrix`; $d_k < -\text{tol}_{\text{sing}}$ returns `MathError::invalid_state` indicating an indefinite or negative-definite matrix).
  4. LDLT pivot-spread safeguard ($\min(d) / \max(d) \le 100 \cdot \epsilon$ triggers `MathError::ill_conditioned`).
  5. Infinity-norm relative backward error verification:
     $$\eta = \frac{\|A x - b\|_\infty}{\|A\|_\infty \|x\|_\infty + \|b\|_\infty}$$
     rejects solutions where $\eta > 100 \cdot \epsilon$ with `MathError::ill_conditioned`.

---

## 9. Verification Evidence

- [`tests/Geometry/Matrix3Test.cpp`](../tests/Geometry/Matrix3Test.cpp): Adjoint cofactor inversion indices, aliasing prevention, pure numerical algebra.
- [`tests/Geometry/SymmetricLinearSolver3Test.cpp`](../tests/Geometry/SymmetricLinearSolver3Test.cpp): Analytic $LDL^T$ solve, backward error tracking, ill-conditioned rejection.
- [`tests/Geometry/AttitudeEngineTest.cpp`](../tests/Geometry/AttitudeEngineTest.cpp): Rotation composition, quaternion-to-matrix agreement.
- [`tests/Geometry/GeometryComparisonTest.cpp`](../tests/Geometry/GeometryComparisonTest.cpp): 13 dedicated tests for `operator==`, `AlmostEqual`, `RotationEquivalent`, $180^\circ$ edge cases, near-zero $w$, compile-time rejection guards.

---

## 10. Registered Deviations

1. **AML-DEVIATION-002**: Public mutable data members (`x, y, z` and `w, x, y, z`) for existing API compatibility and aggregate direct access (registered in [`docs/DEVIATIONS.md`](DEVIATIONS.md)).
2. **AML-DEVIATION-003**: Namespace `AegisMath::Geometry` instead of `aegis::math::geometry` (registered in [`docs/DEVIATIONS.md`](DEVIATIONS.md)).
