# Vectoris Dynamics Module Specification

> [!IMPORTANT]
> **Document**: Dynamics Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Reviewed Starting HEAD**: `cfbecc6390fb30d857f10f2516f39bb2ef75f996` (working tree contains uncommitted remediations)
> **Last Updated**: 2026-09-23
> **Current Qualification**: NOT REQUALIFIED / Experimental
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](../../../docs/ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Dynamics` module implements rigid-body dynamics, rotational mechanics, spatial wrench/twist abstractions, strong physical quantity vectors, and deterministic numerical integration for simulation and aerospace guidance applications.

### Numerics namespace spelling (VRT-12)

The Dynamics namespace remains `vectoris::dynamics`. New callers should use
`vectoris::numerics::core`, `units`, and `geometry` for Numerics APIs. Historical
`Core::`, `Units::`, and `Geometry::` references in the examples below denote
the corresponding `vectoris::numerics` compatibility namespaces; they remain
supported and are not deprecated. Numerics public headers provide their aliases
independently of include order. No `vectoris::dynamics::geometry` (or equivalent
core/units alias) is introduced. This naming clarification changes no Dynamics
state, units, Frame, timestep or numerical contract.

---

## 2. Scope & Dependencies

- **Layer Position**: Layer 3 (highest active module layer; project status remains Experimental).
- **Dependencies**: Depends strictly on `Core`, `Units`, and `Geometry`.
- **Inbound Dependencies**: Consumed by simulation systems, flight dynamics engines, and tests.
- **One-Way Chain**: `Core` $\to$ `Units` $\to$ `Geometry` $\to$ `Dynamics`. Lower layers never depend on `Dynamics`.

---

## 3. Public Headers

The `VectorisDynamics` module exposes 11 public headers under `include/Vectoris/Dynamics/`:

| Header | Description |
| :--- | :--- |
| [`Concepts.h`](../include/Vectoris/Dynamics/Concepts.h) | Dynamics concepts (`DynamicsScalar`, `DynamicsFrameTag`). |
| [`Detail/DynamicsABI.h`](../include/Vectoris/Dynamics/Detail/DynamicsABI.h) | C++ standard-layout, trivial-copyability, and size checks; not a cross-build ABI or wire-format guarantee. |
| [`Detail/StateTypes.h`](../include/Vectoris/Dynamics/Detail/StateTypes.h) | Kinematic state definition and spatial vector type bindings. |
| [`DynamicsConvention.h`](../include/Vectoris/Dynamics/DynamicsConvention.h) | Body frame, inertial frame, and gravitational acceleration conventions. |
| [`EulerIntegrator.h`](../include/Vectoris/Dynamics/EulerIntegrator.h) | Transactionally safe 1st-order semi-implicit Euler state integrator. |
| [`InertiaTensor3.h`](../include/Vectoris/Dynamics/InertiaTensor3.h) | Symmetric positive definite $3 \times 3$ rigid-body inertia tensor. |
| [`QuantityVector3.h`](../include/Vectoris/Dynamics/QuantityVector3.h) | 3D spatial vector whose components are strongly typed `Quantity` instances. |
| [`RigidBodyParameters.h`](../include/Vectoris/Dynamics/RigidBodyParameters.h) | Mass, center of mass offset, and inertia tensor parameters. |
| [`RigidBodyState.h`](../include/Vectoris/Dynamics/RigidBodyState.h) | Full 6-DOF rigid-body dynamics derivative kernel and coupling equations. |
| [`Twist6.h`](../include/Vectoris/Dynamics/Twist6.h) | 6-DOF spatial velocity (linear velocity + angular velocity). |
| [`Wrench6.h`](../include/Vectoris/Dynamics/Wrench6.h) | 6-DOF spatial force and torque (linear force + rotational moment). |

---

## 4. `QuantityVector3` & Spatial Types

`QuantityVector3<QuantityType, FrameTag>` combines compile-time coordinate frames with strongly typed physical dimensions:
- `Velocity3<Frame, T>`: Linear velocity $[L \cdot T^{-1}]$ in `Frame`.
- `Acceleration3<Frame, T>`: Linear acceleration $[L \cdot T^{-2}]$ in `Frame`.
- `AngularVelocity3<Frame, T>`: Angular velocity $[A \cdot T^{-1}]$ in `Frame`.
- `AngularAcceleration3<Frame, T>`: Angular acceleration $[A \cdot T^{-2}]$ in `Frame`.
- `Force3<Frame, T>`: Net force $[M \cdot L \cdot T^{-2}]$ in `Frame`.
- `Torque3<Frame, T>`: Net torque $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$ in `Frame`.

Bypassing the units system via raw float scalars is prohibited.

---

## 5. `InertiaTensor3<T, BodyFrame>` Contract

`InertiaTensor3<T, BodyFrame>` represents the $3 \times 3$ rigid-body mass moment of inertia tensor about the center of mass in `BodyFrame`:

### 5.1 Three-Tier Validity Contract (`IsValid()`)
A valid inertia tensor must strictly satisfy:
1. **Finiteness**: All components $I_{xx}, I_{yy}, I_{zz}, I_{xy}, I_{xz}, I_{yz}$ must be finite (rejects $\text{NaN}$ and $\pm\infty$).
2. **Symmetry**: Off-diagonal elements must satisfy $|I_{ij} - I_{ji}| \le \epsilon$.
3. **Strict Positive Definiteness via $LDL^T$ Pivot Verification**:
   - The matrix must have strictly positive pivots ($D_1 > 0, D_2 > 0, D_3 > 0$) under Cholesky $LDL^T$ factorization.
   - Matrices with positive diagonal entries that are indefinite or singular (e.g. large off-diagonal coupling) are strictly rejected.

### 5.2 Linear System Solution (`TypedSolveSPD`)
Solves $I \boldsymbol{\alpha} = \boldsymbol{\tau}$ for angular acceleration $\boldsymbol{\alpha}$:
- Directly delegates to `Geometry::SymmetricLinearSolver3<T>::SolveSymmetricPositiveDefinite3x3`.
- Strictly adheres to the **Solve-Not-Invert** rule: the inverse matrix $I^{-1}$ is never formed or stored.

---

## 6. Rotational Lie Bracket & Gyroscopic Torque

In the rotating body frame, the Newton-Euler rotational dynamics equation is:
$$\mathbf{I} \dot{\boldsymbol{\omega}} + \frac{\boldsymbol{\omega} \times (\mathbf{I} \boldsymbol{\omega})}{1\text{ rad}} = \boldsymbol{\tau}_{\text{ext}}$$

Under Vectoris's 8D Model B, $[\boldsymbol{\omega} \times \mathbf{L}]$ evaluates to Energy ($[M \cdot L^2 \cdot T^{-2}]$), not Torque ($[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$).
- The gyroscopic torque is computed via `LieBracket(omega, L)` or `RotationalCross(omega, L)`:
  $$\boldsymbol{\tau}_{\text{gyro}} \triangleq \frac{\boldsymbol{\omega} \times \mathbf{L}}{1\text{ rad}}$$
- This explicitly incorporates the radian normalization factor $1/\text{rad}$, restoring dimensional closure to Torque without numerical distortion.

Use generic `Cross` for ordinary dimensional algebra: it preserves the product dimensions and the shared `Frame`. Use `RotationalCross` only when the physical rotational equation interprets the angular coordinate in radians and therefore consumes one Angle dimension. In particular, generic `Cross(AngularVelocity3, Velocity3)` has dimension $A L T^{-2}$; it is not an acceleration. The transport term uses `RotationalCross(AngularVelocity3, Velocity3) = (\omega \times v)/(1\text{ rad})`, with result dimension $L T^{-2}$. Both APIs require matching Frames at compile time.

---

## 7. Full Rigid-Body Dynamics Kernel

[`RigidBodyDynamicsKernel::ComputeDerivative`](../include/Vectoris/Dynamics/RigidBodyState.h) evaluates the full Newton-Euler equations without diagonal-only simplifications:
1. Translational acceleration (resolved in `BodyFrame`, accounting for rotation transport / Coriolis coupling):
   $$\mathbf{a}_{\text{body}} = \frac{\mathbf{F}_{\text{body}}}{m} - \frac{\boldsymbol{\omega}_{\text{body}} \times \mathbf{v}_{\text{body}}}{1\text{ rad}}$$
   The implementation calls `RotationalCross(angularVelocity, linearVelocity)`. Generic `Cross` continues to return the $A L T^{-2}$ dimensional product.
   where `linearVelocity` is stored and resolved in `BodyFrame` (`Velocity3<BodyFrame, T>`).
2. Angular momentum:
   $$\mathbf{L} = \mathbf{I} \boldsymbol{\omega}$$
3. Gyroscopic cross-coupling:
   $$\boldsymbol{\tau}_{\text{gyro}} = \operatorname{LieBracket}(\boldsymbol{\omega}, \mathbf{L})$$
4. Net rotational torque:
   $$\boldsymbol{\tau}_{\text{net}} = \boldsymbol{\tau}_{\text{ext}} - \boldsymbol{\tau}_{\text{gyro}}$$
5. Angular acceleration solve via analytic $LDL^T$ decomposition:
   $$\mathbf{I} \boldsymbol{\alpha} = \boldsymbol{\tau}_{\text{net}} \implies \boldsymbol{\alpha} = \operatorname{SolveSPD}(\mathbf{I}, \boldsymbol{\tau}_{\text{net}})$$

If the inertia tensor is singular, indefinite, or ill-conditioned, `ComputeDerivative` returns the exact failure code propagated by `SolveSPD` / `SolveSymmetricPositiveDefinite3x3` (`MathError::singular_matrix`, `MathError::invalid_state`, `MathError::ill_conditioned`, or `MathError::non_finite_input`).

---

## 8. `EulerIntegrator` Contract

[`EulerIntegrator::Step`](../include/Vectoris/Dynamics/EulerIntegrator.h) implements a deterministic, 1st-order semi-implicit integration step:

```cpp
template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
static constexpr Core::Result<bool, Core::MathError> Step(
    KinematicState<T, RefFrame, BodyFrame>& state,
    const RigidBodyParameters<T, BodyFrame>& params,
    const Wrench6<T, BodyFrame>& wrench,
    Units::Quantity<T, Units::SecondUnit> dt
) noexcept;
```

### 8.1 Input Preconditions
- **Timestep ($dt$)**: Must be finite and strictly positive ($dt > 0$).
  - $dt \le 0$ returns `Result::failure(MathError::invalid_argument)`.
  - Non-finite $dt$ returns `Result::failure(MathError::non_finite_input)`.
- **Rigid Body Mass ($m$)**: Must be finite and strictly positive ($m > 0$).
  - $m \le 0$ returns `Result::failure(MathError::invalid_argument)`.
  - Non-finite $m$ ($\text{NaN}$, $\pm\infty$) returns `Result::failure(MathError::non_finite_input)`.
- **Wrench ($\mathbf{F}_{\text{body}}, \boldsymbol{\tau}_{\text{body}}$)**: All force and torque components must be finite.
  - Any non-finite component returns `Result::failure(MathError::non_finite_input)`.
- **Initial Kinematic State**: All components of position, linear velocity, angular velocity, and attitude quaternion must be finite.
  - Any non-finite initial state component returns `Result::failure(MathError::non_finite_input)`.

### 8.2 Candidate State Validation & Transactional Safety Semantics
- **Zero Partial State Commitment**: Integration operates exclusively on a local candidate copy (`auto candidate = state;`).
- **Translational Candidate Validation**:
  - Candidate linear velocity $\mathbf{v}_{k+1} = \mathbf{v}_k + \mathbf{a}_k \Delta t$ is computed and verified finite.
  - Projected reference velocity $\mathbf{v}_{\text{ref}} = \mathbf{q}_k * \mathbf{v}_{k+1}$ and candidate position $\mathbf{r}_{k+1} = \mathbf{r}_k + \mathbf{v}_{\text{ref}} \Delta t$ are computed and verified finite.
  - If numerical overflow produces non-finite translation components, the step aborts with `Result::failure(MathError::non_finite_input)`.
- **Rotational Candidate Validation**:
  - Candidate angular velocity $\boldsymbol{\omega}_{k+1} = \boldsymbol{\omega}_k + \boldsymbol{\alpha}_k \Delta t$ is verified finite.
  - Candidate quaternion attitude $\mathbf{q}_{k+1}$ is normalized via `Quaternion::TryCreate` and verified valid.
- **Atomic Commit**:
  - The external caller state reference is mutated (`state = candidate;`) **if and only if all operations and candidate validations succeed**, returning `Result<bool, MathError>::success(true)`.
  - On any numerical, physical, or domain failure (singular inertia, invalid mass/timestep, non-finite wrench/state, or candidate overflow), the function returns `Result::failure(err)` and the caller's state remains **100% untouched** across all 13 kinematic state scalar fields.

### 8.3 Attitude Kinematics Boundary
- Quaternion coordinates are dimensionless. The angular increment is first formed as the typed quotient $\Delta\boldsymbol{\theta} = (\boldsymbol{\omega}\Delta t)/(1\text{ rad})$, whose dimension is zero.
- Only after that explicit radian normalization is its scalar representation used in the quaternion update:
  $$\mathbf{q}_{k+1} = \operatorname{normalize}\left(\mathbf{q}_k + \frac{1}{2} \mathbf{q}_k \otimes \Delta\boldsymbol{\theta}\right)$$
- This conversion preserves the SI numeric value because one radian has scalar value one; extracting an unnormalized angular-velocity value and multiplying it by raw `dt` is not the dimensional contract.

---

## 9. Verification Evidence

- [`tests/Dynamics/DynamicsConceptTest.cpp`](../tests/DynamicsConceptTest.cpp): Scalar constraints and frame typing.
- [`tests/Dynamics/InertiaTensorTest.cpp`](../tests/InertiaTensorTest.cpp): Valid SPD checks, rejection of indefinite matrices with positive diagonals, symmetry checks, typed SPD solve.
- [`tests/Dynamics/RigidBodyStateTest.cpp`](../tests/RigidBodyStateTest.cpp): Non-diagonal inertia coupling matching reference analytic solutions, torque-free asymmetric body precession.
- [`tests/Dynamics/EulerDynamicsTest.cpp`](../tests/EulerDynamicsTest.cpp): Torque-free angular momentum conservation, transactional safety on singular inertia, timestep validation.
- [`tests/Dynamics/PropagationTest.cpp`](../tests/PropagationTest.cpp): Frame transformation propagation, attitude kinematic integration.
- [`tests/Dynamics/Regression/FreeFallTest.cpp`](../tests/Regression/FreeFallTest.cpp): Vertical drop under gravity, 1st-order convergence rate verification ($\mathcal{O}(\Delta t)$).
- [`tests/Dynamics/DynamicsUnitsTest.cpp`](../tests/DynamicsUnitsTest.cpp): Dimensional algebra assertions, combined frame and unit safety.

---

## 10. Known Deviations

1. **AML-DEVIATION-003 (Resolved)**: Namespace `vectoris::dynamics` satisfies Section 8; deviation is formally closed (see [`../../../docs/DEVIATIONS.md`](../../../docs/DEVIATIONS.md)).
