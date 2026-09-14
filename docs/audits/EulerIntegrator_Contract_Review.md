# EulerIntegrator Mathematical Contract Review & Remediation Analysis

> **Document Type**: Technical Review & Mathematical Contract Analysis
> **Target Subsystem**: `include/AegisMath/Dynamics/EulerIntegrator.h`
> **Governing Baseline**: `docs/ENGINEERING_STANDARD_V1.md` (Sections 10-13, 22-24, 60, 63)
> **Status**: Contract Review Completed — Implementation Pending User Approval

---

## 1. Executive Summary

A formal mathematical and architectural contract review was conducted on `AegisMath::Dynamics::EulerIntegrator` and its associated regression test `tests/Dynamics/Regression/FreeFallTest.cpp`.

The review established that the failure of `RegressionFreeFall.VerticalDrop` is **not a single isolated bug**, but rather the intersection of three distinct issues:
1. **Mathematical Discretization Error vs. Test Expectation**: `EulerIntegrator` implements a 1st-order **Semi-Implicit Euler** (Symplectic Euler) scheme. Over $t = 1.0\text{ s}$ with step size $\Delta t = 0.01\text{ s}$, Semi-Implicit Euler inherently produces an exact discrete truncation discrepancy of $\Delta z = \frac{1}{2} g t \Delta t \approx 0.049033\text{ m}$. The regression test asserted an analytical continuous solution with an overly strict tolerance of $0.01\text{ m}$, creating a fundamental test-contract mismatch.
2. **Coordinate Frame Mixing Bug**: `KinematicState::position` is defined in `ReferenceFrame`, whereas `KinematicState::linearVelocity` is defined in `BodyFrame`. The integrator directly adds body-frame velocity to reference-frame position without rotating through the attitude quaternion ($\mathbf{v}_{ref} = \mathbf{q} \cdot \mathbf{v}_{body} \cdot \mathbf{q}^*$). In `FreeFallTest`, this bug was masked because vehicle attitude happened to be identity ($\mathbf{q} = [1, 0, 0, 0]$).
3. **Missing Rotational Kinematics**: The integrator computes and integrates `angularVelocity`, but **never integrates vehicle attitude `attitude`** ($\dot{\mathbf{q}} = \frac{1}{2} \mathbf{q} \otimes \boldsymbol{\omega}$ is completely omitted). Vehicle attitude remains forever frozen regardless of angular rates.

---

## 2. Inferred Contract & Coordinate Frame Analysis

### 2.1 State Definition (`KinematicState<T, RefFrame, BodyFrame>`)
As defined in `include/AegisMath/Dynamics/Detail/StateTypes.h`:

| Field | Type | Declared Frame | Physical Meaning |
| :--- | :--- | :--- | :--- |
| `position` | `Point3<T, RefFrame>` | `RefFrame` (e.g. ECEF/NED) | Spatial position in inertial/reference frame |
| `attitude` | `Quaternion<T, BodyFrame, RefFrame>` | `BodyFrame` $\to$ `RefFrame` | Active rotation mapping body vectors to reference frame |
| `linearVelocity` | `Vector3<T, BodyFrame>` | `BodyFrame` | Linear velocity resolved in aircraft/body axes $[u, v, w]$ |
| `angularVelocity` | `Vector3<T, BodyFrame>` | `BodyFrame` | Body angular rates $[p, q, r]$ |

### 2.2 Parameters and Forcing Functions
- **Mass & Inertia (`RigidBodyParameters<T, BodyFrame>`)**: Defined strictly in `BodyFrame`.
- **Applied Loads (`Wrench6<T, BodyFrame>`)**: Net external force $\mathbf{F}_B$ and moment $\mathbf{M}_B$ resolved in `BodyFrame`.
- **Computed Accelerations (`RigidBodyDynamicsKernel`)**:
  $$\mathbf{a}_B = \frac{1}{m} \left( \mathbf{F}_B - \boldsymbol{\omega}_B \times (m \mathbf{v}_B) \right) \quad (\text{resolved in BodyFrame})$$
  $$\boldsymbol{\alpha}_B \approx \mathbf{I}^{-1} \left( \mathbf{M}_B - \boldsymbol{\omega}_B \times (\mathbf{I} \boldsymbol{\omega}_B) \right) \quad (\text{resolved in BodyFrame})$$

### 2.3 Frame Violation in Existing Implementation
In `include/AegisMath/Dynamics/EulerIntegrator.h:37-39`:
```cpp
// 更新位置（基于当前机体速度简单推进）
state.position.x += state.linearVelocity.x * dt;
state.position.y += state.linearVelocity.y * dt;
state.position.z += state.linearVelocity.z * dt;
```
**Violation**:
- LHS: `state.position` $\in \text{RefFrame}$.
- RHS: `state.linearVelocity` $\in \text{BodyFrame}$.
- If the vehicle is heading East ($R_{B \to R}$ rotates $X_B$ to $Y_R$), a forward body velocity $u = 10\text{ m/s}$ will cause the vehicle to erroneously move North in Reference frame, because $u$ is directly added to `position.x`.
- **Mathematically Correct Update**:
  $$\mathbf{v}_{ref} = \text{state.attitude} * \text{state.linearVelocity}$$
  $$\mathbf{r}_{k+1} = \mathbf{r}_k + \mathbf{v}_{ref} \Delta t$$

---

## 3. Mathematical Discretization & FreeFall Analysis

### 3.1 Numerical Scheme Classification
In `EulerIntegrator.h`:
1. Velocity is updated first:
   $$\mathbf{v}_{k+1} = \mathbf{v}_k + \mathbf{a}_k \Delta t$$
2. Position is updated second using the **new** velocity $\mathbf{v}_{k+1}$:
   $$\mathbf{r}_{k+1} = \mathbf{r}_k + \mathbf{v}_{k+1} \Delta t$$

This is the **Semi-Implicit Euler** scheme (Euler-Cromer / Symplectic Euler), **not** Explicit Forward Euler (which would use $\mathbf{v}_k$).

### 3.2 Discrete vs. Continuous Derivation
Consider 1D vertical drop under constant gravity $a = g$, with $z_0 = 0$, $v_0 = 0$:

- **Continuous Analytical Physics**:
  $$v(t) = g t$$
  $$z_{\text{analytical}}(t) = \frac{1}{2} g t^2$$
  For $g = 9.80665\text{ m/s}^2, t = 1.0\text{ s}$:
  $$z_{\text{analytical}}(1.0) = \frac{1}{2} \times 9.80665 \times 1.0^2 = \mathbf{4.903325\text{ m}}$$

- **Semi-Implicit Euler Discrete Physics**:
  At step $k \in \{1, 2, \dots, N\}$:
  $$v_k = v_{k-1} + g \Delta t = k g \Delta t$$
  $$z_k = z_{k-1} + v_k \Delta t = z_{k-1} + k g (\Delta t)^2$$
  Summing from $k = 1$ to $N$:
  $$z_N = \sum_{k=1}^N k g (\Delta t)^2 = g (\Delta t)^2 \frac{N (N + 1)}{2}$$
  Since $N \Delta t = t$:
  $$z_N = \frac{1}{2} g t (N + 1) \Delta t = \frac{1}{2} g t (t + \Delta t) = \mathbf{\frac{1}{2} g t^2 + \frac{1}{2} g t \Delta t}$$

- **Exact Discretization Truncation Discrepancy**:
  $$E_{\text{semi-implicit}} = z_N - z_{\text{analytical}} = \mathbf{\frac{1}{2} g t \Delta t}$$
  For $g = 9.80665\text{ m/s}^2, t = 1.0\text{ s}, \Delta t = 0.01\text{ s}$:
  $$E = \frac{1}{2} \times 9.80665 \times 1.0 \times 0.01 = \mathbf{0.04903325\text{ m}}$$
  Computed position:
  $$z_{100} = 4.903325 + 0.04903325 = \mathbf{4.95235825\text{ m}}$$

- **Test Assertion in `FreeFallTest.cpp:48`**:
  ```cpp
  double expected_z = 0.5 * g * 1.0 * 1.0; // 4.903325
  EXPECT_NEAR(state.position.z, expected_z, 1e-2);
  ```
  The test demands $|z_{100} - 4.903325| \le 0.01$.
  However, the true discretization error is $0.04903325 > 0.01$.
  **GoogleTest Failure Output**:
  ```text
  The difference between state.position.z and expected_z is 0.049033249999995476,
  which exceeds 1e-2, where
  state.position.z evaluates to 4.9523582499999952,
  expected_z evaluates to 4.9033249999999997, and
  1e-2 evaluates to 0.01.
  ```
  The discrepancy matches the mathematical derivation to 15 decimal places.

---

## 4. Rotational Kinematics Contract Analysis

In rigid body kinematics, angular velocity $\boldsymbol{\omega}_B$ produces time evolution of attitude quaternion $\mathbf{q}_{B \to R}$:
$$\dot{\mathbf{q}}_{B \to R} = \frac{1}{2} \mathbf{q}_{B \to R} \otimes \boldsymbol{\omega}_B$$
or via exponential map / incremental quaternion:
$$\Delta \mathbf{q} \approx \left[ \cos\left(\frac{\|\boldsymbol{\omega}\| \Delta t}{2}\right), \frac{\boldsymbol{\omega}}{\|\boldsymbol{\omega}\|} \sin\left(\frac{\|\boldsymbol{\omega}\| \Delta t}{2}\right) \right]$$
$$\mathbf{q}_{k+1} = (\mathbf{q}_k \otimes \Delta \mathbf{q}).\text{normalized}()$$

In the current codebase:
- `EulerIntegrator::Step` completely ignores `state.attitude`.
- `state.attitude` remains unchanged regardless of torque or angular rate.
- Therefore, `EulerIntegrator` currently only functions as a corrupted translational point-mass propagator.

---

## 5. Formal Contract Inquiries & Recommendations

### 5.1 Is an Algorithm Change Required?
- **For `EulerIntegrator`**:
  No, the Semi-Implicit Euler method is a well-established, energy-conserving (symplectic) first-order scheme widely used in real-time simulation and game physics. It should **remain a first-order Euler integrator**, but must be corrected to:
  1. Transform linear velocity through attitude: $\mathbf{v}_{ref} = \mathbf{q} * \mathbf{v}_{body}$.
  2. Propagate quaternion attitude: $\mathbf{q}_{k+1} = \mathbf{q}_k + \frac{1}{2} (\mathbf{q}_k \otimes \boldsymbol{\omega}) \Delta t$ (canonicalized/normalized).
- **For Higher Accuracy (Roadmap Item)**:
  A 4th-order Runge-Kutta (`RK4Integrator`) should be introduced as a separate numerical engine for high-precision GNC trajectories where $O(\Delta t^4)$ global error is required.

### 5.2 Is a Test Change Required?
- **Yes**:
  In `FreeFallTest.cpp`, asserting that an $O(\Delta t)$ discrete Euler integrator with $\Delta t = 0.01$ matches a continuous continuous solution within $0.01$ is mathematically impossible. The test should:
  1. Either compare against the discrete analytical solution of the Euler scheme:
     $$\text{expected\_discrete\_z} = \frac{1}{2} g t^2 + \frac{1}{2} g t \Delta t$$
  2. Or set the tolerance commensurate with the method's truncation error:
     $$\text{tolerance} = \frac{1}{2} g t \Delta t + \epsilon_{\text{roundoff}} \approx 0.05\text{ m}$$
  3. Or reduce step size $\Delta t = 0.001$ ($N = 1000$ steps), where error is $0.0049\text{ m} < 0.01\text{ m}$.

### 5.3 Is an API Redesign Required?
- **No**:
  The existing API signature:
  ```cpp
  template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
  static constexpr void Step(
      KinematicState<T, RefFrame, BodyFrame>& state,
      const RigidBodyParameters<T, BodyFrame>& params,
      const Wrench6<T, BodyFrame>& wrench,
      T dt
  ) noexcept
  ```
  is well-designed, type-safe, frame-tagged, and zero-allocation. The corrections are internal to the function implementation.

---

## 6. Proposed Remediation Implementation (For Future Phase)

```cpp
// Corrected EulerIntegrator::Step logic:
// 1. Compute translational and rotational accelerations in BodyFrame
RigidBodyDynamicsKernel<T, RefFrame, BodyFrame>::ComputeDerivative(
    state, params, wrench, lin_accel, ang_accel
);

// 2. Update velocities (Semi-Implicit Euler order)
state.linearVelocity.x += lin_accel.x * dt;
state.linearVelocity.y += lin_accel.y * dt;
state.linearVelocity.z += lin_accel.z * dt;

state.angularVelocity.x += ang_accel.x * dt;
state.angularVelocity.y += ang_accel.y * dt;
state.angularVelocity.z += ang_accel.z * dt;

// 3. Transform body velocity to reference frame
auto vel_ref = state.attitude * state.linearVelocity;

// 4. Update position in reference frame
state.position.x += vel_ref.x * dt;
state.position.y += vel_ref.y * dt;
state.position.z += vel_ref.z * dt;

// 5. Update attitude quaternion kinematics
// dq/dt = 0.5 * q ⊗ omega
// q_{k+1} = normalize(q_k + dq/dt * dt)
```
