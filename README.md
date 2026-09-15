# AegisMathLib

AegisMathLib is a header-only, deterministic, strongly typed mathematical foundation library implemented in ISO C++20 for aerospace simulation, robotics, navigation, and high-reliability computing.

The library architecture is inspired by and aligned with principles from safety-critical software engineering (e.g. MISRA C++ and DO-178C high-integrity design paradigms), prioritizing compile-time safety, zero-overhead abstraction, standard-layout ABI stability, and verifiable numerical contracts.

---

## Current Status

| Metric | Status |
| :--- | :--- |
| **Language Baseline** | ISO C++20 (`-std=c++20`, strict mode) |
| **Compiler Warnings** | **0 warnings** (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`) |
| **Open CRITICAL Findings** | **0** |
| **Open HIGH Findings** | **0** |
| **Code-Level MEDIUM Blockers** | **0** |
| **Public Headers Standalone Isolation** | **PASS** (66 / 66 headers, 68 / 68 TUs in `AegisMathLib_HeaderIsolation`) |
| **Test Suite Execution** | **108 / 108 PASS (100%)** in Debug and Release |
| **Stable-Core Qualification** | **IN PROGRESS** (5 qualification gates remaining: ASan, UBSan, Coverage, Cross-Compiler, Static Analysis) |
| **Stable-Core Certification** | **NOT CERTIFIED** (Pending resolution of qualification gates) |

---

## Architecture & Subsystems

AegisMathLib enforces a strict **one-way dependency hierarchy** across four primary modules:

```text
Core (Layer 0) ──> Units (Layer 1) ──> Geometry (Layer 2) ──> Dynamics (Layer 3)
```

Lower layers NEVER depend on higher layers. Cyclic dependencies and umbrella header leakages are strictly prevented by automated architecture guards and standalone translation unit compilation.

### 1. [`Core`](docs/core.md) (Layer 0)
- Default scalar: IEEE-754 `double` (`float` supported, integer/long double restricted).
- Monadic error handling: `Result<T, MathError>` backed by standard `std::variant`. Zero exceptions in mathematical kernels.
- Bounded elementary functions: `Core::Math::sqrt` with bounded Newton-Raphson iteration (hard cap 64 iterations) and scale-aware convergence.
- IEEE-754 traits: `AlmostEqual` with dual absolute and relative tolerances.

### 2. [`Units`](docs/units.md) (Layer 1)
- **Model B 8-Dimensional Physical System**: Treats Plane Angle ($A$) as an independent base dimension alongside Length, Mass, Time, Current, Temperature, Amount, and Luminosity.
- Strongly typed `Quantity<T, Unit>` introducing no per-object storage overhead beyond the underlying scalar (`sizeof(Quantity) == sizeof(Scalar)`).
- 8 Base Units (`Meter`, `Second`, `Kilogram`, `Radian`, `Kelvin`, `Ampere`, `Mole`, `Candela`).
- 10 Derived Units (`Velocity`, `Acceleration`, `Force`, `Frequency`, `AngularVelocity`, `AngularAcceleration`, `Torque`, `MomentOfInertia`, `Power`, `AngularMomentum`).
- Compensating inverse-angle exponents ensuring strict dimensional separation between Torque ($[M L^2 T^{-2} A^{-1}]$) and Energy ($[M L^2 T^{-2} A^0]$).

### 3. [`Geometry`](docs/geometry.md) (Layer 2)
- Compile-time coordinate frame safety: `Vector3<T, Frame>`, `Point3<T, Frame>`, `UnitVector3<T, Frame>`.
- Frame-agnostic generic linear algebra: `Matrix3<T>` intentionally carries no frame tags, serving general solvers, Jacobians, and covariance matrices.
- Frame-tagged spatial transformations: `Quaternion<T, From, To>`, `RotationMatrix3<T, From, To>`, `Transform3<T, From, To>`.
- Left-to-right frame pipeline composition: $R_{AB} * R_{BC} \to R_{AC}$ (underlying Direction Cosine Matrix: $M_{AC} = M_{BC} M_{AB}$).
- Three-tier comparison model: exact component-wise `operator==`, tolerance-aware `AlmostEqual`, and sign-invariant $SO(3)$ double-cover `RotationEquivalent`.
- Fixed $3 \times 3$ analytic $LDL^T$ SPD linear solver (`SymmetricLinearSolver3`) enforcing the **Solve-Not-Invert** policy.

### 4. [`Dynamics`](docs/dynamics.md) (Layer 3)
- Strongly typed spatial quantity vectors: `QuantityVector3` (`Velocity3`, `Force3`, `Torque3`, `AngularVelocity3`, `AngularAcceleration3`).
- 6-DOF spatial vectors: `Wrench6` (force + moment) and `Twist6` (linear + angular velocity).
- Rigid-body parameters: `RigidBodyParameters` and `RigidBodyState`.
- `InertiaTensor3`: 3-tier validity check (finite, symmetric, and strictly positive-definite via $LDL^T$ pivots). Indefinite matrices with positive diagonals are rejected.
- Full rigid-body inertia coupling solved via $I \boldsymbol{\alpha} = \boldsymbol{\tau}_{\text{net}}$ without diagonal-only simplifications.
- Rotational Lie bracket `LieBracket(omega, L)` explicitly normalizing by $1/\text{rad}$ to preserve dimensional consistency.
- `EulerIntegrator`: 1st-order semi-implicit state propagation with strict input validation ($dt > 0$) and full transactional safety (zero partial state commitment on numerical failure).

---

## Build & Test Procedure

### Prerequisites
- CMake $\ge 3.14$
- C++20 compliant compiler:
  - Locally verified: AppleClang 21.0.0 (macOS x86_64/arm64)
  - Intended cross-compiler matrix (gate `NOT RUN`): GCC $\ge 13$, LLVM Clang $\ge 16$, MSVC $\ge 2022$

### Build and Run Tests
```bash
# Configure Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Compile all targets in parallel
cmake --build build --parallel

# Execute unit and regression test suite (108 tests)
./build/AegisMathLib_Tests
```

### Run Public Header Standalone Isolation Gate
```bash
# Compile all 66 standalone header TUs and 2 include-order poisoning TUs
cmake --build build --target AegisMathLib_HeaderIsolation --parallel
```

---

## Documentation Authority & Inventory

### Authority Model & Scoped Precedence
1. **Normative Baseline (SSOT)**: [`docs/ENGINEERING_STANDARD_V1.md`](docs/ENGINEERING_STANDARD_V1.md) is the single source of truth for all normative mathematical, architectural, and engineering rules.
2. **Authorized Scoped Exceptions**: [`docs/DEVIATIONS.md`](docs/DEVIATIONS.md) is the Engineering Standard's formally authorized exception mechanism per Sections 101 & 102. Within an explicitly registered deviation scope, the registered deviation modifies only the cited rule for the declared component and lifetime; outside that scope, the Standard remains fully authoritative.
3. **Current Specifications & Overviews**: Subordinate specifications ([`docs/MATHEMATICAL_CONVENTIONS.md`](docs/MATHEMATICAL_CONVENTIONS.md), [`docs/core.md`](docs/core.md), [`docs/units.md`](docs/units.md), [`docs/geometry.md`](docs/geometry.md), [`docs/dynamics.md`](docs/dynamics.md)) and this overview ([`README.md`](README.md)) must strictly conform to the **Engineering Standard plus applicable registered deviations**.
4. **Audit & Evidence**: Audit documents ([`docs/audits/`](docs/audits/)) record historical findings and live qualification evidence; they carry zero normative authority to modify requirements.
5. **Deprecated / Historical**: Legacy notes ([`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), [`docs/CODING_STANDARD.md`](docs/CODING_STANDARD.md), [`CHANGELOG.md`](CHANGELOG.md), [`TECHDEBT.md`](TECHDEBT.md), [`TODO.md`](TODO.md)) are superseded and non-authoritative.

### Complete Documentation Inventory

| Authority Classification | File | Description / Scope |
| :--- | :--- | :--- |
| **Entry Point** | [`README.md`](README.md) | Repository overview, subsystem architecture, build commands, and documentation index. |
| **Normative (SSOT)** | [`docs/ENGINEERING_STANDARD_V1.md`](docs/ENGINEERING_STANDARD_V1.md) | Authoritative Single Source of Truth for engineering rules and coding standards. |
| **Governance** | [`docs/DEVIATIONS.md`](docs/DEVIATIONS.md) | Formal deviation ledger tracking intentional, scoped exceptions to normative rules. |
| **Current Specification** | [`docs/MATHEMATICAL_CONVENTIONS.md`](docs/MATHEMATICAL_CONVENTIONS.md) | Cross-module coordinate systems, rotation directions, and transformation pipeline conventions. |
| **Current Specification** | [`docs/core.md`](docs/core.md) | Core layer specification: scalar types, bounded numerical functions, IEEE-754 traits, error model. |
| **Current Specification** | [`docs/units.md`](docs/units.md) | Units layer specification: Model B 8D dimensional algebra, unit tags, quantity ABI validation. |
| **Current Specification** | [`docs/geometry.md`](docs/geometry.md) | Geometry layer specification: frame safety, matrix linear algebra, quaternion conventions, SPD solver. |
| **Current Specification** | [`docs/dynamics.md`](docs/dynamics.md) | Dynamics layer specification: spatial quantities, rigid-body state, inertia tensor, Euler integrator. |
| **Audit / Evidence** | [`docs/audits/AegisMathLib_Compliance_Audit_v1.md`](docs/audits/AegisMathLib_Compliance_Audit_v1.md) | Comprehensive compliance audit ledger across all CRITICAL, HIGH, MEDIUM, and LOW findings. |
| **Audit / Evidence** | [`docs/audits/AegisMathLib_Stable_Core_Qualification_v1.md`](docs/audits/AegisMathLib_Stable_Core_Qualification_v1.md) | Quality qualification matrix, blocker status, header isolation, and DoD evidence. |
| **Audit / Evidence** | [`docs/audits/EulerIntegrator_Contract_Review.md`](docs/audits/EulerIntegrator_Contract_Review.md) | Mathematical contract review of Euler numerical integrator and coordinate frame mixing. |
| **Audit / Evidence** | [`docs/audits/P0_Remediation_Closure.md`](docs/audits/P0_Remediation_Closure.md) | Historical verification report for P0 remediation closure (AML-CRIT-001 through 003). |
| **Deprecated / Historical** | [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Legacy pre-V1 architecture specification (superseded by Engineering Standard V1). |
| **Deprecated / Historical** | [`docs/CODING_STANDARD.md`](docs/CODING_STANDARD.md) | Legacy pre-V1 coding standard (superseded by Engineering Standard V1). |
| **Deprecated / Historical** | [`CHANGELOG.md`](CHANGELOG.md) | Legacy release notes from initial geometry development. |
| **Deprecated / Historical** | [`TECHDEBT.md`](TECHDEBT.md) | Legacy technical debt tracking list (superseded by compliance audits). |
| **Deprecated / Historical** | [`TODO.md`](TODO.md) | Legacy milestone tracking notes (superseded by stable-core qualification). |

---

## License

No license file is currently committed to this repository. All rights reserved pending formal licensing.
