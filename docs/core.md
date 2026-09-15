# AegisMathLib Core Module Specification

> [!IMPORTANT]
> **Document**: Core Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Core` module provides the foundational type system, scalar definitions, compiler abstraction, floating-point numerical traits, monadic error handling, and mathematical primitive functions for AegisMathLib. It establishes the lowest-level layer upon which all higher mathematical subsystems (`Units`, `Geometry`, `Dynamics`) depend.

---

## 2. Scope

The `Core` module encompasses:
- Standard scalar definitions and compile-time floating-point concepts.
- Strict IEEE-754 numerical traits, tolerances, and comparison utilities.
- Standard-library-backed monadic error representation (`Result<T, MathError>`).
- Compiler feature detection (`AEGIS_CPLUSPLUS`) and portability macros.
- Bounded constexpr elementary mathematical functions (`Core::Math::sqrt`, `acos`, `asin`, `clamp`).

---

## 3. Dependency Rules

- **Layer Position**: Layer 0 (Base Layer).
- **Inbound Dependencies**: Consumed by `Units`, `Geometry`, `Dynamics`, and test suites.
- **Outbound Dependencies**: **Zero internal dependencies**. The `Core` module depends strictly and exclusively on the ISO C++20 standard library headers:
  - `<concepts>`, `<type_traits>`, `<limits>`, `<cmath>`, `<variant>`, `<cstdint>`, `<utility>`
- **Architecture Constraint**: `Core` MUST NEVER include any header from `Units`, `Geometry`, or `Dynamics`.

---

## 4. Public Headers

The `Core` module exposes exactly **10 public headers** under `include/AegisMath/Core/`:

| Header | Description |
| :--- | :--- |
| [`BasicTypes.h`](../include/AegisMath/Core/BasicTypes.h) | Fundamental type aliases (`Real`, `Float32`, `Float64`, `Int32`, `UInt32`, `Bool`, etc.). |
| [`Compiler.h`](../include/AegisMath/Core/Compiler.h) | Standard compliance detection and compiler-specific attribute abstractions (`AEGIS_CPLUSPLUS`). |
| [`Concepts.h`](../include/AegisMath/Core/Concepts.h) | C++20 concepts (`Concepts::FloatingPoint`, `Concepts::NumericInteger`, `Concepts::SupportedSqrtScalar`). |
| [`Constants.h`](../include/AegisMath/Core/Constants.h) | Mathematical constants ($\pi$, $e$, $\sqrt{2}$, $\ln 2$, machine epsilons) with full 64-bit precision. |
| [`Math.h`](../include/AegisMath/Core/Math.h) | Root namespace umbrella including `MathFunctions.h`, `NumericTraits.h`, and `Constants.h`. |
| [`MathError.h`](../include/AegisMath/Core/MathError.h) | Strongly typed error enum `MathError` and string converter `to_string(MathError)`. |
| [`MathFunctions.h`](../include/AegisMath/Core/MathFunctions.h) | Bounded numerical functions: `Core::Math::sqrt`, `clamp`, `acos`, `asin`, `deg2rad`, `rad2deg`. |
| [`NumericTraits.h`](../include/AegisMath/Core/NumericTraits.h) | Compile-time IEEE-754 traits: `AlmostEqual`, `IsZero`, `IsFinite`, `IsNaN`. |
| [`Precision.h`](../include/AegisMath/Core/Precision.h) | Default floating-point scalar alias `Scalar = double;`. |
| [`Result.h`](../include/AegisMath/Core/Result.h) | `std::variant`-backed error container `Result<T, MathError>`. |

---

## 5. Scalar Policy

- **Default Scalar**: `Scalar` is alias to IEEE-754 `double` (64-bit double precision).
- **Supported Floating-Point Types**: `float` (32-bit single precision) and `double` (64-bit double precision).
- **Constrained Function Domain**: `Core::Math::sqrt` is explicitly constrained by `Concepts::SupportedSqrtScalar` to `float` and `double`.
- **Unsupported Types for Mathematical Kernels**:
  - Integral types (`int`, `long`, `int64_t`) are rejected at compile time for square roots and transcendental functions.
  - `long double` is unsupported: its representation and precision are implementation-defined and vary across toolchains/platforms; it is not part of the supported scalar contract, and there is no current requirement for it.

---

## 6. Error Model

AegisMathLib enforces a strict **zero-exception** policy across all mathematical kernels (Rule 6 and Section 36 of Engineering Standard V1). Errors are represented as strongly typed enumerators in `Core::MathError`:

```cpp
namespace AegisMath::Core {
    enum class MathError : std::uint8_t {
        invalid_argument = 1,
        domain_error,
        non_finite_input,
        singular_matrix,
        ill_conditioned,
        zero_norm,
        normalization_failure,
        non_convergence,
        max_iterations,
        invalid_state
    };
}
```

Swallowing errors or returning sentinel scalars (such as `-1` or `0.0` on invalid state) is strictly prohibited at API boundaries.

---

## 7. `Result<T, MathError>`

All fallible mathematical operations return `Core::Result<T, MathError>` (or `Core::Result<T, E>`):
- **Implementation**: Backed by `std::variant<T, E>` (`storage_`). Placement-new and raw union allocations are eliminated.
- **Constexpr Capability**: Fully functional in compile-time `constexpr` contexts.
- **Contract & API Surface**:
  - `has_value()`, `IsSuccess()`, `explicit operator bool()`: Query whether the result holds a valid value.
  - `value()`, `Value()`: Access payload by reference/rvalue. Precondition: `has_value() == true`. Asserts in Debug; undefined behavior in Release if violated.
  - `error()`: Access error enumerator by reference/rvalue. Precondition: `!has_value() == true`. Asserts in Debug; undefined behavior in Release if violated.
  - `value_or(default_val)`: Returns value if successful, or fallback `default_val`.
  - `value_if()`: Safe checked pointer accessor. Returns `const T*` / `T*` if successful, or `nullptr` on failure.
  - `error_if()`: Safe checked pointer accessor. Returns `const E*` / `E*` on failure, or `nullptr` on success.
  - `Result::success(...)`, `Result::failure(...)`: Explicit static factory functions.

---

## 8. `NumericTraits`

`AegisMath::Core::NumericTraits<T>` provides type-safe numerical queries:
- `epsilon()`: Machine epsilon ($2.22 \times 10^{-16}$ for `double`, $1.19 \times 10^{-7}$ for `float`).
- `almost_equal(a, b, abs_tol, rel_tol)`: Dual-tolerance comparison:
  $$|a - b| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a|, |b|))$$
- Direct `==` comparisons on computed floating-point quantities are forbidden by Rule 9.

---

## 9. Math Functions

Mathematical helpers in `Core::Math` enforce robust domain contracts:
- `clamp(val, min_val, max_val)`: Constrains scalar to $[min, max]$.
- `acos(val)`: Clamps input to $[-1.0, 1.0]$ before computing arc-cosine, preventing NaN generation from slight numerical overflow (e.g. $\arccos(-1.0000000000000002) = \pi$).
- `asin(val)`: Clamps input to $[-1.0, 1.0]$.
- `deg2rad(deg)`, `rad2deg(rad)`: Explicit conversion constants.

---

## 10. `Core::Math::sqrt` Exact Contract

`Core::Math::sqrt(T x)` implements the **AegisMath project-specific domain policy**. It is NOT described as `std::sqrt`-compatible domain semantics or full IEEE-754 sqrt semantics, because of its intentional safety clamping on negative finite values.

### 10.1 Domain Policy

| Input Condition | Return Value | Rationale |
| :--- | :--- | :--- |
| Finite $x > 0$ | $\sqrt{x}$ | Standard square root approximation. |
| $+0.0$ | $+0.0$ | Zero root. |
| $-0.0$ | $+0.0$ | Signed zero normalized to $+0.0$. |
| Finite $x < 0$ | $+0.0$ | Project-specific non-negative clamping for embedded loop resilience. |
| $-\infty$ | $+0.0$ | Clamped to non-negative domain. |
| $+\infty$ | $+\infty$ | Positive infinity preserved. |
| $\text{NaN}$ | $\text{NaN}$ | Quiet NaN propagation. |

> [!NOTE]
> Returning $+0.0$ for negative finite inputs, signed zero (`-0.0`), and $-\infty$ is the intentional, project-specific numerical domain policy of `AegisMath::Core`, designed to safeguard recursive state estimation and long-term simulation loops against sudden NaN corruption.

### 10.2 Compile-Time Execution (`constexpr`)
- Employs bounded Newton-Raphson iteration (`Detail::BoundedNewtonSqrt`).
- Hard iteration cap: `kMaxIterations = 64` (strictly enforcing Rule 7).
- Scale-aware initial estimate via IEEE-754 exponent halving (`Detail::InitialSqrtGuess`):
  - Normal `double`: `(bits >> 1) + (1023ULL << 51)` using `std::bit_cast<uint64_t>`.
  - Subnormal `double`: scaled by $2^{52}$, exponent halved, and scaled back by $2^{-26}$.
  - Normal `float`: `(bits >> 1) + (127U << 22)` using `std::bit_cast<uint32_t>`.
  - Subnormal `float`: scaled by $2^{24}$, exponent halved, and scaled back by $2^{-12}$.
- Scale-aware termination criterion:
  `curr == prev || abs(curr - prev) <= eps * (curr > prev ? curr : prev)`.

### 10.3 Runtime Execution
- Delegates directly to the standard library `std::sqrt` implementation for non-negative values, guaranteeing compiler intrinsics and FPU vectorization without hand-rolled runtime loops.

---

## 11. Floating-Point Policy

- Adheres strictly to Section 14–18 of Engineering Standard V1.
- Fast-math optimization flags (`-ffast-math`, `/fp:fast`, `-Ofast`) are strictly forbidden in build configurations.
- Subnormal numbers are preserved unless hardware flushing is explicitly requested at the system root level.
- Non-finite inputs ($\text{NaN}$, $\pm\infty$) must be detected and rejected with `MathError::non_finite_input` where mathematical kernels require finite convergence.

---

## 12. Determinism

- Follows pure state-in / state-out contracts.
- Zero dependency on system clocks, wall time (`std::chrono::system_clock`), or hardware randomness (`std::random_device`, `rand`).
- Production headers contain no hidden RNG, wall-clock dependencies, or mutable static state (Hidden nondeterminism source audit: PASS).
- Bitwise reproducibility across differing compilers, architectures, or floating-point environments has not been benchmarked (Cross-build/runtime reproducibility: NOT RUN).

---

## 13. Allocation Policy

- Zero explicit dynamic allocation APIs (`new`, `delete`, `malloc`, `free`) or dynamic containers (`std::vector`, `std::string`) in `Core` headers (Explicit Allocation Audit: PASS).
- Runtime heap allocation interception under full workload stress is tracked as NOT RUN.
- All core types are fixed-size, standard-layout, trivially destructible value objects.

---

## 14. Failure Modes

| Failure Mode | Detection Mechanism | Handling Policy |
| :--- | :--- | :--- |
| Non-finite input | `std::isnan()`, `std::isinf()` | Return `Result::failure(MathError::non_finite_input)`. |
| Zero norm divisor | `Traits::IsZero(sq_len)` | Return `Result::failure(MathError::zero_norm)`. |
| Out of domain / Invalid argument | Boundary or sign check | Return `Result::failure(MathError::domain_error)` or `MathError::invalid_argument`. |
| Iteration limit exceeded | Iteration counter $> kMaxIterations` | Return `Result::failure(MathError::max_iterations)`. |

---

## 15. Verification Evidence

The `Core` module contracts are verified by dedicated test suites:
- [`tests/Core/NumericTraitsTest.cpp`](../tests/Core/NumericTraitsTest.cpp): Machine epsilon, dual-tolerance comparison.
- [`tests/Core/MathFunctionsTest.cpp`](../tests/Core/MathFunctionsTest.cpp): `CoreSqrtTest` (signed zero, negative domain clamping, constexpr verification, convergence bounds), `acos`/`asin` boundary clamping.
- [`tests/Core/ResultTest.cpp`](../tests/Core/ResultTest.cpp): Monadic chaining, constexpr execution, ABI triviality.
- [`tests/Core/PublicTemplateInstantiationTest.cpp`](../tests/Core/PublicTemplateInstantiationTest.cpp): Explicit template instantiation for `float` and `double`.

---

## 16. Known Deviations

1. **AML-DEVIATION-003**: Top-level namespace `AegisMath::Core` instead of standard-mandated `aegis::math::core` (registered in [`docs/DEVIATIONS.md`](DEVIATIONS.md)).
