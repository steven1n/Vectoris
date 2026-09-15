# AegisMathLib C++ Coding Standard

> [!WARNING]
> **Status**: DEPRECATED / HISTORICAL (Non-Authoritative)  
> This legacy document has been formally superseded by the authoritative project specifications:
> - Normative Baseline (SSOT): [`docs/ENGINEERING_STANDARD_V1.md`](ENGINEERING_STANDARD_V1.md)
> - Cross-Module Conventions: [`docs/MATHEMATICAL_CONVENTIONS.md`](MATHEMATICAL_CONVENTIONS.md)
> - Subsystem Specifications: [`docs/core.md`](core.md), [`docs/units.md`](units.md), [`docs/geometry.md`](geometry.md), [`docs/dynamics.md`](dynamics.md)
> 
> This document is preserved for historical reference only and carries zero normative authority.

Version:
1.0 (Superseded)

Based on (Historical):
- C++20
- MISRA C++ principles
- Safety critical software practices


---

# 1. General Rules


## Compiler Standard


Required:

C++20


Compiler warnings:


-Wall
-Wextra
-Werror


Warnings are treated as errors.


---

# 2. Naming Convention


## Types


PascalCase:


```cpp
class Quantity;

struct MeterUnit;
Functions
camelCase:
unitCast();

validateABI();
Constants
UPPER_CASE:
MAX_PRECISION;
Template Parameters
Single uppercase:
template<typename T>
or descriptive:
template<typename Scalar>
3. Header Rules
Every header:
#pragma once
Required.
Include order:
// C++ standard
#include <type_traits>

// AegisMath Core
#include "Core.h"

// Local
#include "Quantity.h"
4. Class Design Rules
Mathematical Types
Must be:
final
trivially copyable
standard layout
Example:
class Vector3 final
{
};
Constructors
Prefer:
constexpr
noexcept
explicit
Example:
constexpr explicit Quantity(
    double value
) noexcept;
5. Memory Rules
Forbidden:
virtual
new
delete
malloc
inside core math modules.
Allowed:
std::array
std::span
6. Exception Policy
Current policy:
No exceptions
Mathematical operations should:
return valid objects
use compile-time constraints
use explicit error types where required
7. constexpr Policy
Pure mathematical operations should be:
constexpr
Example:
constexpr auto norm() const noexcept;
8. Floating Point Policy
Floating point comparisons:
Forbidden:
a == b
Use:
AlmostEqual()
9. Unit Safety Rules
Physical values MUST use Units.
Forbidden:
double velocity;
Preferred:
Velocity velocity;
10. Template Rules
Templates must:
have clear constraints
avoid SFINAE complexity
prefer concepts
Example:
Preferred:
template<FloatingPoint T>
Avoid:
enable_if
11. Testing Requirements
Every module requires:
Compile Tests
Verify:
invalid code fails
valid code compiles
Runtime Tests
Verify:
numerical correctness
edge cases
ABI Tests
Verify:
sizeof

alignof

trivially_copyable

standard_layout
12. Documentation Requirement
Public types require:
Purpose
Mathematical definition
Unit convention
Usage example
13. Review Requirement
Before merging:
Required approval:
Architecture Review

+

Code Review

+

Test Review
End of Standard

---