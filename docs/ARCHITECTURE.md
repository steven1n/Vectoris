# AegisMathLib Architecture Specification

> [!WARNING]
> **Status**: DEPRECATED / HISTORICAL (Non-Authoritative)  
> This legacy document has been formally superseded by the authoritative project specifications:
> - Normative Baseline (SSOT): [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)
> - Cross-Module Conventions: [`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md)
> - Subsystem Specifications: [`docs/core.md`](core.md), [`docs/units.md`](units.md), [`docs/geometry.md`](geometry.md), [`docs/dynamics.md`](dynamics.md)
> 
> This document is preserved for historical reference only and carries zero normative authority.

Version:
1.0

Architecture Revision:
B.2.5

Historical Status:
Foundation Freeze Candidate (Superseded)


---

# 1. Purpose

AegisMathLib is a deterministic, strongly typed mathematical foundation library designed for:

- Aerospace simulation
- Navigation systems
- Radar and sensor simulation
- Robotics
- Scientific computing
- High reliability embedded systems


The architecture prioritizes:

1. Compile-time correctness
2. Deterministic execution
3. Zero-overhead abstraction
4. Stable memory layout
5. Long-term maintainability


---

# 2. Architecture Philosophy


## 2.1 Compile-Time First Design


The library follows:

Detect errors at compile time
before runtime

Priority:

Compile Error
>
Static Analysis Warning
>
Runtime Assertion
>
Runtime Error


Examples:

Invalid:

```cpp
Meter + Second
must fail during compilation.
3. Layer Architecture
AegisMathLib is divided into independent layers:
AegisMathLib

|
+-- Core
|
+-- Units
|
+-- Algebra
|
+-- Geometry
|
+-- Coordinate
|
+-- Physics
|
+-- Algorithms
Dependency direction:
Algorithms

    |
    v

Physics

    |
    v

Coordinate

    |
    v

Geometry

    |
    v

Algebra

    |
    v

Units

    |
    v

Core
Higher layers may depend on lower layers.
Lower layers MUST NOT depend on higher layers.
4. Core Layer
Location:
include/AegisMath/Core/
Responsibilities:
Fundamental concepts
Numeric traits
Precision policy
Compiler configuration
Core MUST NOT contain:
Physical units
Geometry
Coordinate systems
5. Units Layer
Location:
include/AegisMath/Units/
Purpose:
Provide dimensional safety.
Architecture:
Units

|
+-- Dimension
|
+-- UnitTraits
|
+-- Quantity
|
+-- Conversion
|
+-- Detail
5.1 Dimension System
AegisMathLib uses an extended SI dimension model.
Supported dimensions:
Length
Mass
Time
Current
Temperature
Amount
Luminosity
Angle
Representation:
Dimension<
    L,
    M,
    T,
    I,
    Θ,
    N,
    J,
    A
>
5.2 Quantity System
Core type:
Quantity<T, Unit>
Design requirements:
Trivial layout
No virtual functions
No dynamic memory
constexpr capable
Memory model:
Quantity<double, Meter>

sizeof == sizeof(double)
5.3 ABI Contract
Every public physical quantity must satisfy:
standard_layout

+

trivially_copyable

+

same size as scalar

+

same alignment as scalar
Validation:
static_assert(
    Detail::ValidateQuantityABI<T>()
);
Purpose:
Provide source-level properties useful when defining an independently verified:
Serialization
DMA
Shared memory

These C++ type traits alone do not define a cross-language ABI, serialization schema,
DMA descriptor, or persistent/wire format. Each external representation requires its
own explicit mapping and platform validation.
Network transport
6. Algebra Layer
Location:
include/AegisMath/Algebra/
Contains:
Vector

Matrix

Quaternion

Tensor
Requirements:
Fixed memory layout
SIMD compatible
constexpr operations
Unit aware
Example:
Vector3<Meter>
is valid.
Vector3<Meter>
+
Vector3<Second>
is forbidden.
7. Geometry Layer
Purpose:
Provide mathematical geometric objects.
Contains:
Point

Line

Plane

Transform

Rotation
Rules:
Point and Vector are different concepts.
Forbidden:
Point + Point
Allowed:
Point + Vector
8. Coordinate Layer
Location:
Coordinate/
Purpose:
Provide reference frame management.
Examples:
ECEF

ECI

ENU

NED

Body Frame

Sensor Frame
Coordinate transformations must explicitly declare:
Source frame
Target frame
Rotation
Translation
Example:
Transform<
    ECEF,
    BodyFrame
>
9. Physics Layer
Contains:
Kinematics

Dynamics

Atmosphere

Gravity

Ballistics
Depends on:
Units
Algebra
Coordinate
10. Design Restrictions
The following are forbidden:
Dynamic allocation
No:
new
malloc
shared_ptr
in mathematical core.
Runtime polymorphism
Forbidden:
virtual
inside core math types.
Reason:
unpredictable ABI
memory overhead
cache inefficiency
Hidden conversions
Forbidden:
double -> Meter
must require:
Meter{value}
11. Future Extension Policy
New modules must provide:
Architecture document

Unit tests

ABI tests

Benchmark results

before entering main branch.

---
