# Vectoris Global Rename Migration Guide & Specification

> [!IMPORTANT]
> **Document**: Vectoris Global Rename Migration Guide & Specification  
> **Version**: 1.0  
> **Status**: Historical Migration Record; not a current qualification report
> **Baseline Commit**: `f9ebd7783621d7150a2761e52f6bbeab46bc4a45`  
> **Target Release**: Vectoris v1.0.0-RC1  
> **Date**: 2026-09-17  

> [!WARNING]
> This document records the rename snapshot at baseline `f9ebd7783621d7150a2761e52f6bbeab46bc4a45`.
> Its R1 qualification, test, header, and coverage statements below are superseded
> by later red-team remediation. The current project status is
> **NOT REQUALIFIED / Experimental**. See the append-only remediation ledger for
> step-specific local evidence; no old count in this migration record is a current
> metric.

---

## 1. Executive Summary & Rationale

In September 2026, the **AegisMathLib** project completed its transition from an ad-hoc geometry and units utility library to a high-reliability, mathematically verified numerical computing and simulation framework. As part of this evolution, the project was restructured into a unified product family under the name **Vectoris**.

### Core Drivers for Migration:
1. **Product Line Expansion**: Vectoris encompasses a modular ecosystem of high-reliability scientific computing libraries:
   - `VectorisNumerics`: Foundational ISO C++20 pure mathematics, IEEE-754 traits, 8D dimensional analysis, and SE(3) spatial geometry.
   - `VectorisDynamics`: 6-DOF rigid-body mechanics, spatial quantity vectors, inertia tensors, and semi-implicit Euler integration.
   - *Future planned modules*: `VectorisEstimation`, `VectorisControl`, `VectorisSignal`, `VectorisSimulation`.
2. **Namespace Standard Conformance**: The legacy PascalCase root namespace `AegisMath` (which required a formal deviation `AML-DEVIATION-003`) was replaced with modern ISO C++ conforming lowercase hierarchical namespaces: `vectoris::numerics` and `vectoris::dynamics`.
3. **Strict Zero-Coupling Architecture**: Header locations and include paths reflect clean physical module boundaries with zero legacy umbrella leakage.
4. **Migration-Snapshot Scope**: The rename commit itself was intended as an architectural, naming, and structural migration. The historical statement that no mathematical formulas changed applies only to that migration snapshot; later VRT remediations do intentionally change numerical contracts.

---

## 2. Comprehensive Migration Mappings

### 2.1 Product & Repository Naming
| Component | Legacy Name | Vectoris Canonical Name |
| :--- | :--- | :--- |
| **Product Suite** | AegisMathLib | **Vectoris** |
| **Pure Numerics Module** | `AegisMathLib` (`modules/AegisMathLib`) | **`VectorisNumerics` (`modules/VectorisNumerics`)** |
| **Dynamics Physics Module** | `AegisDynamics` (`modules/AegisDynamics`) | **`VectorisDynamics` (`modules/VectorisDynamics`)** |
| **GitHub Repository** | `steven1n/AegisMathLib` | **`steven1n/Vectoris`** |

---

### 2.2 Public Include Headers
All public include paths were migrated to namespaced directory hierarchies:

| Subsystem | Legacy Include Path | Vectoris Include Path |
| :--- | :--- | :--- |
| **Numerics Core** | `<AegisMath/Core/BasicTypes.h>` | `<Vectoris/Numerics/Core/BasicTypes.h>` |
| | `<AegisMath/Core/Compiler.h>` | `<Vectoris/Numerics/Core/Compiler.h>` |
| | `<AegisMath/Core/Concepts.h>` | `<Vectoris/Numerics/Core/Concepts.h>` |
| | `<AegisMath/Core/Constants.h>` | `<Vectoris/Numerics/Core/Constants.h>` |
| | `<AegisMath/Core/Math.h>` | `<Vectoris/Numerics/Core/Math.h>` |
| | `<AegisMath/Core/MathError.h>` | `<Vectoris/Numerics/Core/MathError.h>` |
| | `<AegisMath/Core/MathFunctions.h>` | `<Vectoris/Numerics/Core/MathFunctions.h>` |
| | `<AegisMath/Core/NumericTraits.h>` | `<Vectoris/Numerics/Core/NumericTraits.h>` |
| | `<AegisMath/Core/Precision.h>` | `<Vectoris/Numerics/Core/Precision.h>` |
| | `<AegisMath/Core/Result.h>` | `<Vectoris/Numerics/Core/Result.h>` |
| **Numerics Units** | `<AegisMath/Units/BaseUnits/*.h>` | `<Vectoris/Numerics/Units/BaseUnits/*.h>` |
| | `<AegisMath/Units/DerivedUnits/*.h>` | `<Vectoris/Numerics/Units/DerivedUnits/*.h>` |
| | `<AegisMath/Units/Detail/*.h>` | `<Vectoris/Numerics/Units/Detail/*.h>` |
| | `<AegisMath/Units/*.h>` | `<Vectoris/Numerics/Units/*.h>` |
| **Numerics Geometry** | `<AegisMath/Geometry/*.h>` | `<Vectoris/Numerics/Geometry/*.h>` |
| | `<AegisMath/Geometry/Detail/*.h>` | `<Vectoris/Numerics/Geometry/Detail/*.h>` |
| **Dynamics** | `<AegisDynamics/*.h>` | `<Vectoris/Dynamics/*.h>` |
| | `<AegisDynamics/Detail/*.h>` | `<Vectoris/Dynamics/Detail/*.h>` |

---

### 2.3 C++ Namespaces
| Subsystem | Legacy Namespace | Vectoris Canonical Namespace |
| :--- | :--- | :--- |
| **Numerics Root** | `namespace AegisMath` | `namespace vectoris::numerics` |
| **Numerics Core** | `namespace AegisMath::Core` | `namespace vectoris::numerics::core` |
| **Numerics Units** | `namespace AegisMath::Units` | `namespace vectoris::numerics::units` |
| **Numerics Geometry** | `namespace AegisMath::Geometry` | `namespace vectoris::numerics::geometry` |
| **Dynamics** | `namespace AegisDynamics` | `namespace vectoris::dynamics` |

For the current public contract, lowercase `core`, `units`, and `geometry` are
canonical. `Core`, `Units`, and `Geometry` remain nondeprecated compatibility
spellings for this release. Each public Numerics header provides its namespace
contract independently; see the Engineering Standard and module specifications.

---

### 2.4 Preprocessor Macros & Header Guards
| Category | Legacy Identifier | Vectoris Canonical Identifier |
| :--- | :--- | :--- |
| **Header Guard Prefix** | `AEGIS_MATH_*` | `VECTORIS_NUMERICS_*` |
| **Dynamics Guard Prefix** | `AEGIS_DYNAMICS_*` | `VECTORIS_DYNAMICS_*` |
| **Compiler Version Macro** | `AEGIS_CPLUSPLUS` | `VECTORIS_CPLUSPLUS` |
| **Compiler Inline Macro** | `AEGIS_FORCE_INLINE` | `VECTORIS_FORCE_INLINE` |
| **ABI Version Macro** | `AEGIS_MATH_ABI_VERSION` | `VECTORIS_NUMERICS_ABI_VERSION` |
| **Dynamics ABI Macro** | `AEGIS_DYNAMICS_ABI_VERSION` | `VECTORIS_DYNAMICS_ABI_VERSION` |

---

### 2.5 CMake Targets & Configuration Options
| Category | Legacy Identifier | Vectoris Canonical Identifier |
| :--- | :--- | :--- |
| **Root Project** | `project(AegisMathLib LANGUAGES CXX)` | `project(Vectoris LANGUAGES CXX)` |
| **Numerics Target** | `AegisMathLib` | `VectorisNumerics` |
| **Numerics Alias** | `AegisMathLib::AegisMathLib` | `Vectoris::Numerics` |
| **Dynamics Target** | `AegisDynamics` | `VectorisDynamics` |
| **Dynamics Alias** | `AegisDynamics::AegisDynamics` | `Vectoris::Dynamics` |
| **Numerics Test Target** | `AegisMathLib_Tests` | `VectorisNumerics_Tests` |
| **Dynamics Test Target** | `AegisDynamics_Tests` | `VectorisDynamics_Tests` |
| **Numerics Isolation** | `AegisMathLib_HeaderIsolation` | `VectorisNumerics_HeaderIsolation` |
| **Dynamics Isolation** | `AegisDynamics_HeaderIsolation` | `VectorisDynamics_HeaderIsolation` |
| **Coverage Target** | `AegisMathLib_Coverage` | `VectorisNumerics_Coverage` |
| **Clang-Tidy Target** | `AegisMathLib_ClangTidy` | `VectorisNumerics_ClangTidy` |
| **Option: Warning Mode** | `AEGIS_STRICT_WARNINGS` | `VECTORIS_STRICT_WARNINGS` |
| **Option: Build Dynamics**| `AEGIS_BUILD_DYNAMICS` | `VECTORIS_BUILD_DYNAMICS` |
| **Option: ASan** | `AEGIS_ENABLE_ASAN` | `VECTORIS_ENABLE_ASAN` |
| **Option: UBSan** | `AEGIS_ENABLE_UBSAN` | `VECTORIS_ENABLE_UBSAN` |
| **Option: Coverage** | `AEGIS_ENABLE_COVERAGE` | `VECTORIS_ENABLE_COVERAGE` |
| **Option: Static Analysis**| `AEGIS_ENABLE_STATIC_ANALYSIS` | `VECTORIS_ENABLE_STATIC_ANALYSIS` |

---

### 2.6 CMake Presets
| Preset Name | Function | Module Scope |
| :--- | :--- | :--- |
| `debug` | Debug build, assertions enabled | All active modules (`VectorisNumerics` + `VectorisDynamics`) |
| `release` | Release build, optimized | All active modules |
| `pure-numerics` | Standalone pure-mathematics build | `VectorisNumerics` only (`VECTORIS_BUILD_DYNAMICS=OFF`) |
| `pure-math` | Legacy alias for `pure-numerics` | `VectorisNumerics` only (backward compatibility) |
| `asan` | AddressSanitizer debug build | All active modules |
| `ubsan` | UndefinedBehaviorSanitizer debug build | All active modules |
| `asan-ubsan` | Combined ASan+UBSan debug build | All active modules |
| `coverage` | Source-based code coverage build | `VectorisNumerics` |
| `static-analysis` | Clang-Tidy static analysis build | `VectorisNumerics` |

---

### 2.7 Governance & Deviations
| Item | Legacy Status | Vectoris Status | Rationale |
| :--- | :--- | :--- | :--- |
| **`AML-DEVIATION-003`** | `REGISTERED / ACCEPTED` | **`RESOLVED / CLOSED`** | Root namespace migrated from `AegisMath` to standard `vectoris::numerics`. |
| **Active Deviations** | 2 (`AML-DEVIATION-002`, `003`) | At the migration snapshot, `AML-DEVIATION-002` remained active. | Consult the current [`DEVIATIONS.md`](DEVIATIONS.md); later remediations registered additional scoped deviations. |

---

## 3. Historical Integrity & Documentation Policy

To preserve historical audit records without falsifying past engineering logs:
1. **Audit Logs Preserved**: Historical audit documents in [`docs/audits/`](audits/) maintain their historical file names, finding identifiers (`AML-CRIT-*`, `AML-HIGH-*`, `AML-MED-*`, `AML-LOW-*`), and historical text unchanged.
2. **Disclaimer Annotations**: Each historical audit document contains an authoritative disclaimer noting its role as an immutable historical record of the pre-migration baseline.
3. **Migration-Snapshot Documentation**: The listed specifications were synchronized at the migration snapshot. Their current contracts and namespace details are maintained in the Engineering Standard and module specifications.

---

## 4. Historical Verification Evidence

The R1 result recorded by the migration-era qualification documents applied only
to the historical migration snapshot identified above. Its test, header-isolation,
and coverage counts are intentionally not repeated here as current metrics. The
old R1 report contains two different baseline hashes in its header and its
environment section; that inconsistency is documented in the report and neither
hash identifies the current working tree. Current coverage uses raw LLVM branch
counts without denominator adjustment, and current local findings remain
**NOT REQUALIFIED / Experimental**. Consult the dated incremental remediation
ledger for later, scoped verification evidence.
