# Technical Debt Registry

> [!WARNING]
> **Status**: DEPRECATED / HISTORICAL (Non-Authoritative)  
> This file contains pre-governance technical debt tracking notes. Authoritative compliance findings and qualification debt are formally tracked in:
> - [`docs/audits/AegisMathLib_Compliance_Audit_v1.md`](docs/audits/AegisMathLib_Compliance_Audit_v1.md)
> - [`docs/audits/AegisMathLib_Stable_Core_Qualification_v1.md`](docs/audits/AegisMathLib_Stable_Core_Qualification_v1.md)
> - [`docs/DEVIATIONS.md`](docs/DEVIATIONS.md)


---

# TD-001

## Ratio Overflow Protection

Status:

PARTIAL


Description:

Current std::ratio based implementation depends on intmax_t range.

Large composite units may exceed limits.


Impact:

Medium


Plan:

Implement custom compile-time rational arithmetic.


Priority:

HIGH



---

# TD-002

## SIMD Backend

Status:

OPEN


Description:

Vector and Matrix classes currently have no SIMD abstraction layer.


Potential solutions:

- SSE2
- AVX2
- NEON


Priority:

MEDIUM


---

# TD-003

## Exception Policy

Status:

OPEN


Description:

Define global error handling strategy.

Options:

- No exceptions
- Expected<T,E>
- Error codes


Priority:

HIGH


---

# TD-004

## Serialization Contract

Status:

OPEN


Description:

Need formal binary serialization specification.


Requirements:

- Endianness
- Versioning
- Alignment
- Compatibility


Priority:

HIGH


---

# TD-005

## Static Analysis Compliance

Status:

OPEN


Required:

- clang-tidy
- MISRA checking
- Compiler warnings as errors


Priority:

MEDIUM


---

# TD-006

## Documentation Coverage

Status:

OPEN


Need:

- API documentation
- Architecture diagrams
- Usage examples


Priority:

LOW