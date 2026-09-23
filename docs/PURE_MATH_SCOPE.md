# Vectoris Pure Mathematics & Numerical Scope Specification

> [!IMPORTANT]
> **Document**: Pure Mathematics Scope Specification  
> **Document Version**: 1.1 (Vectoris Baseline)  
> **Authority**: Subordinate specification governed by [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)  
> **Effective Baseline**: Vectoris Global Architecture Baseline  

---

## 1. Mission Statement

**VectorisNumerics** is a standalone, deterministic, header-only ISO C++20 mathematics and numerical computation library within the Vectoris framework.

Its exclusive purpose is to provide:
- Strict, zero-overhead dimensional algebra and physical unit representation (`vectoris::numerics::units`)
- High-integrity scalar floating-point primitives and IEEE-754 traits (`vectoris::numerics::core`)
- Coordinate frame safety and affine spatial geometry (`vectoris::numerics::geometry`)
- Standard-layout linear algebra vectors, matrices, and decompositions
- Deterministic, bounded numerical kernels and solvers
- Monadic error handling without runtime exceptions

`VectorisNumerics` is **NOT** a physics simulation engine, flight mechanics simulator, navigation framework, tracking system, or control library.

---

## 2. Core Boundary Principle: The Pure-Math Test

An abstraction belongs in **VectorisNumerics** if and only if it satisfies the following two tests:

1. **The System-Agnostic Test**:
   > *If a type or algorithm requires knowing what a physical vehicle, rigid body, sensor, actuator, or simulation scenario is, it does NOT belong in VectorisNumerics.*

2. **The Generic Mathematical Object Test**:
   > *If an algorithm can be described entirely using pure mathematical abstractions (scalars, fields, vector spaces, groups, algebras, matrices, manifolds, generic ODEs) and generic state, it belongs in VectorisNumerics.*

---

## 3. Allowed Abstraction Categories

| Category | Allowed Concepts & Primitives | Examples in VectorisNumerics |
| :--- | :--- | :--- |
| **Error & Type Infrastructure** | Monadic results, mathematical errors, type constraints | `Result<T, MathError>`, `Numeric`, `FloatingPoint` |
| **Scalar Numerics** | Bounded elementary functions, traits, dual tolerances | `core::sqrt` (`core::Math::sqrt` compatibility wrapper), `AlmostEqual`, `NumericTraits` |
| **Dimensional Algebra** | Base dimensions, derived dimensions, strong quantities | `Dimension`, `Quantity<T, Unit>`, `UnitCast` |
| **SI Physical Units** | Standard SI units, prefixes, dimensional products | `Meter`, `Second`, `Radian`, `Newton`, `TorqueUnit` |
| **Spatial Geometry** | Euclidean vectors, affine points, frame tags | `Vector3<T, Frame>`, `Point3<T, Frame>`, `FrameTag` |
| **Rotational Lie Groups** | Direction cosine matrices, Hamilton unit quaternions | `RotationMatrix3<T, F1, F2>`, `Quaternion<T, F1, F2>` |
| **Rigid Transforms** | Homogeneous spatial transformations $SE(3)$ | `Transform3<T, From, To>` |
| **Linear Solvers** | Direct solvers, decompositions, SPD systems | `SymmetricLinearSolver3` ($LDL^T$) |
| **Future Numerical** | Generic ODE steppers, root-finding, quadrature | (Reserved for future numerical layers) |

---

## 4. Forbidden Domain Categories

The following domain abstractions are **strictly prohibited** from `VectorisNumerics` and must reside in downstream domain modules (e.g., `VectorisDynamics`, `VectorisEstimation`, `VectorisControl`):

| Forbidden Category | Forbidden Abstractions | Proper Architectural Destination |
| :--- | :--- | :--- |
| **Rigid-Body Mechanics** | `RigidBodyState`, `RigidBodyParameters`, mass properties | `VectorisDynamics` |
| **Spatial Physics Vectors** | `Wrench6` (force + moment), `Twist6` (linear + angular) | `VectorisDynamics` |
| **Inertia Tensors** | Physical mass moment of inertia matrices (`InertiaTensor3`) | `VectorisDynamics` |
| **Domain State Propagators** | Rigid-body kinematic/dynamic Euler/RK4 propagators | `VectorisDynamics` |
| **Vehicles & Scenarios** | Aircraft, missiles, satellites, radar, weapon systems | Downstream Simulation Systems |
| **Target Tracking** | Kalman filters, EKF, UKF, IMM, track management | `VectorisEstimation` |
| **Control Algorithms** | PID, LQR, guidance laws, actuator allocators | `VectorisControl` |
| **Signal Processing** | FFT, spectral estimators, filter banks | `VectorisSignal` |

---

## 5. Architectural Hierarchy & Dependency Rules

Vectoris enforces a strict one-way downward dependency model:

```text
┌────────────────────────────────────────┐
│  Downstream Domain Simulation Modules  │
│  (VectorisDynamics, Estimation, etc.)  │
└───────────────────┬────────────────────┘
                    │ (Depends upon)
                    ▼
┌────────────────────────────────────────┐
│            VectorisNumerics            │
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
1. **Zero Upward Coupling**: `VectorisNumerics` MUST NEVER include, link against, or reference `VectorisDynamics` or any other downstream domain module.
2. **Subsystem Peer Independence**: Within `VectorisNumerics`, `Geometry` and `Units` are independent peer modules above `Core`. Neither includes the other.
3. **Automated Enforcement**: Continuous architecture regression tests (`tests/Architecture/DependencyLayerTest.cpp`) verify on every build that no `#include` references domain physics.

---

## 6. Borderline Decision Framework

When evaluating a borderline abstraction, follow this decision tree:

```text
Does the abstraction represent a universal mathematical entity?
├── YES ──> Does it require domain vehicle/physical state concepts?
│            ├── YES ──> Move to Domain Library (e.g. VectorisDynamics)
│            └── NO  ──> Does it couple Geometry with Units?
│                         ├── YES ──> Move to Domain Library
│                         └── NO  ──> Keep in VectorisNumerics
└── NO  ──> Move to Domain Library
```

### Case Studies:
- **`QuantityVector3`**: Combines 3D geometry vectors with physical units and defines physical aliases (`Force3`, `Torque3`, `Velocity3`) and rotational cross operators. **Result: Extracted to `VectorisDynamics`.**
- **`MomentOfInertia` (Scalar Unit)**: Defines the dimensional exponent vector $[M \cdot L^2 \cdot A^{-2}]$ under Model B. **Result: Retained in `Units` as dimensional algebra.**
- **`InertiaTensor3`**: A spatial matrix of physical inertia quantities with rigid-body positive-definiteness validation. **Result: Extracted to `VectorisDynamics`.**
- **`SymmetricLinearSolver3`**: Solves $A \mathbf{x} = \mathbf{b}$ for symmetric positive-definite matrices using analytic $LDL^T$ decomposition. **Result: Retained in `Geometry` as pure linear algebra.**
- **`EulerIntegrator`**: Integrates rigid-body Newton-Euler equations with attitude quaternion rate kinematics. **Result: Extracted to `VectorisDynamics`.**

---

## 7. Non-Goals

The pure-math scope definition explicitly excludes the following non-goals:
- Implementing missing mathematical algorithms (e.g., RK4, generic ODE solvers, optimization, FFT)
- Modifying working mathematical kernels (`Vector3`, `Matrix3`, `Quaternion`, `Result`)
