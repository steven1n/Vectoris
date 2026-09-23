# Vectoris Units Module Specification

> [!IMPORTANT]
> **Document**: Units Module Specification  
> **Document Version**: 1.0  
> **Status**: Authoritative Module Specification  
> **Reviewed Starting HEAD**: `cfbecc6390fb30d857f10f2516f39bb2ef75f996` (working tree contains uncommitted remediations)
> **Last Updated**: 2026-09-23
> **Current Qualification**: NOT REQUALIFIED / Experimental
> **Authority**: [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)

---

## 1. Purpose

The `Units` module implements a zero-overhead, compile-time dimensionally safe physical quantity system. It guarantees that incompatible physical quantities cannot be added, subtracted, or assigned to one another, while automatically deducing correct dimensional types under multiplication and division.

### Public namespace contract (VRT-12)

`vectoris::numerics::units` is the canonical public spelling.
`vectoris::numerics::Units` remains a compatibility spelling for this release,
**not deprecated**. Older `Units::` examples in this document use that spelling;
both denote identical entities. Each public header directly includes the layer's
`Units/Namespace.h`, making the alias usable from that header alone and
independent of include order. No umbrella header is required. Definitions stay
in their historical namespace; template identity, ADL and numerical behavior do
not change. Existing nested `Detail` names remain implementation-only under
both spellings, with no supported API or ABI promise. The Units header inventory
is now 31, including the authoritative
[`Namespace.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/Namespace.h).
No aliases are placed
under `vectoris::dynamics`. Overall status remains Experimental.

---

## 2. Model B Dimensional System

Vectoris adopts **Model B**, an 8-dimensional physical system where Plane Angle is treated as an independent physical base dimension ($A$) rather than a dimensionless scalar ($1$).

The 8 fundamental dimensions are:
1. **Length** ($L$)
2. **Mass** ($M$)
3. **Time** ($T$)
4. **Electric Current** ($I$)
5. **Thermodynamic Temperature** ($\Theta$)
6. **Amount of Substance** ($N$)
7. **Luminous Intensity** ($J$)
8. **Plane Angle** ($A$)

---

## 3. Base Dimensions

In [`include/Vectoris/Numerics/Units/Dimension.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/Dimension.h), dimensions are tracked as integer template parameters:

```cpp
template<
    int Length,
    int Mass,
    int Time,
    int Current,
    int Temperature,
    int Amount,
    int Luminosity,
    int Angle
>
struct Dimension;
```

Predefined base dimensions:
- `Dimensionless`: `Dimension<0,0,0,0,0,0,0,0>`
- `LengthDimension`: `Dimension<1,0,0,0,0,0,0,0>`
- `MassDimension`: `Dimension<0,1,0,0,0,0,0,0>`
- `TimeDimension`: `Dimension<0,0,1,0,0,0,0,0>`
- `CurrentDimension`: `Dimension<0,0,0,1,0,0,0,0>`
- `TempDimension`: `Dimension<0,0,0,0,1,0,0,0>`
- `AmountDimension`: `Dimension<0,0,0,0,0,1,0,0>`
- `LumDimension`: `Dimension<0,0,0,0,0,0,1,0>`
- `AngleDimension`: `Dimension<0,0,0,0,0,0,0,1>`

---

## 4. Dimension Algebra

Compile-time arithmetic on dimensions is performed through metaprogramming metafunctions:
- `DimensionAdd<D1, D2>`: Component-wise addition of dimension exponents (multiplication of quantities).
- `DimensionSubtract<D1, D2>`: Component-wise subtraction of dimension exponents (division of quantities).
- `DimensionEqual<D1, D2>`: Compile-time concept ensuring all 8 dimension exponents match identically.

---

## 5. Unit Tags

A unit tag represents a physical unit with a specific dimension and scale factor relative to SI base units:

```cpp
template <typename U>
concept IsUnitTag = requires {
    typename U::Dimension;
    typename U::Ratio;
} &&
IsStdRatio<typename U::Ratio>::value &&
requires {
    { U::IsBaseUnit } -> std::convertible_to<bool>;
};
```

---

## 6. `Quantity<T, Unit>`

`Quantity<T, Unit>` wraps a raw floating-point scalar `T` with a compile-time `Unit` tag:
- **Explicit Construction**: Direct construction from untyped scalar is `explicit`: `Meter m{10.0};`.
- **Zero Overhead**: `sizeof(Quantity<T, Unit>) == sizeof(T)`.
- **Strict Operations**:
  - Addition and subtraction require identical dimensions and identical ratios.
  - Multiplication and division generate composite derived unit tags automatically via `DerivedUnitTagImpl`.
  - Multiplication/division by dimensionless scalars preserves the unit.
  - Raw arithmetic between quantities and untyped scalars is prohibited.

---

## 7. Unit Conversion

Implicit unit conversion is strictly forbidden. Conversion across units sharing the same dimension is achieved via explicit `unit_cast`:

```cpp
template <IsUnitTag TargetUnit, typename T, IsUnitTag SourceUnit>
requires DimensionEqual<typename TargetUnit::Dimension, typename SourceUnit::Dimension>
constexpr auto unit_cast(const Quantity<T, SourceUnit>& q) noexcept;
```

---

## 8. Base Units

The `Units` module defines **8 SI Base Units** under `include/Vectoris/Numerics/Units/BaseUnits/`:

| Quantity | Base Unit | Header |
| :--- | :--- | :--- |
| Length | `Meter` | [`Length.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Length.h) |
| Mass | `Kilogram` | [`Mass.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Mass.h) |
| Time | `Second` | [`Time.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Time.h) |
| Current | `Ampere` | [`Current.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Current.h) |
| Temperature | `Kelvin` | [`Temperature.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Temperature.h) |
| Amount | `Mole` | [`Amount.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Amount.h) |
| Luminosity | `Candela` | [`Luminosity.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Luminosity.h) |
| Angle | `Radian` | [`Angle.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/BaseUnits/Angle.h) |

---

## 9. Derived Units

Under `include/Vectoris/Numerics/Units/DerivedUnits/`, **10 Derived Units** are defined:

| Quantity | Unit Name | Dimension Exponents ($L, M, T, I, \Theta, N, J, A$) | SI Unit Expression |
| :--- | :--- | :--- | :--- |
| `Velocity` | `MeterPerSecond` | $[1, 0, -1, 0, 0, 0, 0, 0]$ | $\text{m}/\text{s}$ |
| `Acceleration` | `MeterPerSecondSquared` | $[1, 0, -2, 0, 0, 0, 0, 0]$ | $\text{m}/\text{s}^2$ |
| `Force` | `Newton` | $[1, 1, -2, 0, 0, 0, 0, 0]$ | $\text{N} = \text{kg}\cdot\text{m}/\text{s}^2$ |
| `Frequency` | `Hertz` | $[0, 0, -1, 0, 0, 0, 0, 0]$ | $\text{Hz} = \text{s}^{-1}$ |
| `AngularVelocity` | `RadianPerSecond` | $[0, 0, -1, 0, 0, 0, 0, 1]$ | $\text{rad}/\text{s}$ |
| `AngularAcceleration` | `RadianPerSecondSquared` | $[0, 0, -2, 0, 0, 0, 0, 1]$ | $\text{rad}/\text{s}^2$ |
| `Torque` | `NewtonMeterPerRadian` | $[2, 1, -2, 0, 0, 0, 0, -1]$ | $\text{N}\cdot\text{m}/\text{rad}$ |
| `MomentOfInertia` | `KilogramMeterSquaredPerRadianSquared` | $[2, 1, 0, 0, 0, 0, 0, -2]$ | $\text{kg}\cdot\text{m}^2/\text{rad}^2$ |
| `Power` | `Watt` | $[2, 1, -3, 0, 0, 0, 0, 0]$ | $\text{W} = \text{J}/\text{s}$ |
| `AngularMomentum` | `AngularMomentumUnit` | $[2, 1, -1, 0, 0, 0, 0, -1]$ | $\text{kg}\cdot\text{m}^2/(\text{rad}\cdot\text{s})$ |

---

## 10. Rotational Dimensional Model

### 10.1 Compensating Inverse-Angle Exponents
To maintain dimensional consistency across dynamics, rotational work, kinetic energy, and Newton-Euler angular acceleration, rotational mechanical quantities carry compensating inverse-angle exponents:
1. **Angular Velocity ($\boldsymbol{\omega}$)**: $[A \cdot T^{-1}]$ ($\text{rad}/\text{s}$)
2. **Angular Acceleration ($\boldsymbol{\alpha}$)**: $[A \cdot T^{-2}]$ ($\text{rad}/\text{s}^2$)
3. **Moment of Inertia ($I$)**: $[M \cdot L^2 \cdot A^{-2}]$ ($\text{kg}\cdot\text{m}^2/\text{rad}^2$)
4. **Torque ($\boldsymbol{\tau}$)**: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$ ($\text{N}\cdot\text{m}/\text{rad}$)
5. **Rotational Kinetic Energy ($E = \frac{1}{2} I \omega^2$)**:
   $$[E] = [M \cdot L^2 \cdot A^{-2}] \cdot [A^2 \cdot T^{-2}] = [M \cdot L^2 \cdot T^{-2} \cdot A^0] \equiv \text{Joule}$$
6. **Newton-Euler Rotational Law**:
   $$[I \boldsymbol{\alpha}] = [M \cdot L^2 \cdot A^{-2}] \cdot [A \cdot T^{-2}] = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] \equiv [\boldsymbol{\tau}]$$

### 10.2 Dimensional Separation of Torque and Energy
Under standard 7D SI, Torque and Energy share identical units ($\text{N}\cdot\text{m} \equiv \text{J}$). Under Model B:
- **Energy / Work**: $[M \cdot L^2 \cdot T^{-2} \cdot A^0]$
- **Torque**: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$
This prevents accidental assignment of work or energy to torque at compile time.

### 10.3 Rotational Lie Bracket & Gyroscopic Cross Coupling
In the Euler rotational equation, the gyroscopic term is $\boldsymbol{\omega} \times \mathbf{L}$.
Computing dimensions:
$$[\boldsymbol{\omega} \times \mathbf{L}] = [A \cdot T^{-1}] \cdot [M \cdot L^2 \cdot A^{-1} \cdot T^{-1}] = [M \cdot L^2 \cdot T^{-2} \cdot A^0] \equiv \text{Energy}$$
The ordinary Cartesian cross product yields Energy, not Torque. To resolve this, Vectoris provides:
$$\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L}) \triangleq \frac{\boldsymbol{\omega} \times \mathbf{L}}{1\text{ rad}}$$
$$[\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L})] = \frac{[M \cdot L^2 \cdot T^{-2} \cdot A^0]}{[A^1]} = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] \equiv [\boldsymbol{\tau}]$$
Calling `LieBracket(omega, L)` or `RotationalCross(omega, L)` explicitly performs this normalization. Do not extract `.value()` to bypass dimensional cross-product algebra. Scalar access remains appropriate for finite checks, same-unit numerical solver adapters that reify their result type, and dimensionless values after an explicit typed normalization.

### 10.4 Generic Cross Versus Physical Rotational Cross
The generic Dynamics `Cross(a, b)` follows ordinary dimensional multiplication and preserves the `Frame` type. For example:
$$[\boldsymbol{\omega} \times \mathbf{v}] = [A \cdot T^{-1}][L \cdot T^{-1}] = [A \cdot L \cdot T^{-2}]$$
This is not `Acceleration`. The physical rotating-frame transport term uses the radian coordinate convention explicitly:
$$\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{v}) \triangleq \frac{\boldsymbol{\omega} \times \mathbf{v}}{1\text{ rad}}$$
$$[\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{v})] = [L \cdot T^{-2}]$$
The divisor is a typed `Radian` quantity with numeric value one, so this dimensional conversion does not introduce a numerical scale factor. Use `Cross` for ordinary algebra; use `RotationalCross` only for physical rotational equations whose angular-coordinate convention consumes one radian. The implementation currently supports the production transport term `(AngularVelocity3, Velocity3)` and gyroscopic term `(AngularVelocity3, AngularMomentum3)`; it does not expose unused position or angular-acceleration combinations.

---

## 11. Frequency Contract

[`include/Vectoris/Numerics/Units/DerivedUnits/Frequency.h`](../modules/VectorisNumerics/include/Vectoris/Numerics/Units/DerivedUnits/Frequency.h) specifies:
- Dimension: `FrequencyDimension = Dimension<0, 0, -1, 0, 0, 0, 0, 0>;` ($T^{-1}$).
- Unit Tag: `struct HertzUnit { using Dimension = FrequencyDimension; using Ratio = std::ratio<1>; static constexpr bool IsBaseUnit = false; };`.
- Quantity Aliases: `using Frequency = Quantity<Scalar, HertzUnit>;` and `using Hertz = Frequency;`.
- Dimensional Algebra:
  - $\text{Frequency} \times \text{Time} \implies$ result dimension is `Dimensionless` ($[L^0 M^0 T^0 I^0 \Theta^0 N^0 J^0 A^0]$):
    $$\text{Hertz}(50.0) \times \text{Second}(0.1) \implies \text{numerical scalar } 5.0 \text{ with dimensionless compound unit tag}$$
  - $\text{Dimensionless} / \text{Time} \implies$ result dimension is `FrequencyDimension` ($[T^{-1}]$):
    $$5.0 / \text{Second}(0.1) \implies \text{Quantity}<\text{Scalar}, \text{HertzUnit}>(50.0)$$

---

## 12. C++ Object-Representation Checks

All exported quantity types must satisfy `Detail::ValidateQuantityABI<Q>()`:
- `sizeof(Quantity<T, Unit>) == sizeof(T)`
- `alignof(Quantity<T, Unit>) == alignof(T)`
- `std::is_standard_layout_v<Quantity<T, Unit>> == true`
- `std::is_trivially_copyable_v<Quantity<T, Unit>> == true`

These compile-time checks confirm the listed properties for the instantiated `Quantity` type under the current compiler. The historical validator names containing `ABI` are retained for source compatibility; they do not establish a C ABI, cross-build binary compatibility, DMA suitability, persistent representation, or network wire format. Serialization and device transfer require separately specified encodings and alignment contracts.

---

## 13. Failure / Compile-Time Safety

- Compile errors are triggered immediately if:
  - Two quantities of differing dimensions are added or subtracted (`meter + second`).
  - An untyped raw scalar is assigned to a `Quantity` without explicit constructor invocation.
  - A quantity is cast to an incompatible dimension (`unit_cast<SecondUnit>(meter)`).
  - Torque is assigned from translational energy without explicit conversion.

---

## 14. Verification Evidence

- [`tests/Units/UnitsTest.cpp`](../modules/VectorisNumerics/tests/Units/UnitsTest.cpp):
  - `CRTPABIAndZeroInit`: ABI size, alignment, trivial copyability.
  - `StrictTypeConceptAndCast`: Concept checks, velocity derivation from length/time.
  - `RotationalAndInertiaUnits`: Torque, moment of inertia, power, angular momentum dimensional algebra.
  - `FrequencyCompileTimeContractAndAlgebra`: Hertz dimension exponents, ABI validation, frequency-time algebra, and exclusion of duplicate `NewtonUnit` symbol collision.
- [`tests/Units/PublicTemplateInstantiationTest.cpp`](../modules/VectorisNumerics/tests/Units/PublicTemplateInstantiationTest.cpp): Instantiation tests across `float` and `double`.
