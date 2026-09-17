# Vectoris Dynamics Module Specification

> [!IMPORTANT]
> **Document**: Dynamics Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Dynamics` module implements rigid-body dynamics, rotational mechanics, spatial wrench/twist abstractions, strong physical quantity vectors, and deterministic numerical integration for simulation and aerospace guidance applications.

---

## 2. Scope & Dependencies

- **Layer Position**: Layer 3 (Highest Layer in Stable Core).
- **Dependencies**: Depends strictly on `Core`, `Units`, and `Geometry`.
- **Inbound Dependencies**: Consumed by simulation systems, flight dynamics engines, and tests.
- **One-Way Chain**: `Core` $\to$ `Units` $\to$ `Geometry` $\to$ `Dynamics`. Lower layers never depend on `Dynamics`.

---

## 3. Public Headers

The `VectorisDynamics` module exposes **11 public headers** under `include/VectorisDynamics/`:

| Header | Description |
| :--- | :--- |
| [`Concepts.h`](../include/VectorisDynamics/Concepts.h) | Dynamics concepts (`DynamicsScalar`, `DynamicsFrameTag`). |
| [`Detail/DynamicsABI.h`](../include/VectorisDynamics/Detail/DynamicsABI.h) | ABI standard layout, trivial copyability, and memory padding validation. |
| [`Detail/StateTypes.h`](../include/VectorisDynamics/Detail/StateTypes.h) | Kinematic state definition and spatial vector type bindings. |
| [`DynamicsConvention.h`](../include/VectorisDynamics/DynamicsConvention.h) | Body frame, inertial frame, and gravitational acceleration conventions. |
| [`EulerIntegrator.h`](../include/VectorisDynamics/EulerIntegrator.h) | Transactionally safe 1st-order semi-implicit Euler state integrator. |
| [`InertiaTensor3.h`](../include/VectorisDynamics/InertiaTensor3.h) | Symmetric positive definite $3 \times 3$ rigid-body inertia tensor. |
| [`QuantityVector3.h`](../include/VectorisDynamics/QuantityVector3.h) | 3D spatial vector whose components are strongly typed `Quantity` instances. |
| [`RigidBodyParameters.h`](../include/VectorisDynamics/RigidBodyParameters.h) | Mass, center of mass offset, and inertia tensor parameters. |
| [`RigidBodyState.h`](../include/VectorisDynamics/RigidBodyState.h) | Full 6-DOF rigid-body dynamics derivative kernel and coupling equations. |
| [`Twist6.h`](../include/VectorisDynamics/Twist6.h) | 6-DOF spatial velocity (linear velocity + angular velocity). |
| [`Wrench6.h`](../include/VectorisDynamics/Wrench6.h) | 6-DOF spatial force and torque (linear force + rotational moment). |

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
$$\mathbf{I} \dot{\boldsymbol{\omega}} + \boldsymbol{\omega} \times (\mathbf{I} \boldsymbol{\omega}) = \boldsymbol{\tau}_{\text{ext}}$$

Under Vectoris's 8D Model B, $[\boldsymbol{\omega} \times \mathbf{L}]$ evaluates to Energy ($[M \cdot L^2 \cdot T^{-2}]$), not Torque ($[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$).
- The gyroscopic torque is computed via `LieBracket(omega, L)` or `RotationalCross(omega, L)`:
  $$\boldsymbol{\tau}_{\text{gyro}} \triangleq \frac{\boldsymbol{\omega} \times \mathbf{L}}{1\text{ rad}}$$
- This explicitly incorporates the radian normalization factor $1/\text{rad}$, restoring dimensional closure to Torque without numerical distortion.

---

## 7. Full Rigid-Body Dynamics Kernel

[`RigidBodyDynamicsKernel::ComputeDerivative`](../include/VectorisDynamics/RigidBodyState.h) evaluates the full Newton-Euler equations without diagonal-only simplifications:
1. Translational acceleration (resolved in `BodyFrame`, accounting for rotation transport / Coriolis coupling):
   $$\mathbf{a}_{\text{body}} = \frac{\mathbf{F}_{\text{body}}}{m} - \boldsymbol{\omega}_{\text{body}} \times \mathbf{v}_{\text{body}}$$
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

[`EulerIntegrator::Step`](../include/VectorisDynamics/EulerIntegrator.h) implements a deterministic, 1st-order semi-implicit integration step:

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
- Timestep $dt$ must be finite and strictly positive ($dt > 0$).
- $dt \le 0$ returns `Result::failure(MathError::invalid_argument)`.
- Non-finite $dt$ returns `Result::failure(MathError::non_finite_input)`.

### 8.2 Transactional Safety Semantics
- **Zero Partial State Commitment**: Integration operates on a local stack copy (`auto candidate = state;`).
- All derivatives, body velocity increments ($\mathbf{v}_{\text{body}} += \mathbf{a}_{\text{body}} \Delta t$, $\boldsymbol{\omega} += \boldsymbol{\alpha} \Delta t$), reference position advance ($\mathbf{r}_{\text{ref}} += (\mathbf{q} * \mathbf{v}_{\text{body}}) \Delta t$), and quaternion attitude kinematics are calculated and verified.
- The external caller state reference is mutated (`state = candidate;`) **if and only if all operations succeed**, returning `Result<bool, MathError>::success(true)`.
- On any numerical or domain failure (e.g. singular inertia, invalid timestep, non-finite wrench), the function returns `Result::failure(err)` and the caller's state remains **100% untouched**.

### 8.3 Attitude Kinematics Boundary
- Quaternion kinematics $\dot{\mathbf{q}} = \frac{1}{2} \mathbf{q} \otimes \boldsymbol{\omega}_{\text{body}}$ consume dimensionless quaternion coordinates.
- Angular velocity components in $\text{rad}/\text{s}$ are extracted as explicit scalars at the local integration boundary:
  $$\mathbf{q}_{k+1} = \operatorname{normalize}\left(\mathbf{q}_k + \frac{1}{2} \mathbf{q}_k \otimes \boldsymbol{\omega} \Delta t\right)$$

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
