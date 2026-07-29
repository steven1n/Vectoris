# AegisMathLib

## Overview

AegisMathLib is a high-performance, strongly typed mathematical foundation library designed for aerospace, defense, simulation, robotics and scientific computing applications.

The primary design goals are:

- Zero-overhead abstraction
- Compile-time dimensional safety
- Deterministic numerical behavior
- ABI stability
- MISRA C++ / DO-178C oriented architecture principles
- Modern C++20 implementation

AegisMathLib provides the mathematical foundation layer for higher-level systems including:

- Coordinate transformation systems
- Navigation algorithms
- Radar simulation
- Guidance and control systems
- Physics simulation frameworks


---

# Current Status

## Version

AegisMathLib Units System v1.0 RC1
Revision: B.2.5
Status: Frozen Candidate

The Units subsystem has completed architecture review and entered freeze state.


---

# Core Features

## 1. Strongly Typed Physical Quantities

AegisMathLib prevents invalid physical operations at compile time.

Example:

```cpp
Meter distance{100.0};
Second time{5.0};

auto velocity = distance / time;
The resulting type:
Quantity<double, MeterPerSecondUnit>
is generated automatically.
Invalid operations are rejected:
Meter distance;
Second time;

auto invalid = distance + time; // compile error
Units System Architecture
The Units subsystem provides:
SI Dimension Model
Supported dimensions:
Length
Mass
Time
Electric Current
Temperature
Amount of Substance
Luminous Intensity
Angle
Example:
using VelocityDimension =
    Dimension<1,0,-1,0,0,0,0,0>;
Quantity Type
Core abstraction:
template<
    Concepts::FloatingPoint T,
    IsUnitTag Unit
>
class Quantity;
Properties:
No implicit unit conversion
No hidden runtime overhead
Strong compile-time checking
Trivial memory layout
ABI Contract
Every exported physical quantity must satisfy:
static_assert(
    Detail::ValidateQuantityABI<T>()
);
ABI guarantees:
sizeof(Unit) == sizeof(Scalar)
alignof(Unit) == alignof(Scalar)
std::is_standard_layout_v
std::is_trivially_copyable_v
This allows safe:
Binary serialization
DMA transfer
Shared memory communication
Network packet structures
Supported Units
Base Units
Quantity	Type
Length	Meter
Time	Second
Mass	Kilogram
Angle	Radian
Temperature	Kelvin
Current	Ampere
Amount	Mole
Luminosity	Candela

Derived Units
Currently implemented:
Quantity	Type
Velocity	Meter/Second
Acceleration	Meter/Second²
Force	Newton
Frequency	Hertz

Design Principles
Compile Time First
Errors should be detected by the compiler whenever possible.
Example:
Wrong:
distance + time;
will fail during compilation.
No Hidden Conversion
Implicit conversion is forbidden.
This is intentional.
Conversion must be explicit:
auto km =
unit_cast<KilometerUnit>(meter);
Requirements
Compiler
Minimum:
C++20
Recommended:
GCC 13+
Clang 16+
MSVC 2022+
Build
Example:
mkdir build
cd build

cmake ..
cmake --build .
Roadmap
Phase 1
Completed:
Dimension system
Unit traits
Quantity system
ABI contract
Unit conversion
Phase 2
Next:
Vector3
Matrix
Quaternion
Coordinate Frame System
Phase 3
Future:
Navigation mathematics
Aerospace reference frames
Guidance algorithms
License
TBD

---

# CHANGELOG.md

```markdown
# Changelog

All notable changes to AegisMathLib are documented here.


# [1.0.0-RC1] - Revision B.2.5

Status:
Release Candidate


## Added

### Units System

- Added strongly typed Quantity system
- Added compile-time dimension checking
- Added SI base dimension model
- Added derived unit generation


### ABI Contract System

Added:

Units/Detail/ABI.h

Features:

- Standard layout validation
- Trivially copyable validation
- Size equality validation
- Alignment validation


### Unit Conversion

Added:

unit_cast()

Features:

- Explicit conversion only
- Compile-time dimension validation
- Ratio based conversion


---

## Changed

### Header Architecture

Before:

Common.h
 |
 Quantity
 |
 Conversion

After:

Core.h
Quantity.h
Detail/
 └── ABI.h


Improved:

- Dependency isolation
- Compilation performance
- Module boundaries


---

## Fixed

### ABI Validation

Fixed:

- Constructor based ABI checks
- Lazy template validation
- Missing derived unit validation


ABI checks are now explicitly bound to exported units.


---

## Security / Reliability Improvements

Added protection against:

- Accidental implicit float promotion
- Invalid unit operations
- Unsafe memory layouts
- Hidden conversion


---

# Previous Versions

None.