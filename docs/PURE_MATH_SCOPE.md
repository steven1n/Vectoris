# AegisMathLib Pure Mathematics & Numerical Scope Specification

> [!IMPORTANT]
> **Document**: Pure Mathematics Scope Specification  
> **Document Version**: 1.0 (P3 Baseline)  
> **Authority**: Subordinate specification governed by [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)  
> **Effective Baseline**: P3 Architectural Boundary Refactor  

---

## 1. Mission Statement

**AegisMathLib** is a standalone, deterministic, header-only ISO C++20 mathematics and numerical computation library.

Its exclusive purpose is to provide:
- Strict, zero-overhead dimensional algebra and physical unit representation
- High-integrity scalar floating-point primitives and IEEE-754 traits
- Coordinate frame safety and affine spatial geometry
- Standard-layout linear algebra vectors, matrices, and decompositions
- Deterministic, bounded numerical kernels and solvers
- Monadic error handling without runtime exceptions

AegisMathLib is **NOT** a physics simulation engine, flight mechanics simulator, navigation framework, tracking system, or control library.

---

## 2. Core Boundary Principle: The Pure-Math Test

An abstraction belongs in **AegisMathLib** if and only if it satisfies the following two tests:

1. **The System-Agnostic Test**:
   > *If a type or algorithm requires knowing what a physical vehicle, rigid body, sensor, actuator, or simulation scenario is, it does NOT belong in AegisMathLib.*

2. **The Generic Mathematical Object Test**:
   > *If an algorithm can be described entirely using pure mathematical abstractions (scalars, fields, vector spaces, groups, algebras, matrices, manifolds, generic ODEs) and generic state, it belongs in AegisMathLib.*

---

## 3. Allowed Abstraction Categories

| Category | Allowed Concepts & Primitives | Examples in AegisMathLib |
| :--- | :--- | :--- |
| **Error & Type Infrastructure** | Monadic results, mathematical errors, type constraints | `Result<T, MathError>`, `Numeric`, `FloatingPoint` |
| **Scalar Numerics** | Bounded elementary functions, traits, dual tolerances | `Core::Math::sqrt`, `AlmostEqual`, `NumericTraits` |
| **Dimensional Algebra** | Base dimensions, derived dimensions, strong quantities | `Dimension`, `Quantity<T, Unit>`, `UnitCast` |
| **SI Physical Units** | Standard SI units, prefixes, dimensional products | `Meter`, `Second`, `Radian`, `Newton`, `TorqueUnit` |
| **Spatial Geometry** | Euclidean vectors, affine points, frame tags | `Vector3<T, Frame>`, `Point3<T, Frame>`, `FrameTag` |
| **Rotational Lie Groups** | Direction cosine matrices, Hamilton unit quaternions | `RotationMatrix3<T, F1, F2>`, `Quaternion<T, F1, F2>` |
| **Rigid Transforms** | Homogeneous spatial transformations $SE(3)$ | `Transform3<T, From, To>` |
| **Linear Solvers** | Direct solvers, decompositions, SPD systems | `SymmetricLinearSolver3` ($LDL^T$) |
| **Future Numerical** | Generic ODE steppers, root-finding, quadrature | (Reserved for future numerical layers) |

---

## 4. Forbidden Domain Categories

The following domain abstractions are **strictly prohibited** from AegisMathLib and must reside in downstream domain modules (e.g., `AegisDynamics`, `AegisEstimation`, `AegisControl`):

| Forbidden Category | Forbidden Abstractions | Proper Architectural Destination |
| :--- | :--- | :--- |
| **Rigid-Body Mechanics** | `RigidBodyState`, `RigidBodyParameters`, mass properties | `AegisDynamics` |
| **Spatial Physics Vectors** | `Wrench6` (force + moment), `Twist6` (linear + angular) | `AegisDynamics` |
| **Inertia Tensors** | Physical mass moment of inertia matrices (`InertiaTensor3`) | `AegisDynamics` |
| **Domain State Propagators** | Rigid-body kinematic/dynamic Euler/RK4 propagators | `AegisDynamics` |
| **Vehicles & Scenarios** | Aircraft, missiles, satellites, radar, weapon systems | Downstream Simulation Systems |
| **Target Tracking** | Kalman filters, EKF, UKF, IMM, track management | `AegisEstimation` |
| **Control Algorithms** | PID, LQR, guidance laws, actuator allocators | `AegisControl` |
| **Signal Processing** | FFT, spectral estimators, filter banks | `AegisSignal` |

---

## 5. Architectural Hierarchy & Dependency Rules

AegisMathLib enforces a strict one-way downward dependency model:

```text
┌────────────────────────────────────────┐
│  Downstream Domain Simulation Modules  │
│  (AegisDynamics, AegisEstimation, etc) │
└───────────────────┬────────────────────┘
                    │ (Depends upon)
                    ▼
┌────────────────────────────────────────┐
│              AegisMathLib              │
│  ┌──────────────┐     ┌─────────────┐  │
│  │    Units     │     │  Geometry   │  │
│  │ (Model B 8D) │     │ (SO3 / SE3) │  │
│  └──────┬───────┘     └──────┬──────┘  │
│         │                    │         │
│         └──────────┬─────────┘         │
│                    ▼                   │
│             Core (Layer 0)             │
└────────────────────────────────────────┘
```

### Dependency Rules:
1. **Zero Upward Coupling**: `AegisMathLib` MUST NEVER include, link against, or reference `AegisDynamics` or any other downstream domain module.
2. **Subsystem Peer Independence**: Within `AegisMathLib`, `Geometry` and `Units` are independent peer modules above `Core`. Neither includes the other.
3. **Automated Enforcement**: Continuous architecture regression tests (`tests/Architecture/DependencyLayerTest.cpp`) verify on every build that no `#include` references domain physics.

---

## 6. Borderline Decision Framework

When evaluating a borderline abstraction, follow this decision tree:

```text
Does the abstraction represent a universal mathematical entity?
├── YES ──> Does it require domain vehicle/physical state concepts?
│            ├── YES ──> Move to Domain Library (e.g. AegisDynamics)
│            └── NO  ──> Does it couple Geometry with Units?
│                         ├── YES ──> Move to Domain Library
│                         └── NO  ──> Keep in AegisMathLib
└── NO  ──> Move to Domain Library
```

### Case Studies Evaluated in P3:
- **`QuantityVector3`**: Combines 3D geometry vectors with physical units and defines physical aliases (`Force3`, `Torque3`, `Velocity3`) and rotational cross operators. **Result: Extracted to `AegisDynamics`.**
- **`MomentOfInertia` (Scalar Unit)**: Defines the dimensional exponent vector $[M \cdot L^2 \cdot A^{-2}]$ under Model B. **Result: Retained in `Units` as dimensional algebra.**
- **`InertiaTensor3`**: A spatial matrix of physical inertia quantities with rigid-body positive-definiteness validation. **Result: Extracted to `AegisDynamics`.**
- **`SymmetricLinearSolver3`**: Solves $A \mathbf{x} = \mathbf{b}$ for symmetric positive-definite matrices using analytic $LDL^T$ decomposition. **Result: Retained in `Geometry` as pure linear algebra.**
- **`EulerIntegrator`**: Integrates rigid-body Newton-Euler equations with attitude quaternion rate kinematics. **Result: Extracted to `AegisDynamics`.**

---

## 7. Non-Goals

The P3 pure-math scope refactor explicitly excludes the following non-goals:
- Implementing missing mathematical algorithms (e.g., RK4, generic ODE solvers, optimization, FFT)
- Modifying working mathematical kernels (`Vector3`, `Matrix3`, `Quaternion`, `Result`)
- Introducing runtime heap allocations or exceptions
- Broadening beyond ISO C++20 strict mode
