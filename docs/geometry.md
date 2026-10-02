# Vectoris Geometry Module Specification

> [!IMPORTANT]
> **Document**: Geometry Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Reviewed Starting HEAD**: `cfbecc6390fb30d857f10f2516f39bb2ef75f996` (working tree contains uncommitted remediations)
> **Last Updated**: 2026-09-23
> **Current Qualification**: NOT REQUALIFIED / Experimental
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Geometry` module implements 3D spatial geometry, linear algebra, coordinate transformations, attitude representations, and frame safety. It enforces strict coordinate-frame typing at compile time, eliminating frame confusion errors in aerospace navigation and simulation algorithms.

### Public namespace contract (VRT-12)

`vectoris::numerics::geometry` is the canonical public spelling.
`vectoris::numerics::Geometry` remains a compatibility spelling for this release,
**not deprecated**. Older `Geometry::` examples in this document use that spelling;
both denote identical entities. Each public header directly includes the layer's
`Geometry/Namespace.h`, making the alias usable from that header alone and
independent of include order. No umbrella header is required. Definitions stay
in their historical namespace; template identity, ADL and numerical behavior do
not change. Existing nested `Detail` names remain implementation-only under
both spellings, with no supported API or ABI promise. No aliases are placed
under `vectoris::dynamics`. Overall status remains Experimental.

---

## 2. Public Headers

The `Geometry` module exposes **16 public headers** under `include/Vectoris/Numerics/Geometry/`:

| Header | Description |
| :--- | :--- |
| [`Namespace.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Namespace.h) | Authoritative `geometry` alias and compatibility namespace declaration. |
| [`AlmostEqual.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/AlmostEqual.h) | Tolerance-aware comparison helpers for spatial and attitude types. |
| [`Concepts.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Concepts.h) | Frame tag concepts (`IsFrameTag`, `SameFrame`, `ValidRotationFrame`). |
| [`CoordinateConvention.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/CoordinateConvention.h) | Right-handed axis conventions, NED/ENU frame documentation. |
| [`Detail/ABI.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Detail/ABI.h) | C++ standard-layout and trivial-copy trait checks; not a cross-build ABI guarantee. |
| [`Detail/RotationInvariant.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Detail/RotationInvariant.h) | Orthogonality verification helper using squared Frobenius norm. |
| [`FrameTags.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/FrameTags.h) | Semantic coordinate frame tags (`WorldFrame`, `BodyFrame`, `ECEFFrame`, etc.). |
| [`Matrix3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Matrix3.h) | Generic, frame-agnostic numerical $3 \times 3$ matrix container in row-major storage. |
| [`Point3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Point3.h) | Frame-tagged affine point in $\mathbb{R}^3$. |
| [`Quaternion.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Quaternion.h) | Frame-tagged Hamilton unit quaternion for $SO(3)$ rotations. |
| [`RotationMatrix3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/RotationMatrix3.h) | Frame-tagged direction cosine matrix (DCM) in $SO(3)$. |
| [`SymmetricLinearSolver3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/SymmetricLinearSolver3.h) | Fixed $3 \times 3$ analytic $LDL^T$ SPD linear solver with conditioning bounds. |
| [`Traits.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Traits.h) | Geometry precision traits and tolerance defaults. |
| [`Transform3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Transform3.h) | Frame-tagged $SE(3)$ homogeneous rigid-body transformation (rotation + translation). |
| [`UnitVector3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/UnitVector3.h) | Normalized unit direction vector in $S^2$. |
| [`Vector3.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Geometry/Vector3.h) | Frame-tagged Euclidean vector in $\mathbb{R}^3$. |

---

## 3. `Matrix3<T>` Contract

`Matrix3<T>` is a **generic, frame-agnostic numerical $3 \times 3$ matrix**.
- **Role**: Pure numerical linear algebra container. It represents linear solver coefficient matrices, Jacobians, covariance blocks, or extracted inertia coefficients.
- **Frame Policy**: `Matrix3<T>` intentionally does NOT carry coordinate frame tags (`FromFrame`, `ToFrame`, or `FrameType`). It is deliberately unframed so that pure linear algebra algorithms (e.g. $LDL^T$ decomposition, determinants, matrix inversions) remain reusable across all domains without artificial frame coupling.
- **No Transformation Semantics**: `Matrix3<T>` does NOT imply coordinate-frame transformation semantics. Spatial transformations between frames are strictly mediated by dedicated semantic types (`RotationMatrix3`, `Transform3`, `Quaternion`).

### Storage and Element Access (VRT-15)

- `Matrix3<T>` exposes `T m[9]` under the narrowly scoped AML-DEVIATION-005. It is a general matrix value with nine scalar entries and no lifetime invariant requiring finite values, orthogonality, rotation semantics, or positive definiteness; callers may assign any `T` value, including NaN/Inf for floating-point `T`.
- Both `matrix(row, col)` overloads are unchecked. The precondition is `row < 3 && col < 3`; invalid indices have no checked-error behavior. Mutable access returns `T&`; const access returns `const T&`.
- Storage order is row-major: `m[row * 3 + col]`. The public built-in array is contiguous by the C++ array rules. There is no separate `data()` member.
- For float and double, tests enforce standard-layout and trivially-copyable traits plus `sizeof(Matrix3<T>) == 9 * sizeof(T)`. Those are source-level type/size properties. `sizeof`/`alignof` numbers observed on one target do not promise a stable cross-compiler ABI, C ABI, DMA layout, persistent representation, or wire format.
- This exception applies only to `Matrix3<T>`; it does not weaken storage encapsulation for other matrix/geometry types. The raw representation remains subject to the deviation review and removal plan.

### Matrix3 performance evidence (VRT-15)

The optional `VECTORIS_BUILD_BENCHMARKS` target measures double-precision
Matrix3 × Matrix3, Matrix3 × Vector3, determinant, `TryInverse`, and element
access. It checks fixed analytical results and an inverse residual before timing,
uses a deterministic corpus, a consumed checksum, 10,000 warm-up operations,
and nine samples of 200,000 operations with `steady_clock`; the reported value is
the sample median. A local AppleClang Release baseline is archived with VRT-15.
These timings describe only that host/build and are informational evidence, not
a correctness result or CI regression threshold. Repeat and qualify baselines on
a controlled runner before establishing a performance gate or declaring this
performance-sensitive kernel Stable.

---

## 4. Frame-Safe Geometry

Vectoris prevents accidental cross-frame operations at compile time:
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

Vectoris explicitly separates three distinct tiers of floating-point comparison:

### 5.1 Exact Component-Wise Value Equality (`operator==`, `operator!=`)
- Direct component comparison using standard C++ floating-point `==` (`x == rhs.x && y == rhs.y && z == rhs.z`).
- **Exact Value Equality, Not Bitwise**: $+0.0 == -0.0$ evaluates to `true`; $\text{NaN} == \text{NaN}$ evaluates to `false`.
- **Usage Policy**: Reserved exclusively for exact identity initialization checks, sentinel states, serialization round-trips, and transactional rollbacks. **FORBIDDEN** for computed convergence or algorithm termination.

### 5.2 Tolerance-Aware Closeness (`AlmostEqual`)
- Available across all geometry types: `AlmostEqual(a, b, abs_tol, rel_tol)`.
- Combines absolute and relative tolerances:
  $$|a_i - b_i| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a_i|, |b_i|))$$
- Vector3, Point3, Matrix3, Quaternion, UnitVector3, RotationMatrix3 and
  Transform3 delegate component comparisons to the overflow-safe scalar
  [AlmostEqual contract](core.md#81-almostequal-finite-and-special-value-contract-vrt-09).
  Both tolerances must be finite and nonnegative; invalid tolerances return
  `false`. Frame constraints are unchanged. Approximate closeness is symmetric
  but not transitive. RotationEquivalent uses the same scalar contract when
  checking both quaternion signs; these comparisons do not establish validity
  of arbitrary mutable quaternion storage.

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
- **Identity Factory**: `Quaternion<T, F, F>::Identity()` is available only when the class's actual source and destination frames match. Legacy explicit function-template arguments remain valid when they name a same-frame pair; they do not select or change the class's frame mapping. Cross-frame quaternion types cannot call `Identity`, regardless of those arguments.
- **Construction-Time Normalization**: `Quaternion::TryCreate(w, x, y, z)` normalizes the four dimensionless components to $\|\mathbf{q}\| \approx 1$. For finite nonzero `float`/`double` input, divide by the largest absolute component before computing the norm; the scaled squared norm lies in `[1, 4]`. This avoids overflow and rejects only exact all-zero input (`zero_norm`), rather than applying an absolute small-norm cutoff. NaN/Inf returns `non_finite_input`. Complexity and storage are O(1), with no allocation or iteration. Extremely small components relative to the largest may round to zero; gradual underflow and the normal IEEE-754 environment are assumed.
- **Checked Matrix Conversion**: `ToRotationMatrix()` and `RotationMatrix3::FromQuaternion(q)` return `Result<RotationMatrix3<T, FromFrame, ToFrame>, MathError>`. Non-finite components return `non_finite_input`; finite components outside the unit-quaternion domain or a matrix failing the existing determinant/orthogonality checks return `invalid_state`. Neither function silently renormalizes or unwraps a failed result. A successful conversion preserves the declared source and target frames.
- **Composition Drift**: Repeated Hamilton products may drift from unit norm. Callers can explicitly renormalize using `TryCreate(q.w, q.x, q.y, q.z)` and must check its result before conversion. Other quaternion operations still require valid unit-quaternion inputs; this change does not establish a lifetime invariant for public mutable storage.
- **Construction-Time Canonicalization**: If $w < 0$, all components are negated to force $w \ge 0$. If $w == 0$, the sign of the vector part is preserved as supplied.
- **Mutability & Lifetime Invariant**:
  - Components `w, x, y, z` remain public mutable members under **AML-DEVIATION-002** and are checked for C++ standard-layout/trivial-copy properties. These properties alone do not establish a C ABI, wire format, DMA mapping, or persistent-storage format.
  - Therefore, **canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant**.
- **180-Degree Degeneracy**: For $180^\circ$ rotations ($w = 0$), $[0, 1, 0, 0]$ and $[0, -1, 0, 0]$ represent the identical physical rotation. Canonicalization does not enforce lexicographical ordering on the vector part. Applications requiring equivalence testing MUST use `RotationEquivalent()`.

`Transform3<T, F, F>::Identity()` follows the same actual-class-frame restriction.
`RotationMatrix3<T, From, To>::Identity()` retains its distinct matrix-valued
meaning: it returns the numerical identity matrix even for different frame tags.
That cross-frame value represents a valid rotation only when the caller knows the
two coordinate bases are aligned; frame tags alone do not establish that fact.

---

### Checked-conversion migration (VRT-01)

This is a source-breaking return-type change. Replace direct use of the returned matrix with an explicit success check:

```cpp
auto rotation = q.ToRotationMatrix();
if (!rotation.IsSuccess()) {
    return rotation; // propagate the error from a Result-returning caller
}
const auto& matrix = rotation.Value();
// Use matrix only while rotation remains alive.
```

The same handling is required for `RotationMatrix3::FromQuaternion(q)`.
Regression coverage in `AttitudeEngineTest.cpp` uses both floating-point precisions,
minimum subnormal/normal and maximum finite inputs, analytic rotations, invalid
components, and 100,000 compositions. Existing frame and matrix-agreement tests
continue to check the conversion's geometry.

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

`SolveSymmetricPositiveDefinite3x3(A,b)` solves a fixed 3x3 SPD system using
sqrt-free LDLT, following **Solve-Not-Invert** (Engineering Standard §18).
It retains its `constexpr`, `noexcept`, Frame-preserving, return-by-value API;
there is no caller-supplied output parameter. Inputs are const and failure returns
`Result::failure(MathError)` without a partial solution. No inverse, heap
allocation, external factorization library or unbounded iteration is used.

### Domain and scaling (VRT-08)

The local regression-test assumptions here are binary IEEE-754 float/double with gradual
underflow and normal floating-point rounding. Finite input is required. Define

$$s_A=\max_{ij}|A_{ij}|,\qquad s_b=\max_i|b_i|,\qquad
\bar A=A/s_A,\qquad\bar b=b/s_b.$$

Reject `sA == 0` as `singular_matrix`. Factor and validate the matrix even for a
zero RHS, then return zero when `sb == 0`. Otherwise solve

$$\bar A y=\bar b,\qquad x=(s_b/s_A)y.$$

This is equivalent to `Ax=b` in exact arithmetic: multiply the normalized
equation by `sb` and substitute `y=(sA/sb)x`. Each normalized input component
has magnitude at most one. Do not form an unscaled squared norm, reciprocal
of a tiny scale, or the possibly overflowing/underflowing ratio `sb/sA`.
For each restored component, split value and scales into binary fractions and
integer exponents, compute the bounded fraction product/quotient, then apply the
combined exponent with `scalbn`. Only an unrepresentable final result overflows.
C++20 constant evaluation uses bounded arithmetic equivalents of `frexp/scalbn`;
they are independently compared with the standard library at runtime in tests.

### Symmetry, pivots and conditioning

Require `abs(Abar_ij - Abar_ji) <= 100*epsilon`. This is the original relative
symmetry tolerance expressed in normalized coordinates; boundary decisions may
differ by rounding, and tiny-scale tolerance no longer collapses to zero.
As before, factor the **lower-triangle mirror**, while checking the residual
against **all entries of the original normalized matrix**, including its upper
triangle. This does not silently symmetrize the residual equation.

The LDLT decomposition equations are unchanged. On normalized A, pivots below
`-10*epsilon` return `invalid_state`; pivots at or below `10*epsilon` return
`singular_matrix`. These are the original scale-relative thresholds divided by
`sA`. Non-finite pivots/factorization results return `ill_conditioned`.
Retain the safeguard `min(pivots)/max(pivots) > 100*epsilon`.
**Pivot spread is not cond(A)**; it is a rejection heuristic, not a condition
number estimate or proof of a small forward error.

### Backward-error acceptance

Validate the **rounded returned x**, including any subnormal rounding, by safely
reconstructing `z = x*sA/sb` with the same exponent method. Let

$$q=\max(1,\|z\|_\infty),\qquad w=z/q,\qquad c=\bar b/q.$$

Evaluate the infinity-norm criterion using bounded quantities:

$$\eta=\frac{\|\bar A w-c\|_\infty}
{\|\bar A\|_\infty\|w\|_\infty+\|c\|_\infty}
=\frac{\|Ax-b\|_\infty}{\|A\|_\infty\|x\|_\infty+\|b\|_\infty}.$$

The equality is in exact arithmetic. In production, scaled matrix entries and
`w,c` are bounded by one, matrix row sums by three, and the denominator by four.
Check finite vector components and row residual/norm values before any max
reduction (a comparison-based max can hide NaN). All resulting norms are finite;
check the denominator and residual again, then explicitly reject non-finite
`eta` before accepting `eta <= 100*epsilon`. A zero denominator is accepted only
with zero residual. Numerical validation failure returns `ill_conditioned`.
NaN/Inf inputs return `non_finite_input`; excessive asymmetry returns
`invalid_argument`. No non-finite diagnostic is allowed to pass silently.

### Limits, cost and evidence

Positive common scaling preserves the mathematical solution. It does not remove
input rounding, underflow of extremely small relative components, cancellation,
conditioning sensitivity or limitations of the output scalar. A result that
rounds to zero despite a nonzero normalized RHS fails the backward-error test.
No universal scale invariance or arbitrary-condition robustness is claimed.

The solver remains fixed-size O(1). Added work includes 12 input normalization
divisions, normalized residual evaluation, and six exponent-safe component
rescalings (18 `frexp` and six `scalbn` calls on the nonzero-RHS runtime path).
No performance benchmark or speed guarantee is provided.

`SymmetricLinearSolver3ScaleTest.cpp` tests analytic systems with condition
numbers 2, 3, 4, `3+2*sqrt(2)`, and approximately 19; scales include subnormals,
minimum normal values, double `1e-300` through `1e300`, and near-maximum finite
values. Independent long-double, power-of-two residual calculations validate the
original equation. Fault-injection tests explicitly cover non-finite diagnostic
rejection. These local checks do not establish full public API qualification.
Overall: **NOT REQUALIFIED / Experimental**.

---

## 9. Verification Evidence

- [`tests/Geometry/Matrix3Test.cpp`](../modules/VectorisNumerics/tests/Geometry/Matrix3Test.cpp): Adjoint cofactor inversion indices, aliasing prevention, pure numerical algebra.
- [`tests/Geometry/SymmetricLinearSolver3Test.cpp`](../modules/VectorisNumerics/tests/Geometry/SymmetricLinearSolver3Test.cpp): Analytic $LDL^T$ solve, backward error tracking, ill-conditioned rejection.
- [`tests/Geometry/AttitudeEngineTest.cpp`](../modules/VectorisNumerics/tests/Geometry/AttitudeEngineTest.cpp): Rotation composition, quaternion-to-matrix agreement.
- [`tests/Geometry/GeometryComparisonTest.cpp`](../modules/VectorisNumerics/tests/Geometry/GeometryComparisonTest.cpp): Regression tests cover `operator==`, `AlmostEqual`, `RotationEquivalent`, $180^\circ$ edge cases, near-zero $w$, and compile-time rejection guards.

---

## 10. Registered Deviations

1. **AML-DEVIATION-002**: Public mutable data members (`x, y, z` and `w, x, y, z`) for existing API compatibility and aggregate direct access (registered in [`docs/DEVIATIONS.md`](DEVIATIONS.md)).
2. **AML-DEVIATION-003 (Resolved)**: Namespace `vectoris::numerics::geometry` satisfies Section 8; deviation is formally closed (see [`docs/DEVIATIONS.md`](DEVIATIONS.md)).


## UnitVector3 scale-safe invariant (VRT-02)

`UnitVector3<T, Frame>` stores a dimensionless unit direction with an explicit
Frame. `T` and the input `Vector3<U, Frame>` scalar `U` must be floating-point
types; integer/custom arithmetic types cannot generally represent this invariant
and are now rejected by constraints. Both factory forms keep their existing
return types (`Result<UnitVector3>` and the compatibility `bool` output form).

### Normalization and success contract

1. Reject any NaN or positive/negative infinity component with `non_finite_input`.
2. Calculate in `std::common_type_t<T,U>`; use
   `s = max(abs(x), abs(y), abs(z))`. Finite inputs imply finite `s`.
3. Only exact `s == 0` (including signed zeros) is `zero_norm`. There is no
   absolute epsilon cutoff on the original magnitude.
4. Set `(ux,uy,uz) = (x/s,y/s,z/s)`. Each magnitude is at most one and at least
   one is one, so the squared norm is in `[1,3]`.
5. Set `n = sqrt(ux*ux + uy*uy + uz*uz)` and store `(ux/n,uy/n,uz/n)` in `T`.
   Never compute the original squared norm, reconstruct its magnitude or form `1/s`.
6. Return success only if the stored candidate passes `IsValid()`; otherwise
   return `normalization_failure`. The output-parameter form leaves its output
   unchanged on every failure.

`IsValid()` uses overflow/underflow-safe `std::hypot` on the stored components,
requires a finite norm, and compares it with one using absolute and relative
tolerances of `10 * numeric_limits<T>::epsilon()`. A finite `hypot` excludes
non-finite components. Thus every factory success is finite, approximately unit
length and `IsValid()==true`. The defensive `normalization_failure` path is
retained even though ordinary IEEE-754 float/double cases pass the postcondition.
Time and storage are O(1); there is no allocation, iteration or hidden state.

### Scale and cross-precision limits

Positive finite rescaling preserves direction to floating-point precision when
the rescaled input components remain representable. Scaling that already
underflows/overflows or rounds the input changes the represented input itself;
no normalization API can recover that lost information. Gradual underflow and
the ordinary IEEE-754 environment are assumed; FTZ/DAZ/fast-math are not qualified.

Float-to-double normalization uses double intermediate precision. Double-to-float
normalization happens before narrowing, so even a double input outside the float
magnitude range can yield a valid float unit direction. Unrepresentably small
normalized components may round to zero in float. The stored result must still
pass the float postcondition; exact preservation of every original component is
not promised. `long double` remains a permitted built-in floating type, but the
new deterministic qualification workload specifically covers float and double.

### Encapsulation and observable layout

All six component accessors return values. `ToVector()` returns an independent
copy. Direct/default construction is unavailable; factory construction is checked.
Copy/move construction and assignment preserve valid scalar representations;
unary minus preserves norm. Addition, subtraction and scalar multiplication
produce `Vector3`, not `UnitVector3`.

`GeometryTraits` no longer has private access: a caller-owned Frame specialization
previously could exploit friendship to return a mutable component reference.
Scalar-left multiplication is now a namespace function using value accessors,
with unchanged ordinary `scalar * direction` syntax and no privileged friendship.
Mixed scalar multiplication and `dot(Vector3<U>)` also pass component values to
user-defined arithmetic, so overloaded operators cannot obtain references to the
stored components. An adversarial but valid scalar that modifies its bound
operand is covered by regression tests; only the temporary copy can change.
The existing trait names/aliases remain; private `offsetof` checks are replaced
by the retained standard-layout, trivial-copy and total-size checks. Code relying
on traits friendship to inspect or mutate private storage is intentionally rejected.
Bytewise fabrication is not a supported validating constructor.

Float/double standard-layout and trivially-copyable properties are rechecked in
`UnitVector3Test.cpp`. Local x86_64 results are respectively size/alignment 12/4
and 24/8 bytes. These are observed type-layout properties, **not ABI stability**.

### Verification and qualification boundary

Deterministic float/double tests cover signed axes from smallest subnormal to
largest finite input, mixed large/small magnitudes, negative directions,
NaN/±Inf in every position, positive rescaling of `(1,-2,3)`, both precision
conversions, copying/moving, output-factory failure preservation and access control.
Oracles include analytic axes, `(1,-2,3)/sqrt(14)` and independent power-of-two
rescaling with long-double `hypot` (also safe when long double equals double).

See the incremental remediation ledger for live results. Project status remains
**NOT REQUALIFIED / Experimental**; this patch does not close later findings.


## VRT-04: Matrix multiplication overload contract

`Matrix3<T> * Vector3<U, F>` applies the existing row-major formula
`r_i = sum_j M_ij v_j`. Its result is `Vector3<decltype(std::declval<T>() * std::declval<U>()), F>`
(for types where that component arithmetic is valid), so matching `float` or
`double` inputs retain their scalar type and Frame. Mixed `float`/`double`
inputs promote to `double`; the matrix does not perform a coordinate transform.

`ScalarArithmetic` now rejects known geometry aggregates before asking whether
arithmetic expressions are valid. The new `is_geometry_aggregate_v` trait is
specialized for every `Matrix3<T>` and `Vector3<T,F>`; classification strips
cv/ref qualifiers. This prevents a scalar candidate from recursively asking
whether the same vector multiplication is valid. It also excludes matrices of
another scalar type, unlike the former exact-current-Matrix3 exclusion.
The existing arithmetic requirements remain for component scalars, including
user-defined scalar and unit types. No new implicit conversion or scalar type
is admitted by this concept change. This is a public compile-time API change;
new geometry aggregate types must be classified before scalar overload use.

The overload categories are:

- Matrix × scalar: the existing member, constrained by `ScalarArithmetic<S>`.
- Scalar × Matrix: a new namespace overload, with the same scalar contract;
  component multiplication keeps the written operand order.
- Matrix × Matrix: the existing same-scalar member; mixed matrix scalar types
  remain unsupported (there is no implicit Matrix conversion).
- Matrix × Vector: the existing member templated on the vector scalar and Frame.

Vector operator signatures and all existing `constexpr` / `noexcept` declarations
are unchanged. The new scalar-left Matrix overload is `constexpr` and conditionally `noexcept`
according to component multiplication (non-throwing for built-in arithmetic). No layout, storage, row-major
formula, transpose, determinant, or `TryInverse` algorithm changes are made.
There is no new Matrix × Point operation or implicit Frame conversion.

`Matrix3PublicMultiplicationTest.cpp` separately instantiates public expressions
for `float`, `double`, two Frames, arithmetic promotion, and a unit-valued vector.
It includes constant evaluation and negative constraints. Header isolation proves
independent inclusion, not every template instantiation. These selected API tests
and emitted-function coverage do not qualify the complete public template surface;
VRT-10 remains OPEN. Overall: **NOT REQUALIFIED / Experimental**.


## Square-root dependency contract (VRT-14)

The three current Geometry square-root call sites are UnitVector3::TryCreate's
scaled norm, Quaternion::TryCreate's scaled norm and Quaternion::Slerp's sine term.
They retain their existing `Core::Math::sqrt` calls, which now forward exactly to
canonical `core::sqrt`. No normalization, interpolation or LDLT equation changes
are part of VRT-14. Valid positive float/double runtime radicands still use std::sqrt;
negative nonzero radicands produce NaN rather than a fabricated zero.

Vector3 currently has no square-root/norm API; RotationMatrix3 and Transform3 have
no direct sqrt call. The symmetric solver remains sqrt-free LDLT. Long-double
runtime sqrt support is retained, including UnitVector3's documented long-double
inputs and storage; no portable long-double constexpr guarantee is added.
See [core.md §10](core.md#10-canonical-coresqrt-contract-vrt-14) for signed-zero,
NaN/Inf, subnormal, type, accuracy and compatibility semantics.

The normalization formula itself is unchanged. Its earlier no-iteration description
refers to the runtime normalization path; float/double sqrt constant evaluation
now uses the fixed 24/53-step digit algorithm described in core.md.

## Candidate #8: rotation evaluation and generic scalar exceptions

AFA-001 preserves Quaternion's public signatures and frame mappings. For built-in
floating scalars, rotating a finite vector by a valid unit quaternion derives
bounded rotation coefficients from the homogeneous quadratic coefficients divided
by the stored squared norm. Bounding coefficient roundoff to the mathematical
interval [-1,1] prevents an individual product from overflowing. Each row adds its
minimum and maximum products before the median, so cancelling signs meet before
an intermediate overflow. A conservative addition bound detects rows approaching
the representable limit **before** evaluating an overflowing sum. Those rows are
re-evaluated with two-component compensated products/sums, a corrected quotient,
and power-of-two input/output scaling. This prevents rounded matrix coefficients
from producing a false infinity even when the exact rotated component rounds to
max(). The extra precision works when MSVC long double equals double. Only these
boundary rows are scaled; ordinary rows retain isolated subnormal components.
This avoids scaling a mixed max/subnormal vector into one common exponent range
and losing its independent tiny components. It does not rely on FMA or compiler
contraction. No result is saturated to a finite sentinel. All-zero vectors preserve input zero signs;
other zero signs follow ordinary floating arithmetic. Final unrepresentable sums
may overflow; input finiteness and the existing valid-unit precondition still
apply. Ordinary rounding error remains; no exact rounding/bitwise cross-compiler
claim is made. The supported numerical environment is round-to-nearest, gradual
underflow and no fast-math. The operations remain constexpr for the supported float/double
constant-evaluation surface.

AFA-002 retains ScalarArithmetic support for user-defined arithmetic. Vector3's
value/default constructors, unary/binary arithmetic, scalar multiplication on both
sides, dot, equality/inequality and AlmostEqual now derive noexcept from their
actual construction/evaluation expressions. Implicit copy/move/assignment keep the
compiler-derived conditional exception specifications. The by-value constructor
preserves lvalue reference component bindings and otherwise consumes its local
parameters with std::move_if_noexcept, preserving copy-only
payload support and selecting a nonthrowing move when available; its specification
reflects that exact construction. Vector3 currently has no
public division, compound assignment, or cross member. Built-in float/double retain
nothrow behavior; exceptions thrown by supported custom scalar operations propagate
to the caller. This is an exception-specification correction, not a new mathematical
error channel. It changes noexcept type/trait observations for throwing scalars;
consumers must rebuild affected template instantiations. Layout is unchanged; no
binary ABI stability is promised.

### Generic scalar exception contract (AFA2-002)

`ScalarArithmetic` establishes available expressions; it does not require them,
construction, comparison, conversion or copying to be non-throwing. Vector3 uses
expression-based conditional exception specifications. Point3, Matrix3,
Quaternion, RotationMatrix3 and Transform3 composite operations guarantee
`noexcept` for built-in arithmetic scalar operands; custom scalar composites
conservatively permit exceptions, including result construction. UnitVector3's
floating-only invariant operations remain non-throwing; multiplication and dot
products involving custom scalar/vector operands permit their exceptions to
propagate. Implicit copy/move members keep compiler-inferred specifications.
Reference-only accessors remain non-throwing. FromQuaternion propagates the
complete conversion expression's exception specification, including for a
user-defined adapter. Object layouts and frame constraints are unchanged; custom
specialization exception traits change. This is not an ABI stability guarantee.

### Matrix3 scalar capabilities (Candidate #10)

`ScalarArithmetic<T>` admits scalar arithmetic expressions; it does not imply
IEEE-754 finite-value handling, epsilon, numerical limits or tolerance comparison.
`Matrix3<T>` construction and basic arithmetic retain the custom-scalar support
exercised by the throwing-scalar regression suite. Operations still require the
construction and scalar expressions they actually evaluate.

Matrix3 member/free `AlmostEqual`, and the corresponding Vector3, Point3,
Quaternion (`AlmostEqual` and `RotationEquivalent`), RotationMatrix3 and Transform3
comparison helpers require `Concepts::FloatingPoint<T>`, matching the existing
`Traits::AlmostEqual` policy. Both explicit-tolerance and default-tolerance
comparison are unavailable for an arithmetic-only custom scalar, including
ThrowingScalar; supplying a tolerance does not provide a finite/comparison policy.
No test-only NumericTraits specialization or custom comparison algorithm is added.

Matrix3 member comparison retains its existing signature and default-argument
call spellings. Defaults invoke a private delayed tolerance function whose body
uses NumericTraits only when a floating-point comparison is selected. Constructing
`Matrix3<ThrowingScalar>` never forms `NumericTraits<ThrowingScalar>` in a member
parameter declaration. Float/double default absolute/relative values
(`epsilon * 100`), one-tolerance calls and member-function pointer types remain
unchanged.
`TryInverse` requires `Concepts::Numeric<T>` to make its existing built-in
numerical-limits/epsilon policy visible; its formula, thresholds and float/double
behavior are unchanged. Custom-scalar arithmetic remains exception-propagating.
Consumers rebuild these header-only instantiations; no ABI stability is asserted.
