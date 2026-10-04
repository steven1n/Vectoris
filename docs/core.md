# Vectoris Core Module Specification

> [!IMPORTANT]
> **Document**: Core Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **C14 Remediation Base**: `60264514522cd22b1725b9918662a6d397e1ab24` (frozen Candidate #13)
> **Last Updated**: 2026-10-03
> **Current Qualification**: NOT FINALIZED / Experimental; independent confirmation pending
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Core` module provides the foundational type system, scalar definitions, compiler abstraction, floating-point numerical traits, monadic error handling, and mathematical primitive functions for Vectoris. It establishes the lowest-level layer upon which all higher mathematical subsystems (`Units`, `Geometry`) and downstream modules (`VectorisDynamics`) depend.

---

## 2. Scope

The `Core` module encompasses:
- Standard scalar definitions and compile-time floating-point concepts.
- Strict IEEE-754 numerical traits, tolerances, and comparison utilities.
- Standard-library-backed monadic error representation (`Result<T, MathError>`).
- Compiler feature detection (`VECTORIS_CPLUSPLUS`) and portability macros.
- Canonical scalar square root `core::sqrt`, with bounded float/double constant evaluation and standard-library runtime evaluation.
- Existing `core::Math` wrappers for `abs`, `sin`, `cos`, `acos` and compatibility `sqrt`.

### Public namespace contract (VRT-12)

Use `vectoris::numerics::core` in new public code. `vectoris::numerics::Core`
remains a compatibility spelling, **not deprecated**. Definitions stay in Core;
the alias in `Core/Namespace.h` denotes the same entities, with the same ADL and
template specializations. Every Core header directly includes this contract, so
lookup is self-sufficient and independent of include order. For example:

```cpp
#include <Vectoris/Numerics/Core/Math.h>
static_assert(vectoris::numerics::core::sqrt(4.0) == 2.0);
```

`Core::` examples elsewhere in this specification use the retained compatibility
spelling. Root scalar aliases and the existing `Concepts`, `Constants`, and
`Traits` namespaces retain their locations. `NumericTraits.h` selectively exposes
`core::AlmostEqual` / `Core::AlmostEqual` as the existing `Traits::AlmostEqual`
entity; it does not import Traits wholesale. `core::sqrt` from Math.h is canonical.
`core::Math::sqrt` from MathFunctions.h is a compatibility-only forwarding function
with identical scalar/domain semantics; neither spelling is deprecated.
`Detail` is implementation-only under either spelling, not a supported API.
Namespace aliases do not provide a binary/ABI qualification guarantee.

---

## 3. Dependency Rules

- **Layer Position**: Layer 0 (Base Layer).
- **Inbound Dependencies**: Consumed by `Units`, `Geometry`, downstream modules (`VectorisDynamics`), and test suites.
- **Outbound Dependencies**: **Zero internal dependencies**. The `Core` module depends strictly and exclusively on the ISO C++20 standard library headers:
  - `<concepts>`, `<type_traits>`, `<limits>`, `<cmath>`, `<variant>`, `<cstdint>`, `<utility>`
- **Architecture Constraint**: `Core` MUST NEVER include any header from `Units`, `Geometry`, or downstream domain modules (`VectorisDynamics`).

---

## 4. Public Headers

The `Core` module exposes exactly **11 public headers** under `include/Vectoris/Numerics/Core/`:

| Header | Description |
| :--- | :--- |
| [`Namespace.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Namespace.h) | Authoritative canonical/compatibility namespace declaration; included by every Core header. |
| [`BasicTypes.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/BasicTypes.h) | Fundamental type aliases (`Real`, `Float32`, `Float64`, `Int32`, `UInt32`, `Bool`, etc.). |
| [`Compiler.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Compiler.h) | Standard compliance detection and compiler-specific attribute abstractions (`VECTORIS_CPLUSPLUS`). |
| [`Concepts.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Concepts.h) | C++20 concepts (`Concepts::FloatingPoint`, `Concepts::NumericInteger`, `Concepts::SupportedSqrtScalar`). |
| [`Constants.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Constants.h) | Mathematical constants ($\pi$, $e$, $\sqrt{2}$, $\ln 2$, machine epsilons) with full 64-bit precision. |
| [`MathError.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/MathError.h) | Strongly typed error enum `MathError` and string converter `to_string(MathError)`. |
| [`Math.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Math.h) | Primitive entry: canonical `core::sqrt` and existing `core::abs`; include `MathFunctions.h` explicitly for wrappers. |
| [`MathFunctions.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/MathFunctions.h) | `core::Math::{abs,sin,cos,acos}` and compatibility `sqrt`. |
| [`NumericTraits.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/NumericTraits.h) | Compile-time IEEE-754 traits: `AlmostEqual`, `IsZero`, `IsFinite`, `IsNaN`. |
| [`Precision.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Precision.h) | Default floating-point scalar alias `Scalar = double;`. |
| [`Result.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Core/Result.h) | `std::variant`-backed error container `Result<T, MathError>`. |

### Documented Core consumer entrances (AFA3-001)

`Math.h` is a narrow primitive header, not a Core umbrella. It does not export
`core::Math::sin` or `core::Math::cos`. Include `MathFunctions.h` explicitly for
the wrappers, `NumericTraits.h` for traits/comparisons, and `Constants.h` for
constants. This preserves the dependency direction `MathFunctions.h -> Math.h`
and Engineering Standard sections 7 and 55 (one major concept and minimal,
self-contained dependencies). No production include graph or function changes.

These complete consumer examples are extracted, compiled with strict warnings,
and executed by the mandatory Core include-contract CTest/API gate. Each example
includes only its documented Vectoris entrance. The manifest records their exact
identities; missing, duplicate, unexpected or failing examples fail the gate.

<!-- vectoris-core-contract: primitives -->
```cpp
#include <Vectoris/Numerics/Core/Math.h>
static_assert(vectoris::numerics::core::sqrt(4.0) == 2.0);
static_assert(vectoris::numerics::core::abs(-2.0) == 2.0);
int main() { return 0; }
```

<!-- vectoris-core-contract: wrappers -->
```cpp
#include <Vectoris/Numerics/Core/MathFunctions.h>
template <typename T> bool wrappers_work() {
    const T epsilon = vectoris::numerics::Traits::NumericTraits<T>::epsilon();
    return vectoris::numerics::core::AlmostEqual(
               vectoris::numerics::core::Math::sin(T{0}), T{0}, epsilon, epsilon) &&
           vectoris::numerics::core::AlmostEqual(
               vectoris::numerics::core::Math::cos(T{0}), T{1}, epsilon, epsilon) &&
           vectoris::numerics::core::AlmostEqual(
               vectoris::numerics::core::Math::sqrt(T{4}), T{2}, epsilon, epsilon);
}
int main() { return wrappers_work<float>() && wrappers_work<double>() ? 0 : 1; }
```

<!-- vectoris-core-contract: traits -->
```cpp
#include <Vectoris/Numerics/Core/NumericTraits.h>
static_assert(vectoris::numerics::Traits::NumericTraits<double>::epsilon() > 0.0);
int main() {
    return vectoris::numerics::core::AlmostEqual(1.0, 1.0, 0.0, 0.0) ? 0 : 1;
}
```

<!-- vectoris-core-contract: constants -->
```cpp
#include <Vectoris/Numerics/Core/Constants.h>
static_assert(vectoris::numerics::Constants::Pi<double> > 3.0);
static_assert(vectoris::numerics::Constants::Pi<double> < 4.0);
int main() { return 0; }
```

---

## 5. Scalar Policy

- **Default Scalar**: `Scalar` is alias to IEEE-754 `double` (64-bit double precision).
- **Portable binary numerical scope**: `float` (binary32) and `double` (binary64).
- **sqrt scalar domain**: `Concepts::SupportedSqrtScalar` accepts built-in floating
  types: float, double and long double (including cv/ref qualification in the trait).
  Both public sqrt templates deduce their by-value scalar type and reject integral,
  bool and user-defined conversion sources. Explicit caller casts remain possible.
- **long double**: sqrt runtime support preserves the historical Math wrapper and
  Geometry's documented long-double inputs. Its representation is not bit-inspected;
  no portable ISO C++20 constexpr guarantee is made for this type. This does not
  broaden the supported matrix of every other numerical API.
- Type/constexpr claims for sqrt do not imply that all other transcendental wrappers
  have the same constraints or support constant evaluation.

---

### Absolute-value scalar contract (NR-001 / Candidate #16)

Both `core::abs` (Math.h) and `core::Math::abs` (MathFunctions.h) support built-in
floating-point types and `Concepts::NumericInteger` types. Numeric integers include
signed/unsigned char, short, int, long and long long; bool and the character types
char, wchar_t, char8_t, char16_t and char32_t are excluded, as in the existing Core
concept policy. User-defined scalar conversion sources are not implicitly accepted.

Floating inputs retain their original same-type, constexpr, noexcept behavior,
including signed zero, infinities and NaNs. Signed numeric integers return
`std::make_unsigned_t<T>`: the exact nonnegative magnitude, including `min(T)`.
Unsigned numeric integers return the original type and value. Integer overloads
are constexpr and noexcept; their computation never negates a signed minimum.
For an N-bit signed integer, `abs(min(T)) = 2^(N-1)` cannot fit in T; returning an
unsigned magnitude avoids both undefined overflow and silent saturation.

```cpp
#include <Vectoris/Numerics/Core/Math.h>
#include <limits>
#include <type_traits>
static_assert(std::is_same_v<decltype(vectoris::numerics::core::abs(-1)), unsigned int>);
static_assert(vectoris::numerics::core::abs(std::numeric_limits<int>::min()) ==
              static_cast<unsigned int>(std::numeric_limits<int>::max()) + 1u);
```

Compatibility: v1.0.0 accepted signed integers and returned T; NR-001 was found in
that release. Candidate #16 changes the signed return type and constrains previously
unconstrained templates to numeric scalar types. External callers relying on signed
return deduction, function pointers, overload selection or conversions must review
this change. Patch-release compatibility requires version-policy review; this is
not a claim of ABI compatibility or an authorization to release v1.0.1.

---

## 6. Error Model

Vectoris enforces a strict **zero-exception** policy across all mathematical kernels (Rule 6 and Section 36 of Engineering Standard V1). Errors are represented as strongly typed enumerators in `Core::MathError`:

```cpp
namespace vectoris::numerics::core {
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

All fallible mathematical operations return `core::Result<T, MathError>` (or `core::Result<T, E>`; `Core` remains the compatibility spelling):
- **Implementation**: Backed by `std::variant<T, E>` (`storage_`). Placement-new and raw union allocations are eliminated.
- **Constexpr Capability**: Available when the selected payload operations support constant evaluation.
- **Two-state invariant**: Exactly one valid T or E exists for every constructed Result, including after a supported assignment throws. Payload destruction must be nonthrowing; assignment availability is constrained as specified below.
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

`vectoris::numerics::Traits::NumericTraits<T>` provides type-safe numerical queries:
- `epsilon()`: Machine epsilon ($2.22 \times 10^{-16}$ for `double`, $1.19 \times 10^{-7}$ for `float`).
- `almost_equal(a, b, abs_tol, rel_tol)`: Dual-tolerance comparison:
  $$|a - b| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a|, |b|))$$
- Direct `==` comparisons on computed floating-point quantities are forbidden by Rule 9.

### 8.1 `AlmostEqual` finite and special-value contract (VRT-09)

The canonical callable API is
`vectoris::numerics::core::AlmostEqual(a, b, absoluteTolerance, relativeTolerance)`;
the original `Traits::AlmostEqual` and `Core::AlmostEqual` name the same function.
It retains its argument order, floating-point scalar constraint, `bool` result,
runtime-only implementation and `noexcept`. It is O(1), deterministic and
allocation-free; no exceptions or hidden state are introduced.

Both tolerances must be finite and nonnegative (negative zero is valid).
Negative, NaN or infinite tolerance returns `false`, even for identical values
or same-sign infinities. Invalid tolerances are neither clamped nor ignored.
There is no upper bound of one on the relative tolerance.

For finite inputs, the intended real-arithmetic criterion remains

$$|a-b| \le \max(A, R\max(|a|,|b|)),$$

where `A = absoluteTolerance` has the same units as the compared values and
`R = relativeTolerance` is dimensionless. Absolute tolerance is particularly
useful near zero. This scalar API does not accept `Quantity` and adds no implicit
unit conversion or cross-dimension comparison.

The implementation avoids forming an overflowing difference or tolerance
product. After exact equality, let `h=max(|a|,|b|)>0`, `l=min(|a|,|b|)`:

- Same sign: `d=h-l` cannot overflow. Accept `d<=A` or `d/h<=R`.
- Opposite signs: the real difference is `h+l`. Its absolute criterion is
  `h<=A && l<=A-h`, without forming the sum. Its relative criterion is
  `1+l/h<=R`: for `R<1` reject; for `R==1` accept only `l==0`;
  for `R>=2` accept; otherwise compare `l/h<=R-1`.

These are algebraic rearrangements of the same finite criterion. Ratios lie in
`[0,1]`; the `R==1` endpoint is handled before division so a nonzero tiny operand
cannot disappear through ratio underflow there. Original-scale absolute checks
retain the meaning of subnormal tolerances. In particular, `max` and `-max`
have relative difference exactly two: at zero absolute tolerance, relative
tolerances `0, 0.5, 1, 1.5` reject and `2` accepts for float and double.

With **valid tolerances**, value semantics are:

| Values | Result |
| --- | --- |
| Any NaN operand | `false` |
| Same-sign infinities | `true` |
| Opposite-sign infinities, or finite vs infinity | `false` |
| Any pairing of `+0` and `-0` | `true` |
| Identical finite values | `true` |

The operation is symmetric. It is reflexive for non-NaN values under valid
tolerances, but **not transitive and not an equivalence relation**: with `A=1`
and `R=0`, zero is close to one and one to two, but zero is not close to two.

The tests assume the project's IEEE-754 binary float/double environment with
gradual underflow and the default rounding mode, without fast-math or FTZ/DAZ.
Algebraic equivalence in real arithmetic does not promise identical rounded
decisions at every tolerance boundary or in every floating-point environment.
This local remediation does not requalify the project.

---

## 9. Math Functions

`Math.h` defines the canonical `core::sqrt`. `MathFunctions.h` retains the existing
`core::Math` isolation layer: `abs`, `sin`, `cos`, `acos`, and a forwarding `sqrt`.
The existing acos tolerance-boundary behavior is unchanged by VRT-14.
No runtime code generation, intrinsic or vectorization guarantee is implied by
using a standard-library wrapper.

---

## 10. Canonical `core::sqrt` contract (VRT-14)

Include `<Vectoris/Numerics/Core/Math.h>` alone. The canonical spelling is
`vectoris::numerics::core::sqrt`; `Core::sqrt` is the same function through the
VRT-12 namespace alias. Including MathFunctions.h makes `core::Math::sqrt` /
`Core::Math::sqrt` available as compatibility-only, nondeprecated forwarders to the
fully-qualified canonical function. There is one domain/dispatch implementation;
forwarding does not use unqualified lookup and cannot recurse through aliases.

### 10.1 Scalar and special-value semantics

| Input | Result, in runtime and supported constant evaluation |
| --- | --- |
| +0 | +0 |
| -0 | -0 (sign preserved) |
| Positive finite, including every positive subnormal | Nonnegative finite square root; no collapse to zero |
| Negative finite except -0 | Quiet NaN |
| -Inf | Quiet NaN |
| +Inf | +Inf |
| NaN | NaN; no payload/sign preservation guarantee |

These are scalar IEEE/libm-compatible value semantics. The function remains
`noexcept` and returns the same scalar type, not Result. Negative-domain correction
from +0 to NaN is an intentional behavioral change, not a source-signature change.
This contract supersedes the historical negative-radicand clamping policy, which
could hide an invalid physical state as a plausible zero. No equivalence of errno,
floating-point exception flags, signaling-NaN behavior or NaN payloads is promised.

Float and double support constexpr and runtime evaluation. Long double supports
runtime evaluation through std::sqrt; its positive finite constexpr evaluation is
not guaranteed by ISO C++20. Trivial special values may still be constant-evaluable;
that does not establish a general long-double constexpr API guarantee.

### 10.2 Constant evaluation: fixed digit extraction

For binary32/64, normalize the input to a p-bit integer significand M and unbiased
exponent e (p=24/53), including subnormal normalization using integer leading-zero
counting. Define `odd` as 0/1 for e's parity and the mathematical integer
`N = M * 2^(p-1+odd)`. N can require 106 bits; the implementation streams its bit
pairs without forming a wider-than-64-bit integer.

Exactly p iterations append one square-root bit. After each step the consumed
radicand prefix equals `root^2 + remainder`, with `0 <= remainder < 2*root+1`.
Thus the final integer root is `floor(sqrt(N))`. Round up precisely when
`remainder > root`: the halfway square is `root^2 + root + 1/4`, so an integral N
cannot tie. This yields nearest rounding without Newton iteration, an empirical
convergence cutoff, an iteration-exhaustion fallback or an unproven “<=6 steps” claim.
The hard loop bound is 24 steps for float and 53 for double.

All integer arithmetic fits uint64_t: before the final step root is below 2^52
and remainder below 2^53; shifting remainder by two stays below 2^55. Input shifts
are bounded by p-1. Output reconstruction multiplies an exactly representable root
by a normal power of two. That scale's exponent lies in [-98,40] for float and
[-589,459] for double. Both the scale and all positive square-root outputs are
normal and finite. No x*x, 1/x or unchecked exponent reconstruction is used.
The former internal Detail::InitialSqrtGuess / BoundedNewtonSqrt helpers are removed;
Detail names are implementation-only and carry no compatibility promise.

### 10.3 Runtime and accuracy validation

Positive finite runtime evaluation calls std::sqrt for the deduced scalar type.
All special-value classification occurs in the canonical function before dispatch.
Runtime and constexpr paths share the same value-domain semantics; bit-for-bit
identity across toolchains, rounding modes or all inputs is not promised.

The validation environment is the project's IEEE binary32/binary64 representation,
round-to-nearest, gradual underflow and no fast-math. Boundary/subnormal cases,
every representable power of two and its neighbors, deterministic bit-pattern
samples, and materialized constexpr sample tables are compared to independent
runtime std::sqrt with an explicit **at most one ULP** criterion (the spacing from
the reference toward +Inf). The integer algorithm's nearest-rounding proof is
separate from the finite test sample; no exhaustive all-float sweep is claimed.
Long-double runtime boundaries and exponent samples use the same reference-spacing
criterion without assuming a long-double object layout. Fresh observations and
worst errors are recorded in the remediation ledger.

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
- [`tests/Core/NumericTraitsTest.cpp`](../modules/VectorisNumerics/tests/Core/NumericTraitsTest.cpp): Machine epsilon, dual-tolerance comparison.
- [`tests/Core/MathFunctionsTest.cpp`](../modules/VectorisNumerics/tests/Core/MathFunctionsTest.cpp): existing acos/abs/sin/cos regressions.
- [`SqrtContractTest.cpp`](../modules/VectorisNumerics/tests/Core/SqrtContractTest.cpp): canonical/compatibility sqrt domain, signed zero, constexpr/runtime oracles, subnormals, type constraints and Geometry long-double compatibility.
- [`tests/Core/ResultTest.cpp`](../modules/VectorisNumerics/tests/Core/ResultTest.cpp): Monadic chaining, constexpr execution, ABI triviality.
- [`tests/Core/PublicTemplateInstantiationTest.cpp`](../modules/VectorisNumerics/tests/Core/PublicTemplateInstantiationTest.cpp): Explicit template instantiation for `float` and `double`.

---

## 16. Known Deviations

1. **AML-DEVIATION-003**: Top-level namespace `vectoris::numerics::core` instead of standard-mandated `aegis::math::core` (registered in [`docs/DEVIATIONS.md`](DEVIATIONS.md)).


## Result named-factory alternative selection (VRT-03)

`Result<T,E>::success(x)` always constructs `T` at variant index 0.
`Result<T,E>::failure(x)` always constructs `E` at variant index 1.
The source argument's type, including a source convertible to both alternatives,
does not select the state. For example, `Result<int,long>::failure(3)` contains
`long(3)` as an error, and `Result<double,int>::success(3)` contains `double(3)`
as a value.

Factories use private tagged constructors that initialize `std::variant` with
`std::in_place_index<0>` or `<1>`. A private leading tag separates these internal
constructors from existing public converting constructors, including when a user
intentionally supplies an in-place-index token as a payload.

- Single-argument `success` participates only when T is constructible from the
  forwarded argument; `failure` requires E to be constructible from its argument.
- The existing variadic `success(args...)` constructs T directly, including zero
  arguments when T is default-constructible. `failure` retains its single-argument API.
- Perfect forwarding preserves mutable lvalue, const lvalue and rvalue categories.
  Neither alternative is default-constructed merely to select the other one.
- Conditional `noexcept` is exactly the corresponding
  `std::is_nothrow_constructible_v<T, Args&&...>` or E trait. A potentially throwing
  user payload constructor can propagate its exception; generic Result does not
  promise that arbitrary user payload construction is non-throwing or allocation-free.
- Move-only and non-default-constructible T/E remain supported. T==E remains
  prohibited by the existing class static assertion.
- Direct construction can remain ambiguous when a source constructs both alternatives;
  use named factories to choose the state explicitly. VRT-13 constrains assignment
  to preserve the two-state invariant; construction and forwarding semantics remain.

For every constructed success, `value_if()` is non-null and `error_if()` is null;
for every constructed failure the reverse holds. VRT-13 preserves this invariant
through all supported operations, including caught payload exceptions, as follows.

Regression evidence is in `ResultFactoryTest` within `tests/Core/ResultTest.cpp`
and the incremental remediation ledger. Project status remains
**NOT REQUALIFIED / Experimental**.


## Result exception safety and supported payloads (VRT-13)

`core::Result<T,E>` stores one `std::variant<T,E>`, with no exposed storage or
emplacement API. The canonical `core` and compatibility `Core` spellings share
this contract. VALUE and ERROR are the only states; there is no sentinel or
implicit exception-to-error conversion. `Result<T,T>` remains a compile error.

T/E must be distinct, variant-compatible object types with nonthrowing destructors.
The selected factory or constructor requires only that its selected payload can
be constructed from the forwarded arguments. Default construction is not required.
Move-only, non-default-constructible and even immovable payloads remain usable via
direct in-place construction; unavailable special members stay unavailable.

| Operation | Availability and exception guarantee |
| --- | --- |
| Factory / converting / in-place constructor | Selected payload construction; conditional `noexcept` follows that exact construction. On an exception, no destination Result exists. |
| Copy constructor | Defaulted variant copy construction. Payload exceptions propagate; no destination exists if it fails. |
| Move constructor | Defaulted variant move construction; conditional `noexcept` follows the variant. On failure, no destination exists, and the source keeps its alternative with a potentially modified payload. |
| Copy assignment | Variant must be copy assignable. Each of T/E must have nonthrowing copy construction **or** nonthrowing move construction. Unsafe overload is explicitly deleted. |
| Move assignment | Variant must be move assignable and both T/E must have nonthrowing move construction. Unsafe overload is explicitly deleted, preventing fallback to copy assignment. |
| Observers / reference accessors | `noexcept`; they do not construct or assign payloads. |
| `value_or` | Remains potentially throwing: copies/moves T or constructs a fallback. A caught exception preserves the Result's alternative; moving can modify its payload. |

Enabled assignments are defaulted; their inferred `noexcept` follows the actual
variant operation. In particular, a throwing same-alternative payload assignment
is **not** marked `noexcept`. This is a restriction on operations that can destroy
the active alternative, not a blanket ban on throwing payloads.

### Why the invariant holds

Construction starts the selected alternative or fails before a Result exists.
For cross-alternative copy assignment, nonthrowing copy construction can commit
directly. Otherwise the standard variant operation first copies a temporary;
if that copy throws the destination is unchanged, and if it succeeds the required
nonthrowing move commits it. Cross-alternative move assignment cannot throw during
construction. Same-alternative assignment does not destroy the active alternative.
By induction, every supported operation keeps exactly one alternative alive.

Cross-alternative copy-construction failure therefore provides the strong guarantee
for the destination. Same-alternative throwing assignment provides the basic
**two-state guarantee**: the original alternative remains active, but its content
may have changed according to the payload's own assignment contract. Result does
not add rollback. User payloads must obey their declared exception specifications
and normal C++ lifetime rules, including leaving a valid object after assignment
throws. No operation catches corruption and replaces it with a fabricated payload.

`has_value()`, `IsSuccess()` and bool conversion agree. `value_if()` and `error_if()`
are safe to inspect, including on moved-from Results, and exactly one is non-null.
`value()`/`Value()` require VALUE; `error()` requires ERROR; there is no `Error()`
member. Wrong-alternative caller misuse remains a Debug assertion / Release
precondition violation. Internal loss of both alternatives is prevented by the
operation constraints, so it cannot masquerade as an ERROR satisfying `!has_value()`.

### Compatibility and verification

Previously accepted unsafe assignments now fail at compile time; types declaring
a potentially throwing destructor cannot instantiate Result. Throwing construction
remains supported. Existing numerical payloads require no caller changes. Storage
has no added data members or allocation; local size/alignment observations are
recorded in the remediation ledger and are **not ABI stability guarantees**.
The wrapper cannot promise allocation-free behavior for arbitrary user payloads.

`ResultExceptionTest.cpp` exercises every assignment direction, supported throwing
transitions, moved-from observers, special-member traits and factory forwarding.
The public API manifest declares this payload matrix, with a runtime/compile-time
positive probe and controlled negative probes for unsafe assignments, throwing
destructors and the unchanged T==E prohibition. VRT-03 regressions remain intact.
VRT-13 is **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
Overall status remains **NOT REQUALIFIED / Experimental**.
