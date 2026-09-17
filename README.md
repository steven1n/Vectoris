# Vectoris

Vectoris is a modular, deterministic, header-only ISO C++20 numerical computing and engineering simulation framework for high-reliability applications, scientific computation, and spatial dynamics.

The framework prioritizes compile-time dimensional safety, coordinate frame safety, zero-overhead abstraction, standard-layout ABI stability, and verifiable numerical contracts without runtime exceptions or dynamic heap allocations.

> [!NOTE]
> **Modular Architecture**: Vectoris enforces a strict separation between pure mathematics ([`VectorisNumerics`](modules/VectorisNumerics/)) and domain-specific physical mechanics ([`VectorisDynamics`](modules/VectorisDynamics/)). See [`docs/PURE_MATH_SCOPE.md`](docs/PURE_MATH_SCOPE.md) and [`docs/VECTORIS_RENAME_MIGRATION.md`](docs/VECTORIS_RENAME_MIGRATION.md).

---

## Repository Structure

```text
Vectoris/
├── CMakeLists.txt          # Project-level orchestrator (C++20 baseline, deps, module dispatch)
├── CMakePresets.json        # Standardized build presets (all output under .build/)
├── README.md
├── VERSION
│
├── modules/
│   ├── VectorisNumerics/   # Pure mathematics & numerical computation library
│   │   ├── CMakeLists.txt  # Module targets, tests, isolation, coverage, static analysis
│   │   ├── include/
│   │   │   └── Vectoris/
│   │   │       └── Numerics/
│   │   │           ├── Core/       # IEEE-754 traits, bounded functions, error model
│   │   │           ├── Units/      # Model B 8D dimensional analysis system
│   │   │           └── Geometry/   # Frame-safe SO(3)/SE(3) spatial geometry & linear algebra
│   │   └── tests/
│   │       ├── Architecture/   # Dependency layer violation detection
│   │       ├── Core/
│   │       ├── Units/
│   │       └── Geometry/
│   │
│   └── VectorisDynamics/   # Downstream domain physics module (rigid-body mechanics)
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── Vectoris/
│       │       └── Dynamics/       # Rigid-body state, inertia tensors, Euler integrator
│       └── tests/
│
├── cmake/                   # Reusable CMake infrastructure
│   ├── Coverage.cmake
│   ├── Sanitizers.cmake
│   ├── StaticAnalysis.cmake
│   └── PublicHeaderIsolation.cmake
│
├── tools/                   # Qualification and analysis tooling
│   ├── coverage/
│   └── static_analysis/
│
├── docs/                    # Specifications, audits, and conventions
│   ├── ENGINEERING_STANDARD_V1.md    # Normative SSOT
│   ├── VECTORIS_RENAME_MIGRATION.md  # Architectural migration guide
│   ├── audits/
│   └── ...
│
├── .github/workflows/       # Cross-compiler CI (GCC, Clang, MSVC)
└── .build/                  # Generated build output (gitignored)
```

**Key layout principles:**
- `modules/VectorisNumerics/` = pure mathematics library (55 headers, 12 test files)
- `modules/VectorisDynamics/` = downstream rigid-body / physics module (11 headers, 11 test files, depends on `VectorisNumerics`)
- `.build/` = all local generated build output (never tracked by Git)
- `cmake/` = reusable CMake infrastructure (sanitizers, coverage, isolation, static analysis)

---

## Current Status

| Metric | Status |
| :--- | :--- |
| **Language Baseline** | ISO C++20 (`-std=c++20`, strict mode) |
| **Compiler Warnings** | **0 warnings** (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`) |
| **Open CRITICAL Findings** | **0** |
| **Open HIGH Findings** | **0** |
| **Code-Level MEDIUM Blockers** | **0** |
| **Public Headers Standalone Isolation** | **PASS** (55 / 55 headers in `VectorisNumerics_HeaderIsolation`, 11 / 11 in `VectorisDynamics_HeaderIsolation`) |
| **Numerics Test Suite Execution** | **PASS** in Debug and Release (106 / 106 tests) |
| **Dynamics Test Suite Execution** | **PASS** in Debug and Release (30 / 30 tests) |
| **Total Test Suite Execution** | **PASS** (136 / 136 tests) |
| **VectorisNumerics Qualification** | **Internal Project Qualification: PASS (R1 Remediated & Verified)** |
| **VectorisDynamics Migration** | **Migration Integrity: PASS** |

> [!NOTE]
> **Internal Qualification Baseline**: Current project status:
>
> ```text
> Vectoris
> VectorisNumerics
> Internal Project Qualification: PASS (R1 Remediated & Verified)
> VectorisDynamics
> Migration Integrity: PASS
> ```
>
> This is an internal engineering qualification only.
> It is not DO-178C certification,
> ISO 26262 certification,
> MISRA certification,
> FAA/EASA approval,
> or flight-software certification.

---

## Architecture & Subsystems

Vectoris enforces a strict **one-way downward dependency hierarchy** across its layers:

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

Lower layers NEVER depend on higher layers. Cyclic dependencies, domain bleed, and umbrella header leakages are strictly prevented by automated architecture guards and standalone translation unit compilation.

### 1. [`Core`](docs/core.md) (Layer 0)
- Canonical namespace: `vectoris::numerics::core`
- Default scalar: IEEE-754 `double` (`float` supported, integer/long double restricted).
- Monadic error handling: `Result<T, MathError>` backed by standard `std::variant`. Zero exceptions in mathematical kernels.
- Bounded elementary functions: `core::Math::sqrt` with bounded Newton-Raphson iteration (hard cap 64 iterations) and scale-aware convergence.
- IEEE-754 traits: `AlmostEqual` with dual absolute and relative tolerances.

### 2. [`Units`](docs/units.md) (Layer 1)
- Canonical namespace: `vectoris::numerics::units`
- **Model B 8-Dimensional Physical System**: Treats Plane Angle ($A$) as an independent base dimension alongside Length, Mass, Time, Current, Temperature, Amount, and Luminosity.
- Strongly typed `Quantity<T, Unit>` introducing no per-object storage overhead beyond the underlying scalar (`sizeof(Quantity) == sizeof(Scalar)`).
- 8 Base Units (`Meter`, `Second`, `Kilogram`, `Radian`, `Kelvin`, `Ampere`, `Mole`, `Candela`).
- 10 Derived Units (`Velocity`, `Acceleration`, `Force`, `Frequency`, `AngularVelocity`, `AngularAcceleration`, `Torque`, `MomentOfInertia`, `Power`, `AngularMomentum`).
- Compensating inverse-angle exponents ensuring strict dimensional separation between Torque ($[M L^2 T^{-2} A^{-1}]$) and Energy ($[M L^2 T^{-2} A^0]$).

### 3. [`Geometry`](docs/geometry.md) (Layer 2)
- Canonical namespace: `vectoris::numerics::geometry`
- Compile-time coordinate frame safety: `Vector3<T, Frame>`, `Point3<T, Frame>`, `UnitVector3<T, Frame>`.
- Frame-agnostic generic linear algebra: `Matrix3<T>` intentionally carries no frame tags, serving general solvers, Jacobians, and covariance matrices.
- Frame-tagged spatial transformations: `Quaternion<T, From, To>`, `RotationMatrix3<T, From, To>`, `Transform3<T, From, To>`.
- Left-to-right frame pipeline composition: $R_{AB} * R_{BC} 	o R_{AC}$ (underlying Direction Cosine Matrix: $M_{AC} = M_{BC} M_{AB}$).
- Three-tier comparison model: exact component-wise `operator==`, tolerance-aware `AlmostEqual`, and sign-invariant $SO(3)$ double-cover `RotationEquivalent`.
- Fixed $3 	imes 3$ analytic $LDL^T$ SPD linear solver (`SymmetricLinearSolver3`) enforcing the **Solve-Not-Invert** policy.

### 4. Downstream Module: [`VectorisDynamics`](modules/VectorisDynamics/) (Domain Physics)
- Canonical namespace: `vectoris::dynamics`
- Rigid-body mechanics, spatial physical quantity vectors (`QuantityVector3`), 6-DOF wrench and twist vectors (`Wrench6`, `Twist6`), rigid-body mass parameters, $3 	imes 3$ inertia tensor (`InertiaTensor3`), and 1st-order semi-implicit Euler state propagation (`EulerIntegrator`).
- Resides in `modules/VectorisDynamics/` and depends strictly downward upon `VectorisNumerics`. Zero domain physics resides in `VectorisNumerics`.

---

## Build & Test Procedure

### Prerequisites
- CMake $\ge 3.14$
- ISO C++20 conforming compiler:
  - Locally verified: AppleClang 21.0.0 (macOS x86_64/arm64)
  - Formally qualified cross-compiler matrix (gate `PASS`):
    - GNU GCC 13.3.0 (Ubuntu 24.04.1 x86_64)
    - Upstream LLVM Clang 18.1.3 (Ubuntu 24.04.1 x86_64)
    - MSVC 19.51.36256.0 / Visual Studio 18 2026 (Windows Server 2025 x64)

### Build and Run Tests (Using Presets)
```bash
# Configure and build Debug
cmake --preset debug
cmake --build --preset debug

# Run all tests
ctest --preset debug
```

### Available Presets
| Preset | Build Type | Description |
|:---|:---|:---|
| `debug` | Debug | Standard debug build with all modules (`VectorisNumerics` + `VectorisDynamics`) |
| `release` | Release | Optimized release build |
| `pure-numerics` | Debug | Standalone `VectorisNumerics` only (`VECTORIS_BUILD_DYNAMICS=OFF`) |
| `pure-math` | Debug | Legacy alias for `pure-numerics` |
| `asan` | Debug | AddressSanitizer enabled |
| `ubsan` | Debug | UndefinedBehaviorSanitizer enabled |
| `asan-ubsan` | Debug | Combined ASan + UBSan |
| `coverage` | Debug | LLVM source-based coverage instrumentation |
| `static-analysis` | Debug | Clang-Tidy static analysis targets |

All build output is placed under `.build/<preset>/`.

### Run Public Header Standalone Isolation Gates
```bash
# Compile standalone header translation units
cmake --build .build/debug --target VectorisNumerics_HeaderIsolation VectorisDynamics_HeaderIsolation --parallel
```

---

## Documentation Authority & Inventory

### Authority Model & Scoped Precedence
1. **Normative Baseline (SSOT)**: [`docs/ENGINEERING_STANDARD_V1.md`](docs/ENGINEERING_STANDARD_V1.md) is the single source of truth for all normative mathematical, architectural, and engineering rules.
2. **Authorized Scoped Exceptions**: [`docs/DEVIATIONS.md`](docs/DEVIATIONS.md) is the Engineering Standard's formally authorized exception mechanism per Sections 101 & 102. Within an explicitly registered deviation scope, the registered deviation modifies only the cited rule for the declared component and lifetime; outside that scope, the Standard remains fully authoritative.
3. **Current Specifications & Overviews**: Subordinate specifications ([`docs/VECTORIS_RENAME_MIGRATION.md`](docs/VECTORIS_RENAME_MIGRATION.md), [`docs/PURE_MATH_SCOPE.md`](docs/PURE_MATH_SCOPE.md), [`docs/MATHEMATICAL_CONVENTIONS.md`](docs/MATHEMATICAL_CONVENTIONS.md), [`docs/core.md`](docs/core.md), [`docs/units.md`](docs/units.md), [`docs/geometry.md`](docs/geometry.md), [`modules/VectorisDynamics/docs/dynamics.md`](modules/VectorisDynamics/docs/dynamics.md)) and this overview ([`README.md`](README.md)) must strictly conform to the **Engineering Standard plus applicable registered deviations**.
4. **Audit & Evidence**: Audit documents ([`docs/audits/`](docs/audits/)) record historical findings and live qualification evidence; they carry zero normative authority to modify requirements.
5. **Deprecated / Historical**: Legacy notes ([`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), [`docs/CODING_STANDARD.md`](docs/CODING_STANDARD.md), [`CHANGELOG.md`](CHANGELOG.md), [`TECHDEBT.md`](TECHDEBT.md), [`TODO.md`](TODO.md)) are superseded and non-authoritative.

### Complete Documentation Inventory

| Authority Classification | File | Description / Scope |
| :--- | :--- | :--- |
| **Entry Point** | [`README.md`](README.md) | Repository overview, subsystem architecture, build commands, and documentation index. |
| **Normative (SSOT)** | [`docs/ENGINEERING_STANDARD_V1.md`](docs/ENGINEERING_STANDARD_V1.md) | Authoritative Single Source of Truth for engineering rules and coding standards. |
| **Governance** | [`docs/DEVIATIONS.md`](docs/DEVIATIONS.md) | Formal deviation ledger tracking intentional, scoped exceptions to normative rules. |
| **Migration Guide** | [`docs/VECTORIS_RENAME_MIGRATION.md`](docs/VECTORIS_RENAME_MIGRATION.md) | Authoritative migration guide documenting product, namespace, header, target mappings. |
| **Current Specification** | [`docs/PURE_MATH_SCOPE.md`](docs/PURE_MATH_SCOPE.md) | Pure mathematics scope definition: mission, allowed/forbidden categories, decision framework. |
| **Current Specification** | [`docs/MATHEMATICAL_CONVENTIONS.md`](docs/MATHEMATICAL_CONVENTIONS.md) | Cross-module coordinate systems, rotation directions, and transformation pipeline conventions. |
| **Current Specification** | [`docs/core.md`](docs/core.md) | Core layer specification: scalar types, bounded numerical functions, IEEE-754 traits, error model. |
| **Current Specification** | [`docs/units.md`](docs/units.md) | Units layer specification: Model B 8D dimensional algebra, unit tags, quantity ABI validation. |
| **Current Specification** | [`docs/geometry.md`](docs/geometry.md) | Geometry layer specification: frame safety, matrix linear algebra, quaternion conventions, SPD solver. |
| **Downstream Specification**| [`modules/VectorisDynamics/docs/dynamics.md`](modules/VectorisDynamics/docs/dynamics.md) | Dynamics module specification: spatial quantities, rigid-body state, inertia tensor, Euler integrator. |
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
