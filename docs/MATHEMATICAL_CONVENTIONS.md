# Vectoris Mathematical Conventions

> [!IMPORTANT]
> **Document**: Cross-Module Mathematical Conventions  
> **Document Version**: 1.0  
> **Status**: Authoritative Architectural Specification  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose & Authority Model

This document defines the mathematical conventions used throughout Vectoris to eliminate ambiguity across coordinate frames, spatial transformations, attitude representations, dimensional algebra, and physical equations.

### Documentation Authority & Precedence
- **Normative SSOT**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md) is the authoritative Single Source of Truth for all mathematical, engineering, and coding requirements.
- **Scoped Exception Mechanism**: [`docs/DEVIATIONS.md`](DEVIATIONS.md) is the Engineering Standard's formally authorized scoped exception mechanism per Sections 101 & 102.
- **Conventions & Specifications Hierarchy**: This conventions document and all subsystem module specifications ([`docs/core.md`](core.md), [`docs/units.md`](units.md), [`docs/geometry.md`](geometry.md), [`modules/VectorisDynamics/docs/dynamics.md`](../modules/VectorisDynamics/docs/dynamics.md)) must strictly conform to the **Engineering Standard plus applicable registered deviations**.
- **Historical Evidence**: Audit reports ([`docs/audits/`](audits/)) provide historical evidence and qualification ledgers only; they carry zero normative authority to alter or define requirements.

## 2. Numerical Convention

### 2.1 Floating-Point Scalar Type
The canonical floating-point type for all mathematical computations is:
```cpp
using Scalar = double; // IEEE-754 64-bit double precision
```
Rationale: Aerospace simulation accuracy, navigation state stability, and numerical integration over extended propagation intervals require 64-bit precision. Single precision (`float`) is supported where explicitly declared (e.g. `Quantity<float, MeterUnit>`).

---

## 3. Coordinate System Convention

Vectoris standardizes on a **Right-Handed Coordinate System** with positive rotations governed by the **Right-Hand Rule**.

### 3.1 Default Cartesian Frame (ENU Orientation)
```text
        Z+ (Up)
        |
        |
        |
        O────── X+ (Forward / East)
       /
      /
    Y+ (Right / North)
```

### 3.2 Aerospace Navigation Frame (NED Orientation)
Aerospace and navigation algorithms frequently operate in the North-East-Down (NED) frame:
```text
        North (X+)
        |
        |
        O────── East (Y+)
       /
      /
    Down (Z+)
```

> [!IMPORTANT]
> ENU and NED frames are mathematically distinct. Cross-frame conversion MUST be explicit using frame-tagged transformations: `Transform3<T, ENUFrame, NEDFrame>`.

---

## 4. Vector Convention

A spatial vector represents direction and magnitude in $\mathbb{R}^3$.
- **Storage Order**: Components are ordered as `[x, y, z]` without internal padding.
- **Affine Separation**: Vectors and points are distinct types:
  - $\text{Vector} + \text{Vector} \implies \text{Vector}$
  - $\text{Point} + \text{Vector} \implies \text{Point}$
  - $\text{Point} - \text{Point} \implies \text{Vector}$
  - $\text{Point} + \text{Point}$ is strictly forbidden at compile time.

---

## 5. Matrix Convention

### 5.1 Column Vector Mapping
Vectoris adopts the standard **column vector convention**:
$$\mathbf{v}' = \mathbf{M} \mathbf{v}$$

### 5.2 Storage Layout
`Matrix3<T>` stores 9 scalar components in **row-major order**:
```cpp
// Indexing: m[row][col] -> data_[row * 3 + col]
```
Row-major storage provides optimal cache locality for vector dot products and interoperability with standard linear algebra libraries.

---

## 6. Attitude & Rotation Conventions

### 6.1 Active Rotation
All rotation operators represent **active rotations** transforming vectors from `FromFrame` to `ToFrame`:
$$\mathbf{v}_{\text{Target}} = \mathbf{R}_{\text{Source} \to \text{Target}} \mathbf{v}_{\text{Source}}$$

### 6.2 Hamilton Quaternions
Quaternions follow the **Hamilton convention**:
$$q = w + x\mathbf{i} + y\mathbf{j} + z\mathbf{k}, \quad \mathbf{i}^2 = \mathbf{j}^2 = \mathbf{k}^2 = \mathbf{i}\mathbf{j}\mathbf{k} = -1$$
- **Storage Order**: `[w, x, y, z]` (scalar component first).
- **Unit Constraint**: Rotation quaternions produced through validated construction APIs satisfy $\|\mathbf{q}\| \approx 1$. (Because coordinate components $w, x, y, z$ remain public mutable members per AML-DEVIATION-002, unit normalization is a construction-time guarantee, not an immutable lifetime invariant).
- **Identity Factory**: `Quaternion<T, F, F>::Identity()` is constrained by the class's actual frames; legacy explicit arguments may name any same-frame pair but do not select the class mapping. They cannot make `Quaternion<T, F, H>` with `F != H` produce an identity.

### 6.3 Euler Angle Convention (Aerospace ZYX)
When converting to or from Euler angles, the canonical sequence is **Yaw-Pitch-Roll (ZYX)**:
$$\mathbf{R} = \mathbf{R}_z(\psi) \mathbf{R}_y(\theta) \mathbf{R}_x(\phi)$$
All internal angles are strictly represented in **Radians** ($[A^1]$).

---

## 7. Frame Transformation Pipeline Composition
 
As standardized in Vectoris (historically registered as AML-DEVIATION-001 before confirmation of full standard compliance with Sections 22–24), `operator*` on `RotationMatrix3` and `Quaternion` implements a **left-to-right transformation pipeline convention**:

$$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \implies \mathbf{R}_{A \to C}$$

When applied to a vector $\mathbf{v}_A$, the chained operator pipeline evaluates left-to-right:
$$(\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C}) * \mathbf{v}_A \approx \mathbf{R}_{B \to C} * (\mathbf{R}_{A \to B} * \mathbf{v}_A) = \mathbf{v}_C$$

Underlying Direction Cosine Matrix multiplication:
$$\mathbf{M}_{AC} = \mathbf{M}_{BC} \mathbf{M}_{AB} \quad (\texttt{rhs.ToMatrix() * dcm\_})$$

---

## 8. Physical Quantity Convention

Raw untyped scalars (`double`, `float`) are forbidden at public API boundaries where physical quantities are modeled. All physical values enter the `Units` system:
- Strict SI base units: `Meter`, `Second`, `Kilogram`, `Radian`, `Kelvin`, `Ampere`, `Mole`, `Candela`.
- Implicit dimension promotion or conversion is forbidden.

---

## 9. Gravity Convention

In the standard Cartesian coordinate frame ($Z+$ upward), gravitational acceleration is:
$$\mathbf{g} = [0, 0, -9.80665]^T \text{ m/s}^2$$
In the aerospace NED coordinate frame ($Z+$ downward), gravitational acceleration is:
$$\mathbf{g}_{\text{NED}} = [0, 0, +9.80665]^T \text{ m/s}^2$$

---

## 10. Summary Matrix

| Category | Standard Convention | Reference |
| :--- | :--- | :--- |
| **Coordinate System** | Right-handed, right-hand rule | Sec 3 |
| **Vector Layout** | Column vector, `[x, y, z]` | Sec 4, [`geometry.md`](geometry.md) |
| **Matrix Layout** | Row-major storage in memory | Sec 5, [`geometry.md`](geometry.md) |
| **Transformation Syntax** | Pipeline composition: $R_{AB} * R_{BC} \to R_{AC}$ | Sec 7, [`DEVIATIONS.md`](DEVIATIONS.md) |
| **Rotation Sense** | Active rotation | Sec 6, [`geometry.md`](geometry.md) |
| **Quaternion System** | Hamilton convention, storage `[w, x, y, z]` | Sec 6, [`geometry.md`](geometry.md) |
| **Euler Sequence** | ZYX (Yaw $\to$ Pitch $\to$ Roll) | Sec 6 |
| **Angle Base Unit** | Radian ($[A^1]$) | Sec 6, [`units.md`](units.md) |
| **Dimensional System** | Model B 8-dimensional system | Sec 11, [`units.md`](units.md) |
| **Default Scalar** | IEEE-754 `double` | Sec 2, [`core.md`](core.md) |

---

## 11. Rotational Dimensional Analysis & Angle Base Dimension Convention

Per Engineering Standard Section 10–13, Vectoris treats Plane Angle ($A = \text{Angle}$) as an independent semantic physical base dimension within its 8-dimensional system (`Length`, `Mass`, `Time`, `Current`, `Temperature`, `Amount`, `Luminosity`, `Angle`).

To preserve dimensional closure across rotational kinematics, dynamics, kinetic energy, work, torque, and power without algebraic inconsistency, rotational mechanical quantities carry compensating inverse-angle exponents:

1. **Angular Velocity ($\boldsymbol{\omega}$)**:
   - Dimension: $[A \cdot T^{-1}]$
   - Unit: $\text{rad}/\text{s}$
2. **Angular Acceleration ($\boldsymbol{\alpha}$)**:
   - Dimension: $[A \cdot T^{-2}]$
   - Unit: $\text{rad}/\text{s}^2$
3. **Rotational Kinetic Energy ($E = \frac{1}{2} I \omega^2$)**:
   - Dimension: $[M \cdot L^2 \cdot T^{-2}]$ (Joule)
   - Since $[\omega^2] = [A^2 \cdot T^{-2}]$, Moment of Inertia must carry $A^{-2}$:
4. **Moment of Inertia ($I$)**:
   - Dimension: $[M \cdot L^2 \cdot A^{-2}]$
   - Unit: $\text{kg}\cdot\text{m}^2/\text{rad}^2$
5. **Torque ($\tau = dE / d\theta = I \alpha$)**:
   - Dimension: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$
   - Unit: $\text{N}\cdot\text{m}/\text{rad}$
6. **Rotational Mechanical Power ($P = \boldsymbol{\tau} \cdot \boldsymbol{\omega}$)**:
   - $[P] = [\tau] \cdot [\omega] = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] \cdot [A \cdot T^{-1}] = [M \cdot L^2 \cdot T^{-3}]$ (Watt)
7. **Newton-Euler Rotational Law**:
   - $[I \cdot \alpha] = [M \cdot L^2 \cdot A^{-2}] \cdot [A \cdot T^{-2}] = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] = [\tau]$

### Dimensional Separation of Torque and Energy
In standard SI dimensional systems where angle is treated as dimensionless (1), Torque and Energy share the identical dimension $[M \cdot L^2 \cdot T^{-2}]$. Under Vectoris's 8-dimensional Model B:
- **Energy / Work**: $[M \cdot L^2 \cdot T^{-2} \cdot A^0]$ ($\text{J}$)
- **Torque**: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$ ($\text{N}\cdot\text{m}/\text{rad}$)
This compile-time separation prevents assigning translational energy or work directly to torque (or vice versa), eliminating common dimensional errors in aerospace GNC code.

### Quaternion Kinematics Boundary
Quaternion components are dimensionless scalars in $\mathbb{R}^4$ with unit constraint $\|\mathbf{q}\| = 1$. In quaternion kinematic integration ($\dot{\mathbf{q}} = \frac{1}{2} \mathbf{q} \otimes \boldsymbol{\omega}$), rotational rates typed in $[\text{rad}/\text{s}]$ ($[A \cdot T^{-1}]$) are explicitly extracted as numerical scalars in radians per second at the local, auditable boundary to drive quaternion integration without implicit dimensionless demotion.

---

## 12. Dimensionful Angle and SO(3) Lie Algebra

When Plane Angle is treated as an independent physical dimension ($A$), the ordinary Cartesian cross product and the $\mathfrak{so}(3)$ Lie bracket / adjoint operation represent dimensionally distinct operations.

### 12.1 Ordinary Cross Product vs. Lie Bracket
- **Ordinary Vector Cross Product**:
  For general 3D vectors $\mathbf{u}, \mathbf{v}$, the cross product computes Cartesian components via $\mathbf{u} \times \mathbf{v}$. Its dimension is the strict product of the operand dimensions:
  $$[\mathbf{u} \times \mathbf{v}] = [\mathbf{u}] \cdot [\mathbf{v}]$$
- **Rotational Lie Bracket / Adjoint Operation ($\mathfrak{so}(3)$)**:
  In rigid body dynamics, the gyroscopic cross-coupling term $\boldsymbol{\omega} \times \mathbf{L}$ arises from the Lie algebra adjoint action $\operatorname{ad}_{\boldsymbol{\omega}} \mathbf{L} = [\boldsymbol{\omega}, \mathbf{L}]_{\mathfrak{so}(3)}$.
  Evaluating dimensions with Angle $A$:
  $$[\boldsymbol{\omega}] = [A \cdot T^{-1}], \quad [\mathbf{L}] = [M \cdot L^2 \cdot A^{-1} \cdot T^{-1}]$$
  The ordinary component-wise cross product yields:
  $$[\boldsymbol{\omega} \times \mathbf{L}] = [A \cdot T^{-1}] \cdot [M \cdot L^2 \cdot A^{-1} \cdot T^{-1}] = [M \cdot L^2 \cdot T^{-2}]$$
  This has dimension of **Energy / Work ($A^0$)**, NOT **Torque ($A^{-1}$)**.

### 12.2 Explicit Radian Normalization Factor
Standard engineering textbooks treat the radian as dimensionless ($1$), implicitly suppressing the normalization factor $1 / \text{rad}$. Under Vectoris's rigorous 8-dimensional type system, this hidden convention is forbidden.

The true Lie bracket operation in dimensionful mechanics carries an explicit normalization by $1\text{ rad}$ ($[A^1]$):
$$\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L}) \triangleq \frac{\boldsymbol{\omega} \times \mathbf{L}}{1\text{ rad}}$$
Dimensionally:
$$[\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L})] = \frac{[M \cdot L^2 \cdot T^{-2}]}{[A^1]} = [M \cdot L^2 \cdot A^{-1} \cdot T^{-2}] \equiv [\boldsymbol{\tau}]$$

### 12.3 Compile-Time Type Safety Directives
1. **Generic Cross Product Preservation**: `Cross(a, b)` remains a pure Cartesian vector product. It must never implicitly divide by radians.
2. **Dedicated Rotational Operator**: Rotational dynamics code MUST call `RotationalCross(omega, L)` (or `LieBracket(omega, L)`).
3. **Compile-Time Static Guard**: The type system statically prevents subtracting `Cross(omega, L)` from `Torque3`, because $[M \cdot L^2 \cdot T^{-2} \cdot A^0] \neq [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$.
4. **Consistency with Quaternion Boundary**: $SO(3)$ rotational kinematics similarly consume dimensionless coordinates by explicitly extracting radian scalars at the integration boundary, ensuring total consistency across the library.

---

## 13. Geometry Semantics, Matrix3 Scope, and Floating-Point Comparisons

### 13.1 Matrix3 Scope & Frame Separation
- **Unframed Generic Numerical Matrix**: `Matrix3<T>` is a generic numerical matrix container in $\mathbb{R}^{3 \times 3}$. It may represent coefficients, Jacobians, covariance blocks, solver matrices, or temporarily extracted tensor coefficients. It does not itself imply a geometric frame transformation and intentionally carries no coordinate frame tags (`FromFrame`, `ToFrame`, or `FrameType`).
- **Generic Linear Algebra Preservation**: Pure numerical routines—such as the $LDL^T$ symmetric positive definite solver (`SolveSymmetricPositiveDefinite3x3`), matrix determinants, adjoint inverses, covariances, and Jacobians—MUST accept `Matrix3<T>` without coordinate frame tags.
- **Dedicated Spatial Transformation Wrappers**: Coordinate frame semantics (`FromFrame -> ToFrame`) are strictly carried by semantic geometric types:
  - `RotationMatrix3<T, FromFrame, ToFrame>`
  - `Transform3<T, FromFrame, ToFrame>`
  - `Quaternion<T, FromFrame, ToFrame>`
- **Typed Inertia Bridge**: Inertia tensors (`InertiaTensor3<T, BodyFrame>`) hold coordinate frame tags; they construct local `Matrix3<T>` instances purely to invoke numerical solvers, preventing frame tags from polluting general linear algebra.

### 13.2 Coordinate Transformation & Composition Convention
- **Vector Transformation**:
  $$\mathbf{v}_{\text{Target}} = \mathbf{R}_{\text{Source} \to \text{Target}} \mathbf{v}_{\text{Source}}$$
- **Rotation Composition (Pipeline Convention)**:
  `operator*` on `RotationMatrix3` represents left-to-right frame transformation pipeline composition, not raw `Matrix3` multiplication order:
  $$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \to \mathbf{R}_{A \to C}$$
  In standard linear algebra, the transformation pipeline evaluates:
  $$\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$$
  Therefore, the underlying Direction Cosine Matrix product evaluates $\mathbf{M}_{AC} = \mathbf{M}_{BC} \mathbf{M}_{AB}$ (`rhs.ToMatrix() * dcm_`).
  The composition satisfies pipeline associativity when applied to vectors:
  $$(\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C}) * \mathbf{v}_A \approx \mathbf{R}_{B \to C} * (\mathbf{R}_{A \to B} * \mathbf{v}_A)$$
- **Quaternion to Rotation Matrix Agreement**:
  For any unit quaternion $\mathbf{q} \in SO(3)$ and vector $\mathbf{v}$:
  $$\mathbf{q} * \mathbf{v} \equiv \mathbf{R}(\mathbf{q}) * \mathbf{v}$$

### 13.3 Floating-Point Equality & Comparison Policy
In compliance with Rule 9 ("Never trust floating-point equality"):
1. **Exact Component-Wise Stored-Value Equality (`operator==`, `operator!=`)**:
   - Compares stored IEEE-754 components directly under standard C++ floating-point `==` semantics (`a.x == b.x && a.y == b.y && a.z == b.z`).
   - It is exact component-wise value equality, NOT bitwise equality:
     - $+0.0 == -0.0$ evaluates to `true` despite differing sign bits.
     - $\text{NaN} == \text{NaN}$ evaluates to `false`.
   - Reserved strictly for exact value checks: sentinel states, exact identity matrix initialization, serialization round-trips, and transactional immutability assertions.
   - FORBIDDEN for numerical convergence, solver residuals, or computed geometry comparisons.
2. **Tolerance-Aware Closeness (`AlmostEqual`)**:
   - All geometry types (`Vector3`, `Point3`, `Matrix3`, `UnitVector3`, `RotationMatrix3`, `Transform3`, `Quaternion`) provide `AlmostEqual(a, b, abs_tol, rel_tol)` delegating to `Traits::AlmostEqual`.
   - Thresholds combine absolute and relative tolerances: $|a - b| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a|, |b|))$.
3. **Rotational Equivalence (`RotationEquivalent`) & Canonicalization Policy**:
   - Quaternions represent $SO(3)$ rotations via a double cover ($\mathbb{S}^3 \to SO(3)$), meaning $\mathbf{q}$ and $-\mathbf{q}$ represent the identical physical orientation.
   - **Construction-Time Sign Normalization (Option A)**: `Quaternion::TryCreate` applies exact deterministic sign normalization (`w < 0 -> negate`), guaranteeing $w \ge 0$ whenever $w \ne 0$. For 180-degree pure vector rotations where $w == 0$ (e.g. $[0, 1, 0, 0]$ and $[0, -1, 0, 0]$), sign canonicalization is degenerate and does not enforce lexicographical uniqueness on the vector part.
   - **Transitional Mutability Policy**: Quaternion components (`w, x, y, z`) remain public data members under AML-DEVIATION-002 for source compatibility and direct scalar access. Standard-layout/trivially-copyable checks do not establish a cross-build ABI, telemetry wire format, DMA mapping, or persistent representation. Therefore, canonicalization is a construction-time guarantee, NOT an immutable lifetime invariant.
   - **Independent $SO(3)$ Equivalence**: `RotationEquivalent(q1, q2)` is independent of whether instances are canonical or modified. It checks both $\mathbf{q}_1 \approx \mathbf{q}_2$ and $\mathbf{q}_1 \approx -\mathbf{q}_2$, providing robust tolerance-aware $SO(3)$ equivalence across all orientations, including $\theta = \pi$ ($w = 0$), near-zero $w$, and non-canonical representations.
4. **Rotation Invariant Tolerance**:
   - The orthogonality check $\mathbf{R}^T \mathbf{R} \approx \mathbf{I}$ uses the squared Frobenius norm:
     $$\|\mathbf{R}^T \mathbf{R} - \mathbf{I}\|_F^2 \le \tau^2$$
     guaranteeing exact scale consistency between linear tolerance $\tau$ and the quadratic norm.
