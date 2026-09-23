# Vectoris red-team remediation — 2026-09-20

## Scope and status

This is an incremental remediation ledger for the independent audit of
`cfbecc6390fb30d857f10f2516f39bb2ef75f996`. It does not replace the audit or the
Engineering Standard. The working-tree changes below are not a new qualification
baseline. Overall qualification remains **NOT REQUALIFIED / Experimental**.

| Finding | Current status |
| --- | --- |
| VRT-01 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-02 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-03 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-04 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-05 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-06 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-07 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-08 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-09 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-10 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-11 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-12 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-13 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-14 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-15 | Implemented locally; CI / independent review pending; NOT CLOSED |
| VRT-16 through VRT-19 | OPEN; not remediated in this step |

The current incremental step stops after VRT-15 implementation and local
verification. All historical step entries below are preserved; their counts and
stop conditions refer to those steps.

## Step 1 — VRT-01

### Requirements and mathematical model

- Follow SSOT §§15–17, 22–24, 44–48, 80–84, 89 and 124.
- Quaternion components are dimensionless; source and destination Frame tags
  remain part of both quaternion and returned rotation types.
- Normalize every finite, nonzero float/double quaternion without squaring its
  original magnitude. Preserve canonicalization and the represented orientation.
- Reject all-zero input with `zero_norm`, and NaN/Inf with `non_finite_input`.
- Matrix conversion must report numerical failure through `Result`, including
  public-member mutation and accumulated composition drift. It must not unwrap
  a failed internal result or silently substitute an identity rotation.

For `s = max(|w|, |x|, |y|, |z|) > 0`, let `u = q/s`. At least one component
has absolute value one and all have absolute value at most one, so
`1 <= dot(u,u) <= 4`. Compute `u/sqrt(dot(u,u))`; never reconstruct `s*norm(u)`
and never form `1/s`. Thus both largest finite and smallest subnormal magnitudes
are supported under gradual underflow. Relative components smaller than the
representation permits can round to zero. Runtime/storage complexity is O(1),
without allocation, iteration, randomness or hidden state.

### Implementation and API compatibility

- `Quaternion::TryCreate`: scale before computing the norm; exact zero-input
  classification replaces the old absolute squared-norm cutoff.
- `Quaternion::ToRotationMatrix`: return
  `Result<RotationMatrix3<T, FromFrame, ToFrame>, MathError>`. Check non-finite
  components and finite out-of-range components before matrix arithmetic, then
  propagate the rotation factory's validation result.
- `RotationMatrix3::FromQuaternion`: propagate the same Result.
- Update all repository conversion call sites to check success before access.
- Document error behavior, numerical assumptions and migration in `docs/geometry.md`.

**Source compatibility:** both conversion methods now return Result. External
callers must migrate. Object storage/layout and Frame mapping are unchanged.
Public quaternion fields remain mutable; other quaternion operations still
require valid unit inputs. This step does not resolve the separate public API,
ABI documentation or naming findings.

### Regression and verification

The new scale regression failed on the old implementation for both float and
double before the production change. Tests now cover minimum subnormal, minimum
normal, small, ordinary and maximum finite scales, signed inputs, analytic
normalized components, unit norm, a known 120-degree rotation, determinant,
all four NaN/Inf input positions, zero and invalid mutable states. A 100,000-step
rotation test checks safe conversion and explicit renormalization against an
independent sine/cosine oracle; it does not require identical drift across compilers.

Local environment: AppleClang 21.0.0, ISO C++20, x86_64 Darwin, CMake 4.4.3,
GoogleTest v1.14.0. Required project warning flags remain enabled, including
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.

| Check | Result |
| --- | --- |
| Debug suite | 141/141 passed |
| Release suite | 139/139 passed; two assertion-only tests excluded by design |
| ASan + UBSan suite | 141/141 passed with fail-fast runtime options |
| Public-header isolation and order tests | Both module targets built successfully |
| clang-tidy | 97 translation units passed with explicit repository configuration; final changed test rechecked |
| Compiler warnings / unsuppressed static diagnostics | 0 |
| Fresh LLVM function coverage | 162/162, 100% of emitted function set |
| Fresh LLVM line coverage | 866/873, 99.20% |
| Fresh raw LLVM branch coverage | 322/354, 90.96%; unchanged thresholds, gate passed |
| Whitespace validation | `git diff --check` passed |

Sanitizer environment was `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. This independently compensates
for the still-open CI configuration finding; it does not fix VRT-05.
Clang-tidy required the repository harness's macOS SDK/target arguments plus an
explicit `--config-file`. The initial invocation without platform arguments
failed tool setup; it is not counted as a successful analysis.

A local optimized smoke benchmark measured normalization plus checked conversion:
600,000 calls per precision per run, five runs, six scales from denormal to max,
and an observable checksum. Float ranged 44.38–49.11 ns/call; double
45.76–49.07 ns/call. This is an environment-specific measurement, not a speedup,
latency guarantee or replacement for whole-module benchmark qualification.

### Remaining gates

No commit or push was made. GCC, LLVM Clang and MSVC CI have not run on these
working-tree changes; the old audit SHA's successful jobs do not validate this
patch. Independent reviewer approval and complete project requalification remain
pending. Coverage surface completeness and the other 18 findings remain open.


## Step 2 — VRT-02 only

### Starting baseline and preservation

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before editing, captured `git status`, `git rev-parse HEAD`, `git diff`, a file
snapshot and SHA-256 inventory of the existing VRT-01 working tree.

VRT-01 preservation: **YES**. Quaternion.h, RotationMatrix3.h,
AttitudeEngineTest.cpp and GeometryComparisonTest.cpp are byte-for-byte unchanged
from that snapshot. Existing geometry documentation and the Step 1 ledger body
are preserved; this step appends the UnitVector3 contract and updates the current
status table. No discard, commit, push, rebase or history rewrite occurred.

### Old implementation reproduced before production edits

The same standalone ISO C++20 program tested float and double single-axis inputs:

| Input | float before | double before |
| --- | --- | --- |
| max()/4 | 8.50705867e+37: success, (0,0,0), IsValid=false | 4.4942328371557893e+307: success, (0,0,0), IsValid=false |
| min normal | 1.17549435e-38: zero_norm | 2.2250738585072014e-308: zero_norm |
| min subnormal | 1.40129846e-45: zero_norm | 4.9406564584124654e-324: zero_norm |
| 1e-5 | 9.99999975e-06: zero_norm | 1.0000000000000001e-05: success, x=0.99999999999999989, IsValid=true |
| 1 | success, (1,0,0), IsValid=true | success, (1,0,0), IsValid=true |
| 0 | zero_norm | zero_norm |

The initial numerical regression suite additionally failed 10 of 14 tests against
the unmodified production header. Production edits began only after the original
standalone reproducer was confirmed. After correction, every listed nonzero
single-axis case returns (1,0,0), IsValid=true; zero remains zero_norm.

### Requirements, mathematical model and errors

SSOT §§15–17, 22, 29–34, 40, 44–48, 57, 80–86, 89–90, 117 and 124 apply.
Direction components are dimensionless and retain an explicit Frame. In the
common floating-point type of storage T and input U:

```
s = max(abs(x), abs(y), abs(z))
ux = x/s; uy = y/s; uz = z/s
n = sqrt(ux*ux + uy*uy + uz*uz)
u = (ux/n, uy/n, uz/n)
```

Input finiteness guarantees finite s; exact all-zero input alone gives s=0.
For all other finite inputs s>0, each scaled magnitude is <=1, at least one is 1,
and the scaled squared norm lies in [1,3]. There is no unscaled squared norm,
no reconstructed original magnitude, no reciprocal of s and no absolute small
norm threshold. Runtime and storage are O(1), without allocation or iteration.

- Any NaN/±Inf component: `non_finite_input`.
- Exact all-zero input, including signed zero combinations: `zero_norm`.
- Finite nonzero input: normalization attempt, then conversion to T.
- Stored candidate failing the finite unit-norm check: `normalization_failure`.
- Success: stored components finite, norm approximately one, `IsValid()==true`.

IsValid uses safe `std::hypot`, requires a finite result and checks unit length
with absolute/relative tolerance 10*epsilon(T). It no longer squares unchecked
magnitudes. The normalization_failure and non-finite postcondition branches are
defensive and were not artificially reached by corrupting private object bytes.
Those uncovered raw branches remain in the coverage denominator.

### Encapsulation audit and API compatibility

Two mutable paths were confirmed against the old implementation:

1. A GeometryTraits specialization for an application-owned Frame could obtain
   `double&` to a private component via friendship. The executable changed a
   valid direction to zero. The same source is now rejected for private access.
2. A user-defined scalar could bind a const reference to an internal component
   during multiplication, legally modify the underlying non-const object and
   destroy its norm. A standalone old/new probe confirms old (0,0), invalid vs
   new (0.6,0.8), valid. Mixed dot operations had the same reference exposure.

Remove privileged traits friendship. Scalar-left multiplication is an ordinary
namespace function using value accessors. Member scalar multiplication and mixed
Vector3 dot products pass values to user-defined arithmetic. Compile-time traits
specialization tests cannot write x_/y_/z_; x()/y()/z()/getX()/getY()/getZ() all
return values; ToVector returns a detached value.

Default/direct/tag construction remains inaccessible. Copy/move construction and
assignment are trivial scalar copies and preserve a valid source representation.
Unary negation preserves norm. Both factory variants enforce the postcondition;
the bool output form changes output only after success. Addition, subtraction and
scalar products return ordinary Vector3, never an unchecked UnitVector3.
No remaining mutable escape was found through the supported library API. Bytewise
fabrication or replacing the library's class definition is outside this contract.

Public signatures for normal float/double factory/accessor use are unchanged.
UnitVector3 storage T and factory input U now explicitly require floating-point
types. Integer/custom storage is rejected, rather than permitting an unrepresentable
unit direction. Ordinary scalar-left multiplication syntax is unchanged; its
implementation is no longer a hidden friend. Private traits access is intentionally
removed, while observable layout checks and public trait aliases remain.

### Cross-precision and numerical limits

Float -> double calculates at double precision. Double -> float normalizes at
double precision before narrowing; raw input magnitude outside float range is
therefore supported. Both conversion directions are tested from minimum subnormal
through huge inputs. A double component too small to represent as a normalized
float may round to zero; a dedicated test records this. The stored float unit-norm
postcondition is still required. No exact component preservation outside T's
representable precision/range is promised.

Positive scaling tests use direction (1,-2,3) at seven finite positive scales:
minimum subnormal, minimum normal, 1e-5, 1, 10, sqrt(max), max/4, in both precisions.
Input components remain representable; rounded/overflowed/underflowed external
inputs cannot be reconstructed. Gradual underflow is assumed; FTZ/DAZ and fast-math
are not qualified. Long double remains permitted but is not separately qualified
by the float/double regression matrix.

### Tests, independent oracles and layout

Added 16 deterministic tests (eight per-precision/cross-precision pairs in total):
signed axes from smallest subnormal to maximum finite, mixed huge/tiny components,
wide magnitude separation, negatives, all NaN/±Inf positions, scale invariance,
public operations, output failure preservation, read-only access, custom scalar
reference isolation and precision conversion. Oracles use analytic axes,
(1,-2,3)/sqrt(14), and independent exponent rescaling plus long-double library
hypot, including portability to implementations where long double equals double.

| Type | standard-layout | trivially-copyable | sizeof | alignof |
| --- | --- | --- | --- | --- |
| UnitVector3<float, Frame> | true | true | 12 bytes | 4 bytes |
| UnitVector3<double, Frame> | true | true | 24 bytes | 8 bytes |

These local x86_64 observations match the pre-patch results. They are **not ABI
stability** or wire-format guarantees.

### Final local verification

AppleClang 21.0.0, ISO C++20, x86_64 Darwin, CMake 4.4.3; required warning policy
unchanged. Existing out-of-tree configured builds use GoogleTest v1.14.0 and the
same explicit system-include ordering established in Step 1.

| Check | Result |
| --- | --- |
| Debug | 157/157 passed |
| Release | 155/155 passed (two assertion-only tests excluded) |
| ASan+UBSan | 157/157 passed, fail-fast environment enabled |
| Numerics public-header isolation | 55 standalone headers + 2 order TUs built |
| Dynamics public-header isolation | 11 standalone headers + 2 order TUs built |
| clang-tidy | 98/98 TUs succeeded; explicit repository config and macOS SDK/target arguments |
| Project compiler warnings / unsuppressed static diagnostics | 0 / 0 |
| Fresh raw function coverage | 171/171 = 100% |
| Fresh raw line coverage | 894/903 = 99.0033% |
| Fresh RAW LLVM branches | 325/360 = 90.2778% |
| Coverage gate | PASS at unchanged function=100%, line>=95%, RAW branch>=90% thresholds |
| git diff --check | passed |

Sanitizer environment: `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. No sanitizer diagnostics.
**VRT-05 remains OPEN**; this local execution does not fix the CI failure gate.

Fresh LLVM coverage cleaned the profile directory and merged a new profile.
No branch-denominator subtraction was used for the normative result. An earlier
run exposed two unexecuted instantiated +/- operations; property tests now execute
those operations. No coverage exclusions or threshold changes were introduced.
Function coverage describes the emitted function set, not every public template
instantiation. **VRT-10 remains OPEN**.

No optional before/after performance benchmark was run for VRT-02; no performance
claim is made. GCC/LLVM/MSVC CI and independent review are pending. Evidence includes
baseline snapshots, red/green reproducer outputs, access-escape probes, test/static
logs, coverage summary, final diff and checksums in the supplied Step 2 artifact.

### Stop condition

VRT-01: **Implemented; requalification still pending**.
VRT-02: **Implemented locally; CI and independent review pending; NOT CLOSED**.
VRT-03 through VRT-19: **OPEN**. None was remediated in this step.
Overall status: **NOT REQUALIFIED / Experimental**.
No commit, push or automatic continuation to VRT-03.


## Step 3 — VRT-03 only

### Baseline and scope

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Captured git status, HEAD, full diff and SHA-256/file snapshots before editing.
All pre-existing VRT-01 and VRT-02 code, tests, CMake and geometry documentation
remain byte-for-byte unchanged. The ledger's current-status header is updated
and this record appended; the earlier step bodies are preserved verbatim.
No compatibility adjustment to either prior patch was needed.

This step changes only named Result factory construction, relevant tests and
documentation. It does not change direct converting/in-place constructors,
state queries, checked or contract accessors, value_or or assignment semantics.
**VRT-13 valueless_by_exception remains OPEN and is not remediated.**

### Reproduction before production edits

An independent C++20 program confirmed the requested behavior:

| Expression | Before | After |
| --- | --- | --- |
| Result<int,long>::failure(3) | has_value=1, value_if non-null, error_if null | has_value=0, value_if null, error_if non-null |
| Result<double,int>::success(3) | has_value=0, value_if null, error_if non-null | has_value=1, value_if non-null, error_if null |

Both new minimal regression tests failed against the pre-VRT-03 production header.
Only after confirmation was Result.h modified. Successful corrected construction
contains long(3) in the first error case and double(3) in the second value case.

### Implementation and contract

SSOT §§40–45, 47, 57, 80–86, 89–90 and 124 apply.

- Success always initializes `storage_(std::in_place_index<0>, forwarded args...)`.
- Failure always initializes `storage_(std::in_place_index<1>, forwarded arg)`.
- A private FactoryTag separates internal constructors from public constructor
  overloads. In-place-index tokens supplied as actual payloads remain supported.
- Factory availability requires construction of the named target alternative;
  an argument constructible only as the opposite alternative is now rejected.
- `noexcept` is conditional on construction of that same target with the actual
  forwarded reference categories (`Args&&...`). Source type never selects state.
- Preserve the single-argument success template (including explicit template
  arguments), variadic/default-value success and single-argument failure APIs.
- Retain the T==E prohibition. No default construction of an inactive alternative,
  new data member, heap allocation, loop or state-repair mechanism is introduced.

Generic payload construction can throw or allocate according to its own type.
The throwing-constructor test fails before any Result is published; it does not
exercise or fix throwing assignment / valueless_by_exception. Direct construction
from a source constructible as both T and E may still be ambiguous; named factories
provide the explicit choice. These boundaries are documented in docs/core.md.

### Audit and regression matrix

Nine ResultFactoryTest tests were added to the existing ResultTest.cpp, leaving
the VRT-02 CMake change untouched.

| Requirement | Evidence |
| --- | --- |
| Convertible source types | int-to-long failure; int-to-double success; long-to-int success |
| Source convertible to both T and E | DualNumericSource conversion to int=11 and long=29; name selects the correct conversion |
| Mutable lvalue / const lvalue / rvalue | Each category tested independently for both success and failure, with category-tagged payloads |
| Move-only T and E | Both alternatives non-copyable; real non-trivial moves and cross-alternative move assignment verified |
| Non-default-constructible T/E | Both alternatives have deleted default constructors and construct directly from supplied data |
| Conditional noexcept | Compile-time checks for category-dependent construction and opposing throwing/non-throwing alternatives |
| Constructor exception propagation | Throwing chosen alternative is catchable; unchosen throwing constructor is not invoked |
| T==E prohibition | Standalone negative compile test fails with the retained “cannot be the same type” assertion |
| value_if / error_if | Mutable/const pointers agree with the chosen state and target value |
| value_or | Old and new tests cover success/error, const-lvalue and rvalue Result, plus move-only extraction/fallback |
| Copy/move/assignment | Copy and non-trivial move construction; both state transitions and same-state replacement |
| Additional compatibility | constexpr, empty/variadic success, direct value/error/in-place construction, explicit factory template arguments, index-token payloads |

Static analysis initially identified repeated forwarding and meaningless moves in
test fixtures. These were corrected with independent source objects and genuinely
non-trivial move fixtures, not warning suppressions. The final changed test TU was
rechecked; all other TUs from the full analysis had already succeeded unchanged.

### Final local verification

Environment: AppleClang 21.0.0, ISO C++20, x86_64 Darwin, CMake 4.4.3,
GoogleTest v1.14.0. Existing configured out-of-tree builds and strict warning flags
are retained, including -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror.

| Check | Result |
| --- | --- |
| Debug full suite | 166/166 passed |
| Release full suite | 164/164 passed (two assertion-only tests excluded) |
| ASan+UBSan full suite | 166/166 passed, no diagnostics |
| Principal VRT-01 / VRT-02 regressions | Dedicated 22/22 Debug rerun passed; also included in every full suite |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs built |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs built |
| clang-tidy | Final 98-TU source set passed: 97 unchanged TUs plus rechecked final ResultTest.cpp |
| Project compiler warnings / unsuppressed static diagnostics | 0 / 0 |
| Fresh LLVM function coverage | 175/175 = 100% of emitted function set |
| Fresh LLVM line coverage | 900/909 = 99.0099% |
| Fresh RAW LLVM branches | 325/360 = 90.2778% |
| Coverage gate | PASS with unchanged thresholds; no denominator subtraction or exclusions |
| git diff --check | passed |

Sanitizer environment: `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. This does not close VRT-05.
The coverage runner removed old profiles before generating and merging a new one.
Emitted-function coverage is not proof that every public template instantiation
was covered; VRT-10 remains OPEN. Explicit clang-tidy configuration and macOS
SDK/target arguments were used; the harness issues remain OPEN.

The supplied Step 3 evidence contains the baseline snapshot, old/new reproducers,
negative compile fixture, failing/passing tests, static logs (including initial
test-fixture diagnostics), final raw coverage, incremental diff and SHA-256 manifest.
No remote CI jobs were run on this uncommitted patch; independent review is pending.

### Stop condition

VRT-01: **implemented locally, NOT CLOSED**.
VRT-02: **implemented locally, NOT CLOSED**.
VRT-03: **implemented locally, pending CI / independent review, NOT CLOSED**.
VRT-04 through VRT-19: **OPEN**, including unchanged VRT-13.
Overall: **NOT REQUALIFIED / Experimental**.
No commit, push or automatic continuation to VRT-04.

## Step 4 — VRT-04

### Scope, baseline and preserved work

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before edits, recorded status, HEAD, diff stat, complete diff and SHA-256 hashes
of the 12 existing modified/untracked remediation files. All VRT-01 through
VRT-03 production and test files remain byte-identical to that snapshot.
The pre-existing CMake file only gains the new test source; geometry documentation
only gains the VRT-04 contract. The status overview changes and this Step 4 is
appended; the historical Step 1 through Step 3 body remains byte-identical.
No commit, push, reset or rebase was performed.

### Reproduction and exact cause

Before production edits, compiled an independent single-TU program containing
`Matrix3<double>::Identity() * Vector3<double, WorldFrame>{1,2,3}` with
AppleClang 21.0.0, ISO C++20 and all six project warning/error flags.
The compiler rejected it: `satisfaction of constraint ... depends on itself`.
The original source and full compiler diagnostics are retained in Step 4 evidence.

The Matrix scalar member previously excluded only the exact current Matrix type,
then asked for `ScalarArithmetic<S>`. With `S = Vector3`, that concept probed
`a*b`, which considered Vector scalar multiplication constrained by the same
`ScalarArithmetic<Vector3>` being evaluated. The Vector scalar-left ADL candidate
also examined Matrix types. Merely preferring the Matrix-vector overload would
leave this invalid recursive candidate in the set.

### Implementation and compile-time API compatibility

- Added `is_geometry_aggregate_v`, specialized for all Matrix3/Vector3 types.
  `ScalarArithmetic` first strips cv/ref and rejects these aggregates; only then
  does it check the existing scalar algebra expressions. This short-circuits
  recursion without stripping support for custom scalar arithmetic or units.
- Matrix scalar-right multiplication now uses that semantic exclusion instead
  of an exact-current-Matrix exception. The Matrix storage constraint remains
  `ScalarArithmetic`; no new scalar domain or implicit conversion was added.
- Added namespace scalar-left Matrix multiplication, with original operand order,
  `constexpr`, and conditional `noexcept` based on component multiplication.
  Built-in float/double multiplication is non-throwing. Existing noexcept
  declarations and all Vector operator signatures are unchanged.
- Matrix-vector continues to select the existing member templated on U and Frame.
  Matching scalar inputs return `Vector3<T,F>`; float/double mixing returns
  `Vector3<double,F>`. No Frame conversion, Matrix-point overload or cross-scalar
  Matrix-matrix conversion was added. Existing valid scalar promotions remain.
- Matrix-matrix, Matrix-vector, transpose, determinant, Frobenius norm, inverse
  (including scaled inverse and alias safety), and comparison bodies were checked
  byte-for-byte against HEAD and are unchanged. No storage/layout change occurred.

The semantic trait/constraint is a public compile-time API change. New aggregate
kinds must be classified before use in scalar overload resolution. This work does
not attempt to qualify arbitrary third-party scalar algebra implementations.

### Deterministic public API and regression evidence

Dedicated `Matrix3PublicMultiplicationTest.cpp` adds 10 runtime tests plus static
contracts. Public `A*x` expressions (no workaround dispatch) instantiate all four
multiplication categories for float/double, with World and Body Frames. Static
contracts check exact result types, negative overload viability and noexcept;
four constexpr evaluations check `(1..9)*(1,2,3) = (14,32,50)`.

Runtime independent oracles cover identity, zero, `(14,32,50)`, diagonal scaling
`(-2,-6,-12)`, signed multiplication `(-14,32,-50)`, known matrix square, both
scalar orders, mixed scalar promotion, and unit-valued vector components.
Negative constraints reject unsupported scalar multiplication, Matrix-point,
Vector-matrix, Vector-vector multiplication, cross-scalar Matrix-matrix,
cross-Frame addition and implicit Frame/unit erasure. A Quantity scalar remains
accepted and its result retains the unit and Frame.

All 21 Matrix tests passed, including 11 unchanged inverse/comparison tests.
All 31 principal VRT-01/02/03 regressions passed, including the two original Result
factory failures and the custom-scalar UnitVector invariant escape regression.

### Final local verification

Toolchain: AppleClang 21.0.0 (`clang-2100.1.1.101`), x86_64 Darwin,
CMake 4.4.3; C++20. Existing warning flags were retained without suppression.
Build directories use the same isolated GoogleTest v1.14.0 source/include pairing
as Step 3; no repository dependency configuration changed.

| Check | Final result |
| --- | --- |
| Debug | 176/176 passed |
| Release | 174/174 passed; existing Debug-only tests account for difference |
| ASan + UBSan | 176/176 passed |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs passed |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs passed |
| clang-tidy | 99/99 final-source translation units passed |
| Compiler warnings / unsuppressed static diagnostics | 0 / 0 |
| Fresh raw LLVM functions | 179/179 = 100% |
| Fresh raw LLVM lines | 927/936 = 99.0385% |
| Fresh RAW LLVM branches | 325/360 = 90.2778% |
| Normative gates (100%, >=95%, >=90%) | PASS; no branch denominator deductions |
| git diff --check | passed |

Sanitizers used `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. VRT-05 remains OPEN;
local fail-fast execution does not repair the CI finding.

The initial fresh coverage run found a unit scalar-left component multiplication
that was instantiated but never executed (178/179 functions). A meaningful
unit-valued Matrix-vector runtime oracle was added; the final coverage target
removed old profiles and generated a new profile. Both runs are archived.
Raw metrics cover the runner's Numerics production scope, not Dynamics.
Emitted-function coverage does not prove every public template instantiation is
covered. VRT-10 remains OPEN. Header isolation proves independent inclusion,
not every template instantiation; the dedicated public expressions are separate
evidence for only the combinations tested here.

clang-tidy used the repository configuration and explicit macOS target/SDK
arguments on all 99 final-source TUs. Existing harness findings remain OPEN.
CI and independent review were not performed on this uncommitted patch.

### Stop condition

VRT-01 through VRT-03: **implemented locally, NOT CLOSED**.
VRT-04: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-05 through VRT-19: **OPEN**. No VRT-05 remediation was begun.
Overall: **NOT REQUALIFIED / Experimental**.
Stop after this step. Do not commit or push.

## Step 5 — VRT-05 and VRT-06

### Scope, baseline and preserved work

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before editing, verified clean working tree baseline relative to VRT-01..VRT-04 and recorded SHA-256 hashes of all 16 existing modified and untracked files.
All production and test source files for VRT-01, VRT-02, VRT-03, and VRT-04 remain 100% byte-identical to that snapshot.
The historical records for Steps 1 through 4 are preserved verbatim above.
No commit, push, reset, or rebase was performed.
This step addresses only CI qualification reliability and gate enforcement:
- VRT-05: UBSan diagnostics fail-fast gate.
- VRT-06: CI pipeline tee upstream failure propagation.

### VRT-05 reproduction and remediation

**Defect reproduced before fix:**
An isolated standalone C++20 fault-injection program containing signed integer overflow (`volatile int a = INT_MAX; int b = a + 1;`) was compiled under the original configuration (`-fsanitize=undefined -fno-omit-frame-pointer`).
Execution under default environment produced:
- Diagnostic emitted: YES (`runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'`)
- Raw exit code: `0` (diagnostic emitted but process recovered and exited successfully).

**Remediation:**
1. In `cmake/Sanitizers.cmake`, added `-fno-sanitize-recover=undefined` to both compile and link flags for all UBSan configurations (`VECTORIS_ENABLE_UBSAN`).
2. In `.github/workflows/cross-compiler-qualification.yml`, configured explicit fail-fast environment variables in the `sanitizers-linux` job:
   - `ASAN_OPTIONS: "halt_on_error=1:abort_on_error=1"`
   - `UBSAN_OPTIONS: "halt_on_error=1:print_stacktrace=1"`
3. Created an isolated negative gate self-test script: `tools/sanitizers/verify_sanitizers_gate.py`:
   - Under default environment (no `UBSAN_OPTIONS`), UBSan violation aborts with raw exit code `-6` (SIGABRT).
   - Under `UBSAN_OPTIONS=halt_on_error=1`, UBSan violation aborts with raw exit code `-6`.
   - Under ASan use-after-free violation, ASan aborts with raw exit code `-6`.
   - Under clean execution with combined ASan+UBSan, program exits with code `0` and empty stderr.

### VRT-06 reproduction and remediation

**Defect reproduced before fix:**
Linux bash pipeline command `false | tee /dev/null` executed without pipefail produced:
- Raw exit code: `0` (swallowed upstream failure because `tee` succeeded).
- In a multi-command script under `bash -e`, subsequent commands were executed despite the upstream command failing.
With `set -euo pipefail`, the same pipeline returned raw exit code `1` and immediately halted execution without executing subsequent commands.

**Remediation:**
1. In `.github/workflows/cross-compiler-qualification.yml`:
   - Configured `defaults.run.shell: bash` across all Linux workflow jobs (`gcc-linux`, `clang-linux`, `sanitizers-linux`, `coverage-linux`, `static-analysis-linux`).
   - Added explicit `set -euo pipefail` to every multi-line bash run step that utilizes pipelines (e.g., `cmake ... | tee ...`, `ctest ... | tee ...`).
   - Retained tee logging so full build and test diagnostics remain captured in build logs.
2. Windows native command failure propagation:
   - Audited the MSVC PowerShell (`shell: pwsh`) job.
   - Appended explicit `$LASTEXITCODE` checking immediately after every native command invocation (`cmake.exe`, `ctest.exe`):
     `if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }`
   - Prevents subsequent native commands from overwriting failure status codes.
3. Created an isolated negative pipeline gate self-test script: `tools/ci/verify_pipeline_gate.py`:
   - Validates Linux bash pipefail propagation.
   - Statically audits `.github/workflows/cross-compiler-qualification.yml` to verify every piped step has `pipefail` and every pwsh native command has `$LASTEXITCODE` checks.
4. Workflow syntax validated:
   - `actionlint` was confirmed not installed on the host environment.
   - Validated complete workflow YAML syntax using Ruby YAML parser (`Psych`), confirming all 6 jobs and 35 steps parse cleanly.

### Prior workload historical classification

The prior CI run on audit commit `cfbecc6390fb30d857f10f2516f39bb2ef75f996` executed real builds and tests.
The workload succeeded, but the workflow could not reliably propagate every future failure.
The VRT-05 and VRT-06 fixes ensure gate reliability for all future qualification runs.

### Principal regressions verification (VRT-01 through VRT-04)

All prior Core remediation tests were re-executed locally and verified passing (100%):
- Quaternion extreme normalization and conversion (`QuaternionScaleTest`, `QuaternionTest`, `GeometryComparisonTest`): PASS
- UnitVector3 huge/tiny normalization and encapsulation (`UnitVector3ScaleTest`, `UnitVector3CrossPrecisionTest`): PASS
- Result named factory semantics (`ResultTest`): PASS
- Matrix3 × Vector3 public instantiation and multiplication (`Matrix3PublicMultiplicationTest`): PASS

### Final local verification matrix

Toolchain: AppleClang 21.0.0 (`clang-2100.1.1.101`), x86_64 Darwin, CMake 4.4.3; ISO C++20.
Warning flags: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` (0 warnings).

| Check | Final result | Notes |
| --- | --- | --- |
| Debug | 176/176 passed | Full test suite passed (5.56s) |
| Release | 174/174 passed | 2 assertion-only tests excluded by design (2.37s) |
| ASan | 176/176 passed | Fail-fast runtime options enabled, 0 diagnostics (29.70s) |
| UBSan | 176/176 passed | Fail-fast runtime options enabled, 0 diagnostics (7.53s) |
| ASan + UBSan | 176/176 passed | Fail-fast runtime options enabled, 0 diagnostics (30.10s) |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs passed | 0 warnings |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs passed | 0 warnings |
| clang-tidy | 99/99 translation units passed | 0 production diagnostics |
| Compiler warnings / static diagnostics | 0 / 0 | Zero warnings policy strictly maintained |
| Fresh raw LLVM functions | 179/179 = 100% | Normative gate satisfied |
| Fresh raw LLVM lines | 927/936 = 99.04% | >=95.0% normative threshold satisfied |
| Fresh RAW LLVM branches | 325/360 = 90.28% | >=90.0% normative threshold satisfied |
| Negative Sanitizers Gate | PASS | `tools/sanitizers/verify_sanitizers_gate.py` |
| Negative Pipeline Gate | PASS | `tools/ci/verify_pipeline_gate.py` |
| git diff --check | passed | Clean whitespace and diff formatting |

### Stop condition

VRT-01: **implemented locally; NOT CLOSED**.
VRT-02: **implemented locally; NOT CLOSED**.
VRT-03: **implemented locally; NOT CLOSED**.
VRT-04: **implemented locally; NOT CLOSED**.
VRT-05: **implemented locally; NOT CLOSED**.
VRT-06: **implemented locally; NOT CLOSED**.
VRT-07 through VRT-19: **OPEN**.
Overall: **NOT REQUALIFIED / Experimental**.

Stop after this step. Do not commit or push.

## Step 6 — VRT-07

### Scope, baseline and preserved work

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before editing, verified clean working tree baseline relative to VRT-01..VRT-06 and recorded SHA-256 hashes of all 19 existing modified and untracked files.
All production and test source files for VRT-01, VRT-02, VRT-03, VRT-04, VRT-05, and VRT-06 remain 100% byte-identical to that snapshot.
The historical records for Steps 1 through 5 are preserved verbatim above.
No commit, push, reset, or rebase was performed.
This step addresses only Dynamics integrator transactional safety and input validation:
- Finding: VRT-07: Dynamics integrator may accept invalid physical inputs or produce non-finite translational state, commit it, and still report success.
- Constraint: VRT-16 dimensional semantics (`Cross(angular velocity, linear velocity)`) remains strictly untouched and OPEN.

### VRT-07 reproduction before production edits

Before editing production code, an independent C++20 test program (`tools/reproduce_vrt07.cpp`) evaluated 11 boundary conditions against the existing pre-remediation implementation:

| Test Case | Input Condition | Old Return Code | Old Committed State | Defect Manifestation |
| :--- | :--- | :--- | :--- | :--- |
| 1 | `mass == 0.0` | `SUCCESS` | pos: `(NaN, NaN, NaN)`, vel: `(NaN, NaN, NaN)` | Accepted 0 mass; divided by 0; committed NaN state |
| 2 | `mass < 0.0` (`-10.0`) | `SUCCESS` | pos: `(-1.0, 0, 0)`, vel: `(-2.0, 0, 0)` | Accepted negative mass; accelerated in reverse; committed corrupted state |
| 3 | `mass = NaN` | `SUCCESS` | pos: `(NaN, NaN, NaN)`, vel: `(NaN, NaN, NaN)` | Accepted NaN mass; committed NaN state |
| 4 | `mass = +Inf` | `SUCCESS` | pos: `(1.0, 0, 0)`, vel: `(0, 0, 0)` | Accepted infinite mass without error |
| 5 | `force = NaN` | `SUCCESS` | pos: `(NaN, 0, 0)`, vel: `(NaN, 0, 0)` | Accepted NaN force; committed NaN state |
| 6 | `force = +Inf` | `SUCCESS` | pos: `(+Inf, 0, 0)`, vel: `(+Inf, 0, 0)` | Accepted +Inf force; committed Inf state |
| 7 | `force = -Inf` | `SUCCESS` | pos: `(-Inf, 0, 0)`, vel: `(-Inf, 0, 0)` | Accepted -Inf force; committed -Inf state |
| 8 | `initial pos = NaN` | `SUCCESS` | pos: `(NaN, 0, 0)`, vel: `(0, 0, 0)` | Accepted non-finite position; committed NaN |
| 9 | `initial vel = NaN` | `SUCCESS` | pos: `(NaN, 0, 0)`, vel: `(NaN, 0, 0)` | Accepted non-finite velocity; committed NaN |
| 10 | `pos overflow` (`1e308`) | `SUCCESS` | pos: `(+Inf, 0, 0)`, vel: `(1e308, 0, 0)` | Overflowed to +Inf; committed corrupted state |
| 11 | `vel overflow` (`1e308 * dt`) | `SUCCESS` | pos: `(+Inf, 0, 0)`, vel: `(+Inf, 0, 0)` | Overflowed to +Inf; committed corrupted state |

All 11 cases returned `SUCCESS` and corrupted the caller's state.

### Physical and input contract

Following SSOT §§10–14, 36, 44–48, 57, 60, and 124, the contract was formalized in `modules/VectorisDynamics/docs/dynamics.md`:
1. `dt`: Must be strictly positive and finite (`dt > 0` and `std::isfinite(dt)`). Non-positive or non-finite `dt` fails immediately with `MathError::invalid_argument`.
2. `mass`: Must be strictly positive and finite (`mass > 0` and `std::isfinite(mass)`). Non-positive `mass` fails with `MathError::invalid_argument`; non-finite `mass` fails with `MathError::non_finite_input`.
3. `force`: All 3 spatial components of `Wrench3.force` must be finite (`std::isfinite`). Any non-finite component fails with `MathError::non_finite_input`.
4. `torque`: All 3 spatial components of `Wrench3.torque` must be finite (`std::isfinite`). Any non-finite component fails with `MathError::non_finite_input`.
5. `initial state`: All 13 kinematic scalar components of `RigidBodyState` must be finite:
   - `position` (3 scalars): `std::isfinite`
   - `linearVelocity` (3 scalars): `std::isfinite`
   - `orientation` (4 quaternion scalars): `std::isfinite`, unit norm
   - `angularVelocity` (3 scalars): `std::isfinite`
   Any non-finite component fails with `MathError::non_finite_input`.

### Candidate validation and transactional commit strategy

To guarantee transactional rollback upon failure:
1. **Input Preconditions**: `EulerIntegrator::Step` and `RigidBodyDynamicsKernel::ComputeDerivative` validate all inputs (`dt`, `mass`, wrench, initial state) before computing anything.
2. **Temporary Candidate Calculation**:
   - `RigidBodyState<Frame, T> candidate = state;`
   - Linear derivative $\mathbf{a} = \mathbf{F} / m - \boldsymbol{\omega} \times \mathbf{v}$ is computed and validated for finiteness.
   - Candidate velocity $\mathbf{v}_{\text{cand}} = \mathbf{v} + \mathbf{a} \Delta t$ is computed into temporary storage and verified finite.
   - Candidate position $\mathbf{x}_{\text{cand}} = \mathbf{x} + (\mathbf{q} \mathbf{v}_{\text{cand}}) \Delta t$ is computed into temporary storage and verified finite.
   - Candidate orientation $\mathbf{q}_{\text{cand}}$ is computed via `AttitudeEngine<T>::IntegrateAngularVelocity`. If orientation integration fails, error propagates immediately.
   - Candidate angular velocity $\boldsymbol{\omega}_{\text{cand}} = \boldsymbol{\omega} + \boldsymbol{\alpha} \Delta t$ is computed and verified finite.
3. **Atomic Commit**:
   - The entire candidate state is verified via `IsStateFinite(candidate)`.
   - The state update is committed atomically via `state = candidate;` ONLY after every translational and rotational candidate scalar is validated.
   - On any failure: `return Result<void, MathError>::failure(err);` leaving the caller's `state` 100% bit-for-bit unchanged.
   - Partial updates (e.g. valid rotation + invalid translation, or vice versa) are mathematically impossible.

### Deterministic regression tests added

Added 6 extensive test suites in `modules/VectorisDynamics/tests/EulerDynamicsTest.cpp`:
1. `VRT07_MassValidationAndTransactionalRollback`: Verifies `mass == 0` (`invalid_argument`), `mass < 0` (`invalid_argument`), `mass = NaN` (`non_finite_input`), `mass = +Inf` (`non_finite_input`), and asserts all 13 kinematic scalars in caller state are preserved.
2. `VRT07_ForceValidationAndTransactionalRollback`: Verifies NaN, +Inf, -Inf for force components and torque components return `non_finite_input` and preserve state.
3. `VRT07_InitialStateValidationAndTransactionalRollback`: Verifies NaN, +Inf, -Inf in position, velocity, and angular velocity return `non_finite_input` and preserve state.
4. `VRT07_CandidateOverflowValidationAndTransactionalRollback`: Verifies candidate position and velocity overflow return `non_finite_input` and preserve state.
5. `VRT07_OrdinaryTranslationIndependentOracle`: Independent hand-calculated oracle for discrete symplectic Euler:
   - $m = 2.0\,\text{kg}$, $F_x = 20.0\,\text{N}$, $dt = 0.1\,\text{s}$, $x_0 = 10.0\,\text{m}$, $v_0 = 1.0\,\text{m/s}$
   - Hand calculation: $a_x = 20.0 / 2.0 = 10.0\,\text{m/s}^2$; $v_1 = 1.0 + 10.0 \times 0.1 = 2.0\,\text{m/s}$; $x_1 = 10.0 + 2.0 \times 0.1 = 10.2\,\text{m}$.
   - Verified exact match against independent oracle.
6. `VRT07_ZeroForceAndConstantVelocity`: Independent hand-calculated oracle for zero-force coasting:
   - $F = \mathbf{0}$, $\boldsymbol{\tau} = \mathbf{0}$, $v = [2.0, 1.6, 3.2]\,\text{m/s}$, $dt = 1.0\,\text{s}$, initial $x = \mathbf{0}$.
   - Hand calculation: $x_1 = [2.0, 1.6, 3.2]\,\text{m}$, $v_1 = [2.0, 1.6, 3.2]\,\text{m/s}$.
   - Verified exact match.

Existing rotational and dynamics suites were re-verified:
- `RigidBodyEulerIntegrationTest.StepReturnsSuccessForValidInput`: PASS
- `RigidBodyEulerIntegrationTest.RotationalDynamicsMatchesAnalyticSolution`: PASS
- `RigidBodyEulerIntegrationTest.AttitudeRemainsNormalized`: PASS
- `RigidBodyEulerIntegrationTest.NonDiagonalInertiaPreservesMomentum`: PASS
- `RigidBodyEulerIntegrationTest.AttitudeNormalizationFailureRollback`: PASS

### Principal regressions verification (VRT-01 through VRT-06)

All prior Core and Gate remediation tests were re-executed locally and verified passing (100%):
- VRT-01: Quaternion extreme normalization and conversion (`QuaternionScaleTest`, `QuaternionTest`, `GeometryComparisonTest`): PASS
- VRT-02: UnitVector3 extreme normalization and encapsulation (`UnitVector3ScaleTest`, `UnitVector3CrossPrecisionTest`): PASS
- VRT-03: Result named factory semantics (`ResultTest`): PASS
- VRT-04: Matrix3 × Vector3 public instantiation and multiplication (`Matrix3PublicMultiplicationTest`): PASS
- VRT-05: Sanitizer fail-fast gate (`tools/sanitizers/verify_sanitizers_gate.py`): PASS
- VRT-06: CI pipeline failure propagation gate (`tools/ci/verify_pipeline_gate.py`): PASS

### Final local verification matrix

Toolchain: AppleClang 21.0.0 (`clang-2100.1.1.101`), x86_64 Darwin, CMake 4.4.3; ISO C++20.
Warning flags: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` (0 warnings).

| Check | Final result | Notes |
| :--- | :--- | :--- |
| Debug | 182/182 passed | Full test suite passed (5.71s) |
| Release | 180/180 passed | 2 assertion-only tests excluded by design (2.54s) |
| ASan | 182/182 passed | Fail-fast runtime options enabled, 0 diagnostics (30.37s) |
| UBSan | 182/182 passed | Fail-fast runtime options enabled, 0 diagnostics (7.53s) |
| ASan + UBSan | 182/182 passed | Fail-fast runtime options enabled, 0 diagnostics (31.00s) |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs passed | 0 warnings |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs passed | 0 warnings |
| clang-tidy | 99/99 translation units passed | 0 production diagnostics |
| Compiler warnings / static diagnostics | 0 / 0 | Zero warnings policy strictly maintained |
| Fresh raw LLVM functions | 179/179 = 100% | Normative gate satisfied |
| Fresh raw LLVM lines | 927/936 = 99.04% | >=95.0% normative threshold satisfied |
| Fresh RAW LLVM branches | 325/360 = 90.28% | >=90.0% normative threshold satisfied |
| Negative Sanitizers Gate | PASS | `tools/sanitizers/verify_sanitizers_gate.py` |
| Negative Pipeline Gate | PASS | `tools/ci/verify_pipeline_gate.py` |
| git diff --check | passed | Clean whitespace and diff formatting |

### Stop condition

VRT-01: **implemented locally; NOT CLOSED**.
VRT-02: **implemented locally; NOT CLOSED**.
VRT-03: **implemented locally; NOT CLOSED**.
VRT-04: **implemented locally; NOT CLOSED**.
VRT-05: **implemented locally; NOT CLOSED**.
VRT-06: **implemented locally; NOT CLOSED**.
VRT-07: **implemented locally; CI / independent review pending; NOT CLOSED**.
VRT-08 through VRT-19: **OPEN**.
Overall: **NOT REQUALIFIED / Experimental**.

Stop after this step. Do not commit or push.


## Step 7 — VRT-08

### Scope and baseline preservation

This is the next ledger entry: the existing ledger combined VRT-05/06 in Step 5
and recorded VRT-07 in Step 6. All pre-existing historical entries are preserved
verbatim; none was renumbered or rewritten.

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before production edits, recorded git status, HEAD, diff stat, full diff and
SHA-256 snapshots of all 24 existing modified/untracked files. All VRT-01..07
production, test and CI/gate files remain byte-identical. Previously modified
files changed by this step: `docs/geometry.md` (LDLT section only), Numerics
`CMakeLists.txt` (one test source added), and this ledger (overview + new entry).
`Matrix3.h`, including `TryInverse`, is unchanged. No commit, push, reset or rebase.

Before LDLT remediation, checked the Dynamics specification **at baseline HEAD**:
`modules/VectorisDynamics/docs/dynamics.md` §8.1 already explicitly requires finite
`dt > 0`, rejects non-positive dt, and separately rejects non-finite dt. The
Engineering Standard's general integration/time sections do not contradict it.
Thus the VRT-07 positive-dt policy has pre-existing specification support; no
VRT-07 code/documentation was changed. The exact baseline excerpt is archived.

### Both defects reproduced before production edits

Let `A = s * [[1,c,0],[c,1,0],[0,0,1]]` and `b = s*(1,-1,1)`.
The eigenvalues are `s*(1-c), s, s*(1+c)`, so `cond2=(1+c)/(1-c)`.
The analytic solution is `(1/(1-c), -1/(1-c), 1)`.

| Case | Double scale | Float scale | Old result / observable quantities |
| --- | --- | --- | --- |
| c=0.9, cond2 approximately 19 | 1e308 | 2e38 | `ill_conditioned`; forward substitution y1 overflows to -Inf |
| c=0.5, cond2=3 | 1e308 | 2e38 | SUCCESS `(2,-2,1)`; residual `(Inf,-Inf,0)`, denominator Inf, eta NaN, `eta > bound` false |

The second case demonstrates an ineffective acceptance guard; it does not show
an incorrect solution in that particular example. A scale-one control succeeds.
Full decimal A/b values, rounded float/double values, old result and intermediate
quantities are preserved in `reproducer-before.log`. Both defect classes were
confirmed before changing the production header.

### Scale-safe model and acceptance

The solver remains 3x3, SPD, sqrt-free LDLT, fixed-size O(1), deterministic and
heap-free. No matrix inverse or external factorization was introduced.

Normalize with `sa=max(abs(Aij))` and `sb=max(abs(bi))`: `a=A/sa`, `bn=b/sb`.
Factor a using the original LDLT equations and solve `a*y=bn`. Restore
`x=y*sb/sa` by binary fraction/exponent decomposition; do not form `1/sa` or
`sb/sa`. Exact arithmetic equivalence follows by multiplying the normalized
equation by sb. Zero RHS returns zero only after validating/factoring A.
A finite nonzero RHS producing an unrepresentable solution is rejected.

Normalized symmetry tolerance is `100*epsilon`; normalized pivot threshold is
`10*epsilon`, algebraically the previous scale-relative threshold divided by sa.
The lower triangle is still mirrored for factorization, while all original
normalized entries are retained for residual validation. Threshold-boundary
rounding can change; underflow/overflow of original-scale thresholds is removed.
Negative pivots, zero/unusable pivots and the unchanged pivot-spread safeguard
retain `invalid_state`, `singular_matrix`, and `ill_conditioned` diagnostics.
**Pivot spread is not cond(A)** and does not guarantee small forward error.

Validate the rounded output x, rather than only the intermediate solution y.
Safely reconstruct `z=x*sa/sb`; let `q=max(1,||z||inf)`, `w=z/q`, `c=bn/q`.
The computed acceptance measure is

`eta = ||a*w-c||inf / (||a||inf*||w||inf + ||c||inf)`.

In exact arithmetic it equals `||A*x-b||inf/(||A||inf*||x||inf+||b||inf)`.
Normalized component magnitudes are <=1, matrix row norms <=3 and denominator
<=4. Check finite components and row residual/norm quantities **before max
reductions**, then finite residual/denominator, then explicitly finite eta,
then `eta <= 100*epsilon`. Non-finite diagnostics fail closed as
`ill_conditioned`. A zero denominator is accepted only with zero residual.
Input NaN/Inf remains `non_finite_input`; excessive asymmetry is `invalid_argument`.

The API remains return-by-value with const inputs; no output parameter exists,
so output-parameter aliasing/transactional commit is not applicable. Input
preservation and Dynamics transactional rejection regressions pass.

### Tests and independent oracles

Added `SymmetricLinearSolver3ScaleTest.cpp` with 18 float/double runtime tests,
plus constexpr solves and extreme rescaling assertions. Families cover ordinary,
diagonal, coupled/non-diagonal, common-scale copies, independent RHS scales,
subnormal outputs, unrepresentable intermediate scale ratios with representable
solutions, truly overflowing/underflowing solutions, zero matrix/RHS, all NaN/Inf
input positions, asymmetry, negative/singular pivots and pivot-spread rejection.
Direct diagnostic fault injection checks NaN/Inf norms and finite numerator /
denominator producing infinite eta. Existing backward-error rejection remains
passing. The constant-evaluation algorithms are also compared independently to
standard `frexp/scalbn` across exponents (including overflow/underflow).

Float common-scale tests start at `2^-148` and reach maximum finite float;
single-axis/diagonal tests also use `2^-149`. Double common-scale tests start at
`2^-1073` and reach maximum finite double; diagonal tests use `2^-1074`.
Decimal scales include float `1e-30,1e30,2e38` and double `1e-300,1e300,1e308`.
The cond2 approximately 19 audit system now succeeds, yielding approximately
`(10,-10,1)` for float/double at extreme scale.

Independent expectations use analytic x and eigenvalues, plus a long-double
power-of-two normalized residual calculation on the **original input equation**.
Principal matrices have cond2: identity 1; diagonal `(2,3,4)` 2; block coupling
0.5 -> 3; coupling 0.9 -> approximately 19; tridiagonal `(2,1,0;1,2,1;0,1,2)`
-> `3+2*sqrt(2)` approximately 5.828427; dense diag .5/offdiag .25 -> 4.
These are analytic eigenvalue ratios, not inferred from LDLT pivot spread.

A separate observable oracle program (AppleClang 21, -O2) tested 34 float and
141 double systems, with zero failures:

| Metric | float | double |
| --- | --- | --- |
| Maximum independent eta | 1.9868216504114191e-8 | 4.7580986769649575e-17 |
| Maximum absolute infinity-norm residual | 5.6011525532496495e31 | 2.2219315945991998e292 |
| Maximum component error / max(1,abs(expected)) | 5.7220469898313503e-7 | 1.0658141036401501e-15 |

Absolute residuals reflect the enormous system scales; normwise eta is the
acceptance measure. These are observed local maxima, not universal error bounds.

### Final local verification

AppleClang 21.0.0 (`clang-2100.1.1.101`), CMake 4.4.3, ISO C++20,
x86_64 Darwin. Existing warning/static-analysis policies and isolated GoogleTest
pairing retained. The public function remains constexpr/noexcept with the same
scalar/Frame/result signature. New Detail helpers are implementation facilities.

| Check | Result |
| --- | --- |
| Debug | 200/200 passed |
| Release | 198/198 passed; existing assertion-only tests account for difference |
| ASan | 200/200 passed |
| UBSan | 200/200 passed |
| ASan+UBSan | 200/200 passed |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs passed |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs passed |
| clang-tidy | 100/100 final-source translation units passed |
| Compiler warnings / unsuppressed static diagnostics | 0 / 0 |
| VRT-01/02/03/04/07 principal regressions | 47/47 passed |
| LDLT + Matrix inverse/comparison regressions | 46/46 passed |
| VRT-05 negative sanitizer gate | PASS; nonzero violation exits, clean control passes |
| VRT-06 pipeline gate | PASS; local bash execution and workflow source audit |
| Fresh raw LLVM functions | 190/190 = 100% |
| Fresh raw LLVM lines | 1000/1011 = 98.9120% |
| Fresh RAW LLVM branches | 371/410 = 90.4878% |
| Normative gates: 100% / >=95% / >=90% | PASS; raw denominator unchanged by reporting |
| git diff --check | PASS |

All sanitizer modes used `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`; the retained VRT-05 compiler
configuration disables UBSan recovery. Windows/pwsh was not executed locally.
clang-tidy used explicit repository configuration and macOS SDK/target arguments.

The initial coverage run exposed unexecuted C++20 fallback arithmetic. It is
retained in evidence; independent runtime comparisons were then added for those
algorithms. Final profiles were freshly generated. No denominator subtraction,
coverage exclusion or warning suppression was introduced. Finite-vector checks
were factored into one helper to keep the public solve's control flow bounded.
Coverage measures emitted Numerics code, not all public template instantiations
or Dynamics code. Header isolation proves independent inclusion, not every
instantiation. **VRT-10 remains OPEN**.

No benchmark was performed. Added runtime work includes input normalization,
normalized residual evaluation and six component rescalings (18 frexp / six
scalbn calls); complexity and storage remain fixed. No performance guarantee.
Rounding, representability, gradual underflow and conditioning limits are now
explicit in the geometry numerical contract; no universal scale/condition claim.

### Stop condition

VRT-01 through VRT-07: **IMPLEMENTED LOCALLY; NOT CLOSED**.
VRT-08: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-09 through VRT-19: **OPEN**. VRT-09 was not begun.
Overall: **NOT REQUALIFIED / Experimental**.
No remote CI or independent review was performed. Stop after this step; no commit
or push.

## Step 8 — VRT-09

### Scope, baseline and preservation

Local verification completed 2026-09-22. Starting HEAD remains
`cfbecc6390fb30d857f10f2516f39bb2ef75f996`. Before production edits, recorded git
status, HEAD, diff/stat and SHA-256 snapshots of all 26 already modified/untracked
files. All existing VRT-01 through VRT-08 production, test and CI files remain
byte-identical. Prior-file changes in this step are limited to additions in
`docs/core.md`, `docs/geometry.md`, two test registrations in Numerics CMake,
and this ledger's current-status overview plus appended entry. Every historical
ledger entry remains verbatim. The existing ledger combines VRT-05/06 in Step 5,
so this new entry is Step 8. VRT-07's confirmed positive-dt policy was not revisited.

Only `Core/NumericTraits.h` changes production behavior. No commit, push, reset,
rebase or VRT-10 remediation. Requirements/model follow SSOT §§15–17, 26,
33–34, 80–84, 89–90 and 124. SSOT §16 defines the dual-tolerance equation but
neither the SSOT nor the prior API documentation defines invalid-tolerance
behavior. The explicit finite/nonnegative domain below adopts the audit request.

### Pre-edit reproduction

A standalone ISO C++20 reproducer was compiled and run before changing the
production header. For both float and double, with `a=max`, `b=-max`, `A=0`:

| Relative tolerance R | Old result | Old difference | Old relative threshold | New result |
| --- | --- | --- | --- | --- |
| 0 | false | +Inf | 0 | false |
| 0.5 | false | +Inf | max/2 | false |
| 1 | false | +Inf | max | false |
| 1.5 | **true (defect)** | +Inf | +Inf | **false** |
| 2 | true | +Inf | +Inf | true |

The exact relative difference is two, so the old `Inf <= Inf` accepted the
R=1.5 counterexample incorrectly. Nearby maximum/previous-representable,
maximum/half and maximum/zero controls were also captured before and after.
The after-run reproducer still prints the *old diagnostic expressions* separately;
those Inf values are not intermediate quantities in the repaired implementation.

### Contract and overflow-safe finite model

`Traits::AlmostEqual(a,b,A,R)` keeps its scalar constraint, argument order,
`Bool` result and `noexcept`. It was runtime-only, not constexpr, and remains so.
No public signature, allocation, exception or hidden state was added. The new
`Traits::Detail::FiniteAlmostEqual` helper is an implementation facility.

Both tolerances must be finite and nonnegative. Invalid A or R returns false
before every value fast path, including exact equality and same-sign infinity.
Negative zero is a valid tolerance. No clamping or R<=1 restriction is imposed.
A has the compared values' units; R is dimensionless. The intended finite model is

`|a-b| <= max(A, R*max(|a|,|b|))`.

After equality handling, let `h=max(|a|,|b|)>0`, `l=min(|a|,|b|)`:

- Same sign: `d=h-l` is finite. Accept `d<=A || d/h<=R`.
- Opposite signs: the real difference is h+l. Accept the absolute criterion via
  `h<=A && l<=A-h`, without forming h+l. The relative criterion is `1+l/h<=R`.
  R<1 rejects; R==1 accepts only l==0; R>=2 accepts; otherwise use `l/h<=R-1`.

The max threshold is equivalent to the OR of the absolute/relative criteria;
the rearrangements above are equivalent in real arithmetic. No overflowing
opposite-sign difference, tolerance product, or absolute-tolerance/scale ratio
is formed. The only division has numerator <= denominator. Handling R==1
before division prevents a nonzero tiny operand from disappearing through ratio
underflow at that endpoint. Original-scale absolute comparisons retain subnormal
tolerance meaning. There is no universal exact-rounding guarantee at tolerance
boundaries or across all floating-point environments.

For valid tolerances: any NaN operand returns false; same-sign infinities return
true; opposite infinities and finite/infinite pairs return false; every signed-zero
pair returns true. Comparison is symmetric and reflexive for non-NaN values,
but non-transitive and not an equivalence relation. With A=1/R=0, 0 is close to 1,
1 to 2, but 0 is not close to 2. These contracts are now documented in `docs/core.md`.

### Regression, independent oracles and wrapper audit

Added 16 scalar and four geometry wrapper tests across float/double. They cover
ordinary absolute/relative decisions, max/-max at all five required R values,
maximum/nextafter, maximum/half/zero, widely separated opposite-sign magnitudes,
denorm_min, 2*denorm_min, largest subnormal, minimum normal, signed zero, NaN,
all infinity signs, invalid A/R and symmetry/reflexivity boundaries.

The bounded deterministic property matrix has 2,940 float and 21,315 double
cases. Inputs and absolute tolerances are integer multiples of powers of two;
relative tolerances are multiples of 1/8. An independent exact integer inequality
provides expected decisions. The matrix also checks operand swapping, exact
finite equality and common scaling. No RNG is used. Selected cases additionally
use direct long-double real-equation calculations only where long double has
both wider precision and exponent range: 126 cases per precision locally.
The max/-max family has the independent analytic relative difference two.

Local floating-point environment: float digits=24/exponent max=128, double
53/1024, long double 64/16384, FE_TONEAREST. Float/double report IEC559 and
subnormal tests pass with gradual underflow. FTZ/DAZ environments are not assumed.

Vector3, Point3, Matrix3 member/free comparisons, Quaternion, UnitVector3,
RotationMatrix3, Transform3 and RotationEquivalent delegate to the scalar
comparison; none independently reconstructs the overflowing difference/product.
No geometry API or production wrapper changed. Extreme public quaternion storage
is used solely to test comparison delegation, not to claim rotation validity.
There is no Quantity/Units AlmostEqual overload: the scalar floating-point
constraint excludes Quantity. Compile-time probes retain rejection of Quantity
and mismatched Frame comparisons. No implicit cross-dimension comparison was added.

### Final local verification

AppleClang 21.0.0 (`clang-2100.1.1.101`), ISO C++20, CMake 4.4.3, GoogleTest
v1.14.0, x86_64 Darwin. Warning flags remain
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`; no new suppression.

| Check | Result |
| --- | --- |
| Debug | 220/220 passed |
| Release | 218/218 passed; two existing assertion-only tests account for difference |
| ASan | 220/220 passed |
| UBSan | 220/220 passed |
| ASan+UBSan | 220/220 passed |
| Numerics HeaderIsolation | 55 standalone headers + 2 order TUs passed |
| Dynamics HeaderIsolation | 11 standalone headers + 2 order TUs passed |
| clang-tidy | 102/102 final-source translation units passed |
| Compiler warnings / unsuppressed static diagnostics | 0 / 0 |
| Focused VRT-09 regressions | 20/20 passed |
| VRT-01 Quaternion regressions | 6/6 passed |
| VRT-02 UnitVector3 regressions | 16/16 passed |
| VRT-03 Result regressions | 9/9 passed |
| VRT-04 Matrix/Vector regressions | 10/10 passed |
| VRT-05 negative sanitizer gate | PASS; violations nonzero, clean combined control zero |
| VRT-06 pipeline gate | PASS; local bash execution and workflow source audit |
| VRT-07 Dynamics transactional regressions | 6/6 passed |
| VRT-08 LDLT regressions, including extreme scales/backward error | 35/35 passed |
| Fresh RAW LLVM functions | 192/192 = 100% |
| Fresh RAW LLVM lines | 1016/1027 = 98.9289% |
| Fresh RAW LLVM branches | 399/438 = 91.0959% |
| Normative gates: 100% / >=95% / >=90% | PASS; no denominator subtraction |
| git diff --check | PASS |

Sanitizer runtime options: `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Existing no-recovery UBSan
compiler policy is retained. No Windows/pwsh or remote CI execution is claimed.
clang-tidy used explicit repository configuration and macOS SDK/target arguments.

The first new wrapper-test compile attempted an unsupported Quaternion unary
minus. The test was corrected to construct the opposite quaternion through the
existing checked factory; its failure log is retained alongside final passing
builds. No production API was added to accommodate it.

No benchmark was performed. The finite path remains O(1), allocation-free and
deterministic, with at most one division and additional finite/domain branches.
Numerical behavior changes are corrected finite overflow decisions, explicit
invalid-tolerance rejection and potentially different rounded tolerance-boundary
decisions. Valid-tolerance NaN/Inf/signed-zero semantics are preserved.
Coverage records emitted Numerics code, not every public template instantiation,
all API combinations or Dynamics code. Header isolation does not establish all
template instantiations. **VRT-10 remains OPEN**.

Evidence (baseline hashes, isolated patch, exact verified files, logs and raw
coverage): `Vectoris_Remediation/step-09` in the local audit artifact directory.
The evidence README and SHA-256 manifest identify all artifacts.

### Stop condition

VRT-01 through VRT-08: **IMPLEMENTED LOCALLY; NOT CLOSED**.
VRT-09: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-10 through VRT-19: **OPEN**.
Overall: **NOT REQUALIFIED / Experimental**.
No remote CI or independent review was performed. Stop after VRT-09; no commit
or push.


## Step 9 — VRT-10 and VRT-11

### Scope, baseline and preservation

Starting repository HEAD remains:
`cfbecc6390fb30d857f10f2516f39bb2ef75f996`.

Before editing, captured `git status`, `git rev-parse HEAD`, `git diff --stat`, and verified SHA-256 integrity of all 29 baseline files (21 modified tracked files + 8 untracked files from VRT-01 through VRT-09). All prior production mathematical implementations, physical units, coordinate frame mechanics, regression tests, and CI/sanitizer gates are preserved 100% byte-for-byte.

Prior files touched in this step:
- `docs/ENGINEERING_STANDARD_V1.md`: Section 86 updated to decouple emitted-code coverage from public API surface verification.
- `tools/coverage/verify_coverage.py`: Updated terminal/summary output to explicitly report emitted-code coverage and direct users to the public API verification harness.
- `modules/VectorisNumerics/CMakeLists.txt`: Added `PublicApiSurfaceTest.cpp` to the test suite.
- `tools/static_analysis/run_clang_tidy.py`: Hardened with fail-closed compile database checks, category validation, and explicit config enforcement.
- `docs/audits/Vectoris_Red_Team_Remediation_2026-09-20.md`: Updated current status table and appended this Step 9 record.

No commit, push, reset, rebase, or VRT-12 remediation was performed.

### VRT-10 reproduction before production edits

Prior to this remediation, LLVM coverage tools (`llvm-profdata`, `llvm-cov export`) reported 100.0% function coverage across the library. However, uninstantiated template functions and uncalled template specializations in public headers are never emitted into object code by the C++ compiler. Consequently, LLVM coverage tools have no symbol records for them in the coverage mapping and do not include them in the function denominator.

**Defect reproduced before fix:**
An isolated test header `api.h` containing:
```cpp
template <typename T>
T InstantiatedFunction(T val) { return val * 2; }

template <typename T>
T UninstantiatedFunction(T val) { return val * 3; }
```
was compiled and executed with a test calling only `InstantiatedFunction<int>`.
LLVM coverage report output:
- `functions.count`: 1
- `functions.covered`: 1
- `functions.percent`: 100.0%
- `lines.count`: 2
- `lines.covered`: 2
- `lines.percent`: 100.0%

`UninstantiatedFunction` was completely absent from the emitted function set. Historically in Vectoris, operations such as `Matrix3 × Vector3` and various `UnitVector3` accessors were entirely absent from tests while the CI reported 100% function coverage. Interpreting 100% emitted-function coverage as complete public API coverage was fundamentally invalid.

### The four decoupled verification dimensions

To eliminate this conflation, the Engineering Standard (Section 86) and tooling now formally separate four distinct verification dimensions:

1. **LLVM Emitted-Code Coverage (`verify_coverage.py`)**:
   - *Proves*: Every function, line, and raw branch in compiled translation units was executed during test runs.
   - *Does NOT prove*: That all public API templates, supported scalar types, or Frame combinations were instantiated or tested.
2. **Header Isolation (`*_HeaderIsolation` targets)**:
   - *Proves*: Every public header can be included in isolation in an empty translation unit without missing prerequisites, syntax errors, or header guards.
   - *Does NOT prove*: That templates within the header can be instantiated with supported types or that methods function correctly.
3. **Public API Surface Verification (`verify_api_surface.py`)**:
   - *Proves*: Explicitly declared supported types (`float`, `double`), Coordinate Frames (`WorldFrame`, `BodyFrame`, `SensorFrame`), and SI base/derived quantities can be instantiated and executed against positive public contracts, and that invalid operations fail compilation via negative probes.
   - *Does NOT prove*: That all infinite conceivable C++ expressions or unsupported third-party types are valid.
4. **Static Analysis (`run_clang_tidy.py`)**:
   - *Proves*: All translation units in an explicitly validated compile database satisfy LLVM clang-tidy checks under repository `.clang-tidy`.
   - *Does NOT prove*: Dynamic runtime correctness, absence of floating-point overflow, or algorithmic convergence.

### Public API manifest and verification architecture (VRT-10A, VRT-10B)

A fail-closed manifest was created at `tools/api_surface/public_api_manifest.json`:
- **Header Set Accounting**: Declares all 55 public headers across `VectorisNumerics` (Core, Units, Geometry, Solvers).
- **3-Way Consistency Check**: `verify_api_surface.py` enforces exact 3-way equality:
  `Git tracked headers (55) == CMake public headers (55) == Manifest headers (55)`.
- **Classification Categories**:
  - `runtime_template_api`: 32 headers with explicit probe IDs and supported types
  - `declaration_only`: 6 headers
  - `concept_or_trait`: 6 headers
  - `constant_or_metadata`: 3 headers
  - `compile_time_api`: 4 headers
  - `internal_public_helper`: 4 headers
- **Supported Instantiation Matrix**:
  - Scalars: `float`, `double`
  - Frames: `WorldFrame`, `BodyFrame`, `SensorFrame`
  - Units: Length, Time, Mass, Velocity, Acceleration, Force, Torque, Inertia, Angle, Frequency, Dimensionless
- **Historical Gaps Explicitly Covered**:
  - `Matrix3 × Vector3`
  - `UnitVector3` accessors
  - `Result` named factories
  - `AlmostEqual` float/double
  - `LDLT` float/double
  - `Quaternion` checked conversion
- **Real Compilation & CTest Execution (VRT-10A)**:
  `verify_api_surface.py` compiles the positive test suite target via CMake and executes it through CTest:
  - Registered API Tests: 11
  - Executed API Tests: 11
  - Passed API Tests: 11
  - Failed API Tests: 0

### Negative constraint verification (VRT-10C)

To prevent arbitrary compiler crashes or missing include errors from producing false passes:
1. **Migrated to C++20 Concepts**:
   Three negative compile checks were converted to positive compile-time concept assertions in `PublicApiSurfaceTest.cpp`:
   - Cross-Frame vector addition (`!SupportsAddition<Vector3<double, WorldFrame>, Vector3<double, BodyFrame>>`)
   - Non-scalar matrix multiplication (`!SupportsMultiplication<Matrix3<double>, std::string>`)
   - Incompatible unit addition (`!SupportsAddition<Meter<double>, Second<double>>`)
   All three compile successfully and assert compile-time rejection cleanly.
2. **Compile-Fail Probe with Positive Control**:
   For `Result<T, T>` where class-level `static_assert` cannot be safely inspected via `requires`:
   - Control probe: `tools/api_surface/negative_probes/result_same_type_control.cpp` compiles successfully to prove compiler and include paths are valid.
   - Invalid probe: `tools/api_surface/negative_probes/result_same_type.cpp` must fail compilation and match the expected diagnostic pattern: `Result<T, E> where T == E must fail class-level static_assert.`
3. **Compiler Resolution Order**:
   Resolved in strict order: `--cxx` argument -> `CXX` env var -> `CMakeCache.txt` -> platform fallback (`/usr/bin/c++`).

### API surface gate self-test suite (VRT-10D)

Created `tools/api_surface/verify_api_surface_gate.py` with 13 deterministic self-tests using isolated fixtures:
- Missing manifest, empty manifest, malformed manifest JSON
- Duplicate header entry, header missing from manifest
- Nonexistent referenced positive probe, runtime_template_api with 0 probes
- Missing positive test suite source, zero discovered test definitions
- Positive API test failure propagation
- Negative probe unexpectedly accepted, broken negative-probe control
- Minimal valid controlled PASS
Results: **13/13 self-tests PASSED**.

### Clang-tidy fail-closed harness remediation (VRT-11A, VRT-11C)

Updated `tools/static_analysis/run_clang_tidy.py`:
1. **Independent Expected TU Derivation (VRT-11A)**:
   Independently parses `VectorisNumerics` and `VectorisDynamics` `CMakeLists.txt` to derive expected first-party TUs (99 TUs: 57 Numerics HeaderIsolation, 13 Dynamics HeaderIsolation, 18 Numerics tests, 11 Dynamics tests).
   - Expected first-party TUs: 99
   - Eligible first-party TUs: 99
   - Analyzed TUs: 99
   - Exact set comparison: `expected == eligible == analyzed`
   - Third-party GoogleTest TUs explicitly excluded: 4
2. **Explicit Authoritative Configuration (VRT-11C)**:
   - Config file: `.clang-tidy` (non-empty, verified existence)
   - Config SHA-256: `80e2167e7bc70b7bb5b61e881ebfdc08bac74f7b6e2ad0104929b0100581106f`
   - Explicit `--config-file` passed to every clang-tidy invocation.
3. **Fail-Closed Gate Assertions**:
   Fails if database is missing, empty, or malformed, if any required category is missing, if any expected TU is missing, if any unexpected TU is present, or if any duplicate TU exists.

### Clang-tidy gate self-test suite (VRT-11B)

Extended `tools/static_analysis/verify_clang_tidy_gate.py` to 12 deterministic negative/positive self-tests:
1. Missing compile_commands.json
2. 0-byte compile_commands.json
3. Empty JSON list [] compile_commands.json
4. Missing required project TU categories
5. Malformed compile command entry (missing 'file')
6. Missing .clang-tidy configuration file
7. Minimal valid controlled translation unit
8. Compile DB missing one expected project TU
9. Compile DB containing unexpected project TU
10. Duplicate project TU in compile DB
11. Expected-set derivation returns empty
12. Eligible-set derivation returns empty
Results: **12/12 self-tests PASSED**.

### Full static analysis execution

Executed `python3 tools/static_analysis/run_clang_tidy.py --build-dir .build/static-analysis`:
- Expected TUs: 99
- Eligible TUs: 99
- Analyzed TUs: 99
- Failed TUs: 0
- Production diagnostics: 0
- Result: **PASS**

### Principal regressions verification (VRT-01 through VRT-09)

All prior remediation regressions were re-verified:
- VRT-01 (Quaternion extreme scaling): PASS
- VRT-02 (UnitVector3 normalization & encapsulation): PASS
- VRT-03 (Result factory semantics): PASS
- VRT-04 (Matrix3 × Vector3 public multiplication): PASS
- VRT-05 (Sanitizer fail-fast gate): PASS (`tools/sanitizers/verify_sanitizers_gate.py`)
- VRT-06 (CI pipeline failure propagation): PASS (`tools/ci/verify_pipeline_gate.py`)
- VRT-07 (Dynamics transactional rollback): PASS (6/6 tests)
- VRT-08 (LDLT extreme scale & backward error): PASS (35/35 tests)
- VRT-09 (AlmostEqual extreme overflow & tolerance validation): PASS (20/20 tests)

### Final local verification matrix

Toolchain: AppleClang 21.0.0 (`clang-2100.1.1.101`), ISO C++20, CMake 4.4.3, GoogleTest v1.14.0, x86_64 Darwin.
Strict compiler warning flags: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` (0 warnings).

| Check | Result | Notes |
| :--- | :--- | :--- |
| Debug | 231/231 passed | Full test suite passed (7.89s) |
| Release | 229/229 passed | 2 assertion-only tests excluded by design (3.20s) |
| ASan | 231/231 passed | Fail-fast runtime options enabled, 0 diagnostics (37.45s) |
| UBSan | 231/231 passed | Fail-fast runtime options enabled, 0 diagnostics (10.83s) |
| ASan + UBSan | 231/231 passed | Fail-fast runtime options enabled, 0 diagnostics (38.76s) |
| Numerics HeaderIsolation | 57 standalone headers + 2 order TUs passed | 0 warnings |
| Dynamics HeaderIsolation | 13 standalone headers + 2 order TUs passed | 0 warnings |
| clang-tidy exact-TU gate | 99/99 TUs passed | Expected == Eligible == Analyzed (99), 0 failed, 0 diagnostics |
| clang-tidy gate self-test | 12/12 passed | `tools/static_analysis/verify_clang_tidy_gate.py` |
| Public API surface gate | PASS | 55 headers, 11 positive tests executed via CTest, 1 negative probe verified |
| Public API surface self-test | 13/13 passed | `tools/api_surface/verify_api_surface_gate.py` |
| Fresh RAW LLVM functions | 192/192 = 100% | Emitted-code function coverage gate satisfied |
| Fresh RAW LLVM lines | 1016/1027 = 98.93% | Emitted-code line coverage gate satisfied (>=95.0%) |
| Fresh RAW LLVM branches | 399/438 = 91.10% | Emitted-code branch coverage gate satisfied (>=90.0%) |
| Negative Sanitizers Gate | PASS | `tools/sanitizers/verify_sanitizers_gate.py` |
| Negative Pipeline Gate | PASS | `tools/ci/verify_pipeline_gate.py` |
| git diff --check | PASS | Clean whitespace and diff formatting |

### Stop condition

VRT-01 through VRT-09: **IMPLEMENTED LOCALLY; NOT CLOSED**.
VRT-10: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-11: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-12 through VRT-19: **OPEN**.
Overall status: **NOT REQUALIFIED / Experimental**.

No commit or push was performed. Stop after VRT-10 and VRT-11. Do not begin VRT-12.



## Step 10 — VRT-12

### Baseline and preservation

Audited starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before editing, recorded status, HEAD, diff/stat and SHA-256 snapshots of all
42 pre-existing modified/untracked remediation files. All prior ledger entries
remain verbatim; this entry is appended after the combined VRT-10/VRT-11 step.

The 55 pre-existing Numerics header bodies were mechanically compared with
baseline, removing only the new contract includes, the three relocated aliases,
the selective AlmostEqual using-declaration and EOF whitespace. All bodies match.
No numerical formula, class storage, constructor, error handling, template
constraint, Dynamics implementation or prior regression body changed. Existing
CI workflow, sanitizer/pipeline gates, coverage verifier, clang-tidy harness,
clang-tidy self-tests and `.clang-tidy` configuration remain byte-identical.
The prior API test source is preserved as a prefix, followed by two new tests.

Necessary prior-file changes: namespace includes in nine previously modified
Numerics headers; namespace documentation in the SSOT/core/geometry/Dynamics;
Numerics CMake registrations; API surface test/manifest/verifier/self-tests;
and the ledger overview plus this appended entry. The evidence preservation
manifest records exact before/after hashes and every touched file. No commit,
push, reset, rebase or VRT-13 work.

### Pre-edit reproduction

All probes used AppleClang 21, ISO C++20 and the six strict warning flags.
Standalone probes included only the target public header and required standard
types; no umbrella or unrelated Vectoris header supplied an alias.

| Pre-fix probe | Result |
| --- | --- |
| Core/Math.h -> core::sqrt | FAIL: core namespace unavailable |
| Core/NumericTraits.h -> core::AlmostEqual | FAIL: core namespace unavailable; source audit also found no Core re-export |
| Core/Result.h -> core::Result | PASS via the existing MathError include |
| Units/Quantity.h -> units::Quantity | PASS via UnitConcepts |
| Units/Dimension.h -> units::LengthDimension | FAIL |
| Geometry/Vector3.h -> geometry::Vector3 | PASS via FrameTags |
| Geometry/CoordinateConvention.h -> geometry::SystemConvention | FAIL |
| All old direct headers: Core / Units / Geometry | 2/10, 25/30, 11/15 namespace probes passed; 38/55 total |

For each layer, explicitly placing MathError/UnitConcepts/FrameTags before the
first lowercase use made the corresponding probe pass; placing that provider
after the first use failed. These provider-dependent probes are separate from
the standalone checks. An initial local Quantity fixture lacked IsBaseUnit; the
fixture was corrected and rerun before any production edit. It was not counted
as a namespace defect.

### Canonical policy and rationale

**Option B:** `vectoris::numerics::core`, `units`, and `geometry` are the canonical
public spellings. `Core`, `Units`, and `Geometry` remain compatibility spellings
for this release, **not deprecated**; no deprecation attribute is added.

SSOT §8 explicitly shows lowercase units/geometry/dynamics. Core documentation
already contains lowercase core, while module documentation, Dynamics examples
and existing API tests mix historical PascalCase spellings. This inconsistency
is resolved explicitly in SSOT §8 and the module documentation. It does not
justify relocating definitions or breaking existing PascalCase callers.
SSOT §56 continues to exclude implementation Detail namespaces from supported API.

### Namespace architecture and compatibility

Three authoritative headers declare only their own existing namespace and alias:

- `Core/Namespace.h`: `namespace Core {}; namespace core = Core;`
- `Units/Namespace.h`: `namespace Units {}; namespace units = Units;`
- `Geometry/Namespace.h`: `namespace Geometry {}; namespace geometry = Geometry;`

Every one of the 55 existing Numerics public headers directly includes its own
layer's contract. The old declarations in MathError.h, UnitConcepts.h and
FrameTags.h are removed. Each alias is declared at exactly one location.
The three new contract headers bring the public inventory to **58** (11 Core,
31 Units, 16 Geometry). Dynamics public headers are unchanged.

NumericTraits.h selectively introduces `using Traits::AlmostEqual` in Core,
providing both requested core::AlmostEqual and compatibility Core::AlmostEqual.
The original Traits::AlmostEqual remains the very same function, as verified by
function-pointer identity assertions; there is no forwarding implementation or
new numerical behavior. Root scalar/Concepts/Constants/Traits locations remain.
The separate existing core::sqrt and core::Math::sqrt functions are not merged.

Namespace aliases naturally make existing nested Detail names reachable under
both spellings when their defining header is included. This is intentional
same-namespace lookup, not a new supported Detail contract. No new detail alias,
wholesale Traits import, hidden implementation definition, or alias under
vectoris::dynamics was added. A controlled negative probe rejects
`vectoris::dynamics::geometry` and passes the numerics-root control.

This is an additive source API correction. Existing PascalCase entity identity,
ADL, function signatures and specialization placement remain. No definition or
object layout is relocated, and aliases introduce no runtime code or independent
binary symbols. Header-only consumers must distribute the three new headers and
recompile. These observations are not an ABI stability or binary qualification
guarantee; the project remains Experimental.

### Executed namespace contract tests and gate integration

- Standalone namespace checks: **58/58** passed after repair.
- Every generated Numerics HeaderIsolation TU checks canonical and compatibility
  lookup after its own direct include.
- Thirteen direct-header symbol snippets instantiate float and double APIs:
  sqrt, AlmostEqual, Result, Quantity, unit_cast, base Length, derived Velocity,
  Vector3, Matrix3, UnitVector3, Quaternion, Transform3 and SPD solve. Type or
  function identity checks prove the compatibility spelling denotes the same
  entity. Only the target public header and standard type_traits are included.
- Six ordered layer pairs and all six triple permutations compile as twelve
  explicit test-target TUs. Lookup is checked immediately after each include,
  before later includes could mask a missing alias. Existing complete forward
  and reverse header-order TUs also pass.
- Two fully-qualified runtime cases cover float/double, in addition to the prior
  eleven API cases. PascalCase representative calls and equality of type/function
  identities remain tested; no using-directive is needed by the new calls.
- The API manifest now records each header's canonical and compatibility paths,
  the new headers and direct probes. The API gate builds HeaderIsolation as well
  as its runtime test target, checks namespace metadata/probe existence and
  executes the two controlled negative probes.
- Git inventory includes cached and nonignored untracked public headers to
  validate this expressly uncommitted tree. Exact Git/CMake/manifest equality
  remains mandatory; an untracked undeclared header fails a new self-test.
- All original thirteen VRT-10 self-tests are retained; three namespace-related
  failure cases bring the suite to sixteen. VRT-11 harness/self-test semantics
  are unchanged. Explicit CMake source registration lets the existing independent
  expected-set derivation discover the twelve new order TUs and three new headers.

### Final local verification — 2026-09-22

AppleClang 21.0.0 (`clang-2100.1.1.101`), ISO C++20, CMake 4.4.3, GoogleTest
v1.14.0, x86_64 Darwin. The six strict flags remain
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`, without new suppression.

| Check | Result |
| --- | --- |
| Debug | 233/233 passed |
| Release | 231/231 passed; two existing assertion-only tests excluded |
| ASan | 233/233 passed |
| UBSan | 233/233 passed |
| ASan+UBSan | 233/233 passed |
| Numerics HeaderIsolation | 58 direct headers + 2 complete-order TUs passed |
| Dynamics HeaderIsolation | 11 direct headers + 2 complete-order TUs passed |
| Public API surface | 58/58 header inventory; 13/13 runtime tests; 2/2 controlled negative probes |
| VRT-10 gate self-tests | 16/16 passed, including all original 13 |
| clang-tidy exact TU gate | Expected=eligible=analyzed=114; failed=0 |
| VRT-11 gate self-tests | 12/12 passed |
| Principal VRT-01/02/03/04/07/08/09 + public API regressions | 115/115 passed |
| VRT-01 Quaternion | 6/6 passed |
| VRT-02 UnitVector3 | 16/16 passed |
| VRT-03 Result | 9/9 passed |
| VRT-04 Matrix/Vector | 10/10 passed |
| VRT-05 sanitizer negative gate | PASS |
| VRT-06 pipeline negative gate | PASS |
| VRT-07 Dynamics | 6/6 passed |
| VRT-08 LDLT | 35/35 passed |
| VRT-09 AlmostEqual | 20/20 passed |
| Fresh RAW LLVM functions | 192/192 = 100% |
| Fresh RAW LLVM lines | 1016/1027 = 98.9289% |
| Fresh RAW LLVM branches | 399/438 = 91.0959% |
| Compiler warnings / static diagnostics | 0 / 0 |
| git diff --check | PASS |

Sanitizers use `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, retaining no-recovery UBSan.
The exact first-party TU population is 60 Numerics isolation + 13 Dynamics
isolation + 30 Numerics tests + 11 Dynamics tests = 114; four GoogleTest TUs are
excluded by the unchanged classification policy. No exact-set bypass was used.

The initial static invocation found no Debug compilation database; the initial
self-test invocation detected stale static-analysis compilation data. Both
failed closed. Reconfiguring the static-analysis build generated the current
exact TU set; final gate and self-tests pass without harness/configuration edits.
Initial setup-failure logs are retained alongside final evidence.

Coverage profiles were freshly cleaned and collected. Raw denominator policy and
100% / 95% / 90% gates remain unchanged. Emitted-code coverage and the declared
API-surface probes are separate evidence; neither proves every possible template
instantiation. No Windows/pwsh execution, remote CI or independent review is
claimed. No performance benchmark was needed for namespace-only production edits.

Documentation changes: SSOT namespace policy; core, units and geometry canonical/
compatibility rules, header self-sufficiency and include-order guarantee; Dynamics
namespace notation; core comparison naming clarification. Earlier numerical
contracts and historical remediation evidence are preserved.

Evidence: `Vectoris_Remediation/step-12` in the local audit artifact directory,
including baseline hashes, isolated incremental patch, exact final sources,
namespace probes, all gate/build logs, raw coverage, and SHA-256 manifest.

### Stop condition

VRT-01 through VRT-11: **IMPLEMENTED LOCALLY; NOT CLOSED**.
VRT-12: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.
VRT-13 through VRT-19: **OPEN**.
Overall: **NOT REQUALIFIED / Experimental**.
Stop after VRT-12. No commit or push; VRT-13 was not begun.

## Step 11 — VRT-13 only

### Scope, baseline and preservation

Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996` (unchanged).
Captured `git status`, `git rev-parse HEAD`, `git diff --stat`, `git diff`, and
SHA-256 hashes plus exact copies of all 120 existing remediation files before
production edits. The incremental patch is limited to Result, its tests/API
manifest and documentation. Of those 120 files, 113 remain byte-identical;
the seven modified existing files are Result.h, the Numerics test registration,
PublicApiSurfaceTest.cpp (append only), the API manifest, SSOT §45, core.md, and
this ledger's current overview plus appended Step 11. Steps 1–10 (findings
VRT-01 through VRT-12) remain verbatim. All prior numerical implementations,
namespace contracts, CI/sanitizer/coverage gate code, API gate code, clang-tidy
exact-TU gate code and .clang-tidy configuration remain unchanged. The VRT-03
factory bodies and all original ResultTest.cpp tests remain unchanged.

No commit, push, reset, rebase or VRT-14 work was performed. All earlier findings
remain implemented locally and NOT CLOSED, pending CI / independent review.

### Pre-fix reproduction and accessor audit

A deterministic payload injects exceptions into copy/move constructors,
copy/move assignments and conversion construction. Actual unmodified production
headers demonstrate that ERROR→VALUE and VALUE→ERROR copy **and** move assignments
can leave both `value_if()` and `error_if()` null. `has_value()`, `IsSuccess()` and
bool conversion all report false. A separate temporary header copy with only
read-only diagnostic accessors confirms `valueless_by_exception()==true` and
`index()==variant_npos` (18446744073709551615 locally). Production headers were
not instrumented. Same-alternative throwing assignments preserve their active
alternative. Failed Result construction/factory conversion creates no destination;
it is not reported as a valueless reproduction.

In corrupted historical storage, `value()`/`Value()` would fail the Debug value
precondition, whereas `error()` would pass `!has_value()` and dereference null.
These unsafe reference accesses were audited, not executed. There is no `Error()`
member. After the patch, the original reproducer's unsafe assignments are rejected
at compile time; its raw diagnostics and exit status are preserved.

### Selected policy and proof

**Option A: constrain unsafe operations, retain throwing construction.**
T/E are distinct variant-compatible types with nonthrowing destructors.
`Result<T,T>` stays prohibited. No blanket copy/move/default-construction
requirement is added. The storage remains one private `std::variant<T,E>`;
no allocation, sentinel, third state, termination strategy or exception repair
is introduced. The class remains final, with no public storage/emplacement API.

- Named factories retain explicit index 0/1 and perfect forwarding. Selected
  converting/in-place constructors and factories retain construction-dependent
  `noexcept`; exceptions propagate before a destination Result exists.
- Copy/move constructors are explicitly defaulted; their actual variant traits
  determine availability and exception specifications. A failed construction
  leaves no destination; the source keeps its alternative and may be moved-from.
- Copy assignment requires variant copy assignability and, for **each** alternative,
  nonthrowing copy construction **or** nonthrowing move construction. Cross-state
  construction therefore either cannot throw, or copies a temporary first and
  commits through nonthrowing move. Failed cross-state copy leaves the destination
  unchanged.
- Move assignment requires variant move assignability and nonthrowing move
  construction of **both** alternatives. Cross-state move construction cannot throw.
- Enabled assignments are defaulted; unsafe overloads are explicitly deleted,
  including rvalue assignment to prevent fallback to copy. Same-state payload
  assignment can throw, retains the original alternative, and follows the payload's
  own assignment guarantee. It does not promise rollback of payload content.
- Defaulted operations infer their real `noexcept`. Observers/reference accessors
  remain `noexcept`; `value_or` can propagate payload/fallback construction errors.
  Wrong-alternative caller misuse retains its existing precondition policy. Internal
  loss of both alternatives is prevented, rather than detected after corruption.

Successful construction establishes exactly one alternative. The only operations
that replace it now have safe construction routes; same-state assignment never
destroys the alternative. This inductive argument covers every supported operation,
including moved-from Results. Payloads must obey declared exception specifications
and normal C++ lifetime rules. Mutable payload references cannot change the private
variant discriminator through valid operations.

### Tests and exact counts

Result-prefixed GTest discovery/execution/pass/failure: **62 / 62 / 62 / 0** in
Debug and each sanitizer build; **60 / 60 / 60 / 0** in Release (two Debug-only death
tests). A broad `ctest -R Result` run selected 64 cases because it also includes two
PublicApiSurfaceTest cases. The new ResultExceptionTest.cpp has 36 cases:

| Category | Exact count and scope |
| --- | --- |
| Successful copy/move transitions | 8: VV, VE, EV, EE for both copy and move; all pass. |
| Exception cases | 16: 6 assignment exceptions, 4 Result constructor exceptions, 1 conversion test covering five constructor/factory entries, 3 value_or cases, 2 forwarding factory exception cases. |
| Assignment exceptions | 4 copy directions and 2 same-state move directions. Unsafe cross-state throwing-move assignment is compile-time rejected. |
| Compile-time trait groups | 8 ResultConstraintTest cases; additionally 1 public API type-matrix case. |
| Controlled Result negative probes | 5, each with a positive control: unchanged T==E, unsafe copy, unsafe move, throwing T destructor, throwing E destructor. |
| Factory forwarding | 7 named cases: 4 new ResultForwardingTest cases and 3 unchanged VRT-03 category/dual-conversion/move-only cases. The 2 factory exception tests are counted in the 16 exception cases. |
| VRT-03 named-factory regressions | All 9 unchanged ResultFactoryTest cases pass. |

Categories are explicitly scoped and can overlap; their counts are not summed as
disjoint test totals. Both alternatives are exercised for throwing copy/move
construction, same-state throwing copy/move assignment, strong cross-state copy
failure, conversion failure, reference-qualified observers and moved-from state.
The tests deliberately change payload content before same-state assignment throws,
so they verify the basic guarantee without implying rollback. Move-only and
non-default T/E remain supported; additional immovable in-place construction works.
Nothrow-copy / throwing-move payloads permit copy assignment and reject move
assignment, proving the constraints are not a blanket noexcept ban.

The API manifest declares the supported Result matrix, adds
CoreResultTwoStatePayloadPolicy and four controlled negative probes, and preserves
the original same-type and namespace-root gates. No verification harness is weakened.

### Full local verification

| Verification | Result |
| --- | --- |
| Debug | 270/270 passed |
| Release | 268/268 passed; two Debug-only death tests absent by existing policy |
| ASan | 270/270 passed |
| UBSan | 270/270 passed |
| ASan+UBSan | 270/270 passed |
| Numerics HeaderIsolation | 58 standalone + 2 order TUs passed |
| Dynamics HeaderIsolation | 11 standalone + 2 order TUs passed |
| VRT-12 namespace | 58/58 standalone namespaces; 13 direct-symbol probes and all 12 pair/triple permutations retained and compiled; direct/provider-order reproducer probes passed |
| Public API gate | 58/58 headers; 14 discovered/executed/passed, 0 failed; 6/6 controlled negative probes |
| VRT-10 gate self-tests | 16/16 passed |
| clang-tidy exact TU | 115 expected = 115 eligible = 115 analyzed; 0 failed TUs; 0 final gate diagnostics |
| VRT-11 gate self-tests | 12/12 passed |
| VRT-05 sanitizer fail-fast gate | PASS, unchanged |
| VRT-06 CI failure-propagation gate | PASS, unchanged |
| Principal prior/API regressions | 116/116 passed |
| Compiler warnings | 0 under unchanged -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror |
| git diff --check | PASS; new/incremental untracked-file whitespace also checked |

All sanitizer test runs used `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`; UBSan build flags retain
`-fno-sanitize-recover=undefined`. No CI gate is claimed closed by local execution.

First full static analysis reported two moved-from-object diagnostics in new tests
that intentionally observe Result after moving out its unique_ptr payload. Narrow,
explained test-only NOLINT annotations cover these valid contract checks; no moved-from
unique_ptr is dereferenced. Other targeted moved-from checks are likewise documented.
The complete exact-TU gate was then rerun successfully. Checker configuration,
production diagnostics policy and fail-closed verification code are unchanged.
Initial and final raw diagnostics remain in the evidence archive.

Fresh LLVM production coverage, without denominator subtraction:

| Metric | Raw covered / total | Percentage |
| --- | --- | --- |
| Functions | 198 / 198 | 100% |
| Lines | 1025 / 1036 | 98.9382239382% |
| RAW branches | 402 / 443 | 90.7449209932% |

All unchanged thresholds pass (100% functions, >=95% lines, >=90% raw branches).
Emitted-function coverage does not prove every public template/API instantiation.
The declared API surface remains a separate gate. Local LLVM also reports
1297/1414 emitted instantiations (91.7256011315%); this is not a new normative gate.

### Compatibility, layout and documentation

Unsafe assignments previously accepted are now compile-time errors; potentially
throwing payload destruction is rejected at class instantiation. Throwing construction
and safe throwing assignment remain supported. No numerical/Dynamics call-site change
was needed. Generic code relying on removed assignments (including swap built on them)
must adapt to the supported operation matrix. This is an intentional source/template
compatibility restriction; it is not an ABI stability claim.

Local before/after observations are identical:

| Result specialization | sizeof | alignof | standard layout | trivially copyable |
| --- | --- | --- | --- | --- |
| int | 8 | 4 | true | true |
| float | 8 | 4 | true | true |
| double | 16 | 8 | true | true |
| Matrix3<double> | 80 | 8 | true | true |

Storage has no additional data members. Header-only consumers must rebuild against
the new constraints; no cross-compiler/platform binary or ABI guarantee is provided.
SSOT §45 and core.md now specify the two states, payload/operation requirements,
factory semantics, copy/move exception guarantees, observer/accessor preconditions
and T==E prohibition. Prior numerical and namespace contracts are preserved.

### Evidence and final status

Full baseline snapshots, before/after reproducer, implementation-only patch,
source hashes, operation audit, category counts, raw logs, compile database and
83-item report are archived in:
`[REDACTED_AUDIT_ARTIFACT_PATH]/Vectoris_Remediation/step-13`.

VRT-01 through VRT-12: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.

```text
VRT-13:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-14 THROUGH VRT-19:
OPEN
OVERALL:
NOT REQUALIFIED / Experimental
```

STOP after VRT-13. No VRT-14 remediation is included or started.

## Step 12 — VRT-14

### Scope, baseline and preservation

Only the sqrt API/semantic consistency finding is remediated in this step.
Starting HEAD remains `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
Before production editing, status, SHA, diff/stat and SHA-256 snapshots of all 129
existing dirty/nonignored-untracked remediation files were recorded. Five other
modified files were initially clean; their exact pre-state was recovered from HEAD
and is explicitly labeled as such. Of the 129 prior files, 119 remain byte-identical;
the ten changed shared files contain only this sqrt implementation, tests, contract,
manifest, test registration and ledger additions. All 31 explicitly protected
numerical/namespace/gate files match their hashes. Geometry and Dynamics production
headers, Result, AlmostEqual and verification harnesses are unchanged. Historical
Steps 1–11 (VRT-01 through VRT-13) are preserved verbatim.

### Inventory and measured pre-fix behavior

The exact inventory contains 254 sqrt occurrences in 36 files: 25 in production
headers/API/internal code, 164 in tests and 65 in documentation, with no benchmark
or tooling occurrences. These include comments/symbols, not only executable calls.
Two public API families exist: Core::sqrt (Math.h, constrained float/double,
constexpr Newton and runtime std::sqrt) and Core::Math::sqrt (MathFunctions.h,
unconstrained runtime-only wrapper, including working long-double runtime support).
Lowercase core is the existing VRT-12 namespace alias, not a third implementation.
Internal InitialSqrtGuess and BoundedNewtonSqrt formed the old constexpr path;
Constants::Sqrt2 is a constant, not an algorithm.

Before edits, 147 measured records covered 21 inputs for float/double across Core
runtime, Core constexpr and Math runtime, plus long-double Math runtime. Both paths
returned +0 for negative finite values, negative subnormals, -Inf and -0. +Inf and
NaN remained infinite/NaN; all sampled positive subnormals produced nonzero roots.
Core constexpr/runtime differed by up to 1 ULP in the boundary sample for both
float and double. Math constexpr was independently rejected as non-constexpr.
The CSV records hexadecimal inputs/results, signbit and finite/NaN/Inf classification.

SSOT §§15/17 favor IEEE semantics; §16 does not authorize negative-radicand clamping.
core.md and README attributed constexpr/Newton constraints to the wrong wrapper;
DEVIATIONS.md repeated an unproven <=6-step claim. Existing tests asserted the old
clamp. The manifest lacked explicit runtime/constexpr sqrt probes. Current docs
are corrected; historical records retain their original context.

### Canonical contract and implementation

Canonical API: `vectoris::numerics::core::sqrt`. PascalCase Core is retained under
VRT-12. `core::Math::sqrt` / `Core::Math::sqrt` are nondeprecated compatibility-only
wrappers, forwarding with a fully qualified call to the single canonical dispatch.
Both APIs accept deduced float, double and long double; integral/bool/user-conversion
arguments are rejected by constraints. Explicit caller conversions remain intentional.
Long-double runtime support preserves the existing documented UnitVector3 surface.
Float/double support portable C++20 constexpr; general positive-finite long-double
constexpr evaluation is not promised.

| Input | Result |
| --- | --- |
| +0 / -0 | Preserve zero and sign |
| Positive finite, including subnormal | Finite positive square root |
| Negative nonzero finite / -Inf | Quiet NaN |
| +Inf | +Inf |
| NaN | NaN; no payload/sign guarantee |

The API remains scalar and noexcept. No Result conversion or hidden sentinel is
introduced. errno, floating exception flags, signaling NaN behavior and NaN payload
are outside the constexpr/runtime equivalence guarantee.

Runtime positive inputs use std::sqrt. Binary32/64 constant evaluation now extracts
integer root digits in exactly 24/53 steps. For normalized p-bit significand M and
exponent e, the radicand M*2^(p-1+parity(e)) is streamed as bit pairs, never formed as
a 106-bit object. Each step preserves prefix=root²+remainder and
0<=remainder<2*root+1. Round up iff remainder>root; the integer radicand cannot be an
exact halfway square. Integer intermediates stay below 2^55. Reconstruction uses a
normal power of two with exponent [-98,40] for float or [-589,459] for double.
There is no x*x, reciprocal of the input, convergence cutoff or fallback result.
The old internal Newton helpers are removed. See core.md §10 for the derivation.

Geometry audit found three production call sites: UnitVector3 TryCreate scaled
norm, Quaternion TryCreate scaled norm and Quaternion Slerp. All reach the canonical
API through the forwarding wrapper; zero Geometry call-site edits were necessary.
Vector3 currently has no norm/sqrt API; RotationMatrix3 and Transform3 have no direct
sqrt; LDLT is sqrt-free. All their algorithms remain unchanged.

### Tests and accuracy evidence

31 sqrt-selected cases passed: 27 new contract/property cases, three new public API
cases and one prior constexpr case. Categories overlap: 23 exercise runtime,
9 constexpr and one type-constraint case. Primary negative-domain and NaN/Inf
regressions are five cases each. Both paths cover float/double boundary, denorm_min,
multiple subnormals, largest subnormal, min normal and max finite. Long-double
runtime and Geometry normalization compatibility are included.

Materialized constexpr tables contain 1,116 results. Property tests cover every
representable power of two and neighbors, 32,768 deterministic positive bit samples
per binary type and 2,048 long-double exponent samples. Dynamic execution of the
integer backend is included under sanitizers. Independent std::sqrt comparisons
use <=1 ULP (spacing toward +Inf), round-to-nearest and gradual underflow. The gated
XML records 223,227 comparisons and worst observed error 0 ULP. A separate local
prototype sampled one million values per binary type and also observed 0 ULP.
Neither workload is an exhaustive all-float proof or a cross-platform bitwise promise.

The first fresh coverage run exposed missing runtime coverage of unchanged Core::abs,
formerly called inside Newton sqrt. Direct negative/positive/zero runtime assertions
were added to the existing wrapper regression. The original failure and final pass
are both archived; no coverage gate or denominator rule changed.

### Full local verification

| Verification | Result |
| --- | --- |
| Debug | 291/291 passed |
| Release | 289/289 passed; two existing Debug-only Result death tests absent |
| ASan | 291/291 passed |
| UBSan | 291/291 passed |
| ASan+UBSan | 291/291 passed |
| Numerics HeaderIsolation | 58 standalone + 2 order TUs passed |
| Dynamics HeaderIsolation | 11 standalone + 2 order TUs passed |
| VRT-12 namespaces | 58/58 standalone namespaces; 14 integrated direct-symbol probes; 12 pair/triple permutations; 71 external probes passed |
| Public API gate | 58/58 headers, 17/17 positive cases, 12/12 controlled negatives |
| VRT-10 self-tests | 16/16 passed |
| clang-tidy exact TU | 116 expected = 116 eligible = 116 analyzed; 0 failed; 0 diagnostics |
| VRT-11 self-tests | 12/12 passed |
| VRT-05 sanitizer gate | PASS, unchanged |
| VRT-06 CI failure-propagation gate | PASS, unchanged |
| Principal VRT-01..12/API regressions | 119/119 passed |
| VRT-13 Result regressions | 62/62 passed |
| Compiler warnings | 0 under unchanged strict warning policy |
| git diff --check | PASS; incremental untracked-file whitespace also checked |

Sanitizer execution used ASAN_OPTIONS=halt_on_error=1:abort_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1. UBSan flags retain
-fno-sanitize-recover=undefined. Coverage uses fresh profiles and unmodified raw totals:

| Metric | Raw covered / total | Percentage |
| --- | --- | --- |
| Functions | 198 / 198 | 100% |
| Lines | 1015 / 1027 | 98.8315481986% |
| RAW branches | 395 / 435 | 90.8045977011% |

All thresholds pass. Emitted-function coverage does not establish every public
API/template instantiation; the API-surface gate remains separate. No denominator
subtraction or normalization is applied.

### Compatibility, documentation and evidence

Floating-point source calls and namespace compatibility remain supported. The old
unconstrained Math wrapper's deduced integral/bool/user-conversion calls are now
rejected. Canonical long-double runtime support is added; the compatibility wrapper
retains it. Float/double constexpr remains supported and is newly available through
the compatibility wrapper. Negative nonzero/-Inf -> NaN and -0 preservation are
intentional behavioral corrections. Private Detail helper users have no support
contract. No object data members/layouts changed; header-only consumers should
rebuild. No binary/ABI stability guarantee is made.

Documentation updates: SSOT §15, core.md, geometry.md (append-only dependency audit),
README, PURE_MATH_SCOPE and sqrt-specific DEVIATIONS corrections. Result and
AlmostEqual contracts remain byte-identical. API manifest updates add scalar and
constexpr matrices, three positive cases, six controlled negative pairs, and a
MathFunctions direct-header namespace probe; harnesses are unchanged.

Full baseline snapshots, inventory, before/after CSVs, decisions, source hashes,
implementation-only patch, all raw logs, coverage and the 91-item report are archived:
`[REDACTED_AUDIT_ARTIFACT_PATH]/Vectoris_Remediation/step-14`.

VRT-01 through VRT-13: **IMPLEMENTED LOCALLY; PENDING CI / INDEPENDENT REVIEW; NOT CLOSED**.

```text
VRT-14:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-15 THROUGH VRT-19:
OPEN
OVERALL:
NOT REQUALIFIED / Experimental
```

STOP after VRT-14. No commit, push, reset, rebase or VRT-15 remediation.

## Step 15 — VRT-15

### Baseline and preservation

- Starting HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`.
- Before VRT-15 edits, the working-tree status, HEAD, diff statistics, full diff,
  and SHA-256 snapshots for 148 existing remediation files were saved in the
  step-15 evidence archive. The tracked VRT-01 through VRT-14 working-tree
  content was preserved; changes made in this step are scoped to Matrix3
  representation governance, layout terminology, tests, and benchmark evidence.
- No commit, push, reset, or rebase was performed. No VRT-16 work was started.

### Matrix3 representation audit

`Geometry::Matrix3<T>` remains a general unframed 3×3 matrix with public
`T m[9]`. Before the change, a standalone client assigned through `m.m[0]`
successfully and stored a quiet NaN. That demonstrates public mutation; it does
not violate a Matrix3 invariant because the type does not promise finite values.
It is not `RotationMatrix3`, and Matrix3 itself promises no orthogonality,
positive-definiteness, or finite-value lifetime invariant.

The public contract is nine scalar values in row-major order
`m[row * 3 + col]`; the built-in array is contiguous. Mutable and const
`operator()(row, col)` return `T&` and `const T&`. Indexing is unchecked and
requires `row < 3 && col < 3`. There is no `data()` member. A const Matrix3
exposes const indexed access through its public array and const call operator.
Validation remains with numerical consumers such as `TryInverse`,
`RotationMatrix3::TryCreate`, and the symmetric-positive-definite solver.

The source inventory counted direct `m[...]` lexemes rather than unique call
sites. The captured pre/post inventory reports 269/285 total matches across the
scoped source, test, benchmark, and documentation files. Code categories were:

| Category | Before | After |
| --- | ---: | ---: |
| Matrix3 implementation | 212 | 212 |
| Other production headers | 5 | 5 |
| Tests | 25 | 35 |
| Tools | 0 | 0 |
| Benchmarks | 0 | 2 |
| Documentation/audit references at inventory capture | 27 | 31 |

The five other-production references are in `RotationMatrix3.h` (1) and
`SymmetricLinearSolver3.h` (4). The test references span the Matrix3, solver,
comparison, and public-API tests. The VRT-15 representation test adds deliberate
coverage for mutation and row-major mapping. The exact per-file inventory JSON
is archived; documentation counts exclude this subsequently appended ledger
entry.

### Selected policy and layout

Option B was selected: retain public storage under the narrowly scoped
AML-DEVIATION-005. This preserves established source access while acknowledging
that callers can assign any `T` values. The rationale is based on Matrix3's
actual semantics: it has no stronger state invariant for public writes to
bypass. The deviation does not authorize public storage on other types.

For float and double, tests confirm standard-layout and trivially-copyable
properties, nine-scalar size, first-member offset, row-major mapping, contiguous
array elements, and mutable/const accessor types. On this local x86_64 target:
`Matrix3<float>` is 36 bytes / alignment 4; `Matrix3<double>` is 72 bytes /
alignment 8. These are C++ source/layout observations, not C ABI, cross-build
binary, DMA, persistent, or wire-format guarantees. No field, signature, or
object layout changed in this step.

### Benchmark policy and local baseline

The historical qualification report called missing benchmark evidence
non-blocking (`NO / P2-H`), while the Engineering Standard's performance
checklist and Definition of Done required evidence for performance-sensitive
kernels. The contradiction is resolved in current §§75, 104–106, 115, and 124:
representative Release evidence is required before declaring a
performance-sensitive kernel Stable. Timing remains separate from correctness.
A single local result is informative evidence, not a universal latency claim or
a CI regression threshold. A regression gate requires a qualified runner,
repeated baselines, and an accepted noise band. The current GitHub Actions
workflow has no benchmark job.

An optional, dependency-free Release benchmark target (`VECTORIS_BUILD_BENCHMARKS`,
default OFF) checks deterministic analytical results and an inverse residual
before timing. It uses a deterministic 64-entry corpus, 10,000 warm-up calls,
nine samples of 200,000 operations, `steady_clock`, a consumed checksum, and the
sample median. Local environment: AppleClang 21.0.0, `-O3 -DNDEBUG`, strict
warnings, Darwin x86_64, Intel Core i9-9980HK @ 2.40 GHz.

| Operation | Scalar | Iterations/sample | Samples | Median ns/op | Oracle |
| --- | --- | ---: | ---: | ---: | --- |
| Matrix × Matrix | double | 200,000 | 9 | 8.834 | Fixed analytical product |
| Matrix × Vector | double | 200,000 | 9 | 2.783 | Known vector result `(2,-6,16)` |
| determinant | double | 200,000 | 9 | 1.980 | Known determinant `110` |
| `TryInverse` | double | 200,000 | 9 | 44.619 | Product compared to identity |
| element access | double | 200,000 | 9 | 1.243 | Known row-major entries |

All correctness oracles passed and the checksum was finite. These values are one
machine/build baseline only. No before/after speedup is claimed because VRT-15
did not change numerical operations or storage layout.

### Verification

- Debug: 294/294; Release: 292/292.
- ASan: 294/294; UBSan: 294/294; ASan+UBSan: 294/294. ASan and UBSan ran with
  fail-fast environment options; UBSan retains `-fno-sanitize-recover=undefined`.
- Numerics HeaderIsolation: 58 standalone public headers and 2 order-poison TUs
  passed. Dynamics HeaderIsolation: 11 standalone public headers and 2
  order-poison TUs passed.
- Public API surface: 58/58 headers, 18/18 positive probes and 12/12 controlled
  negative probes passed. VRT-10 gate self-tests: 16/16.
- clang-tidy exact set: 117 expected = 117 eligible = 117 analyzed; zero failed
  TUs and zero project diagnostics. VRT-11 self-tests: 12/12 after refreshing
  the build's compile database; the first attempt used stale generated data.
- VRT-05 sanitizer fail-fast gate: PASS. VRT-06 CI failure-propagation gate:
  PASS. VRT-15 focused representation/API/Result/multiplication regressions:
  36/36.
- Fresh LLVM coverage ran 258 Numerics tests and passed on raw counters:
  functions 198/198 (100.00%), lines 1015/1027 (98.83%), RAW branches
  395/435 (90.80%). No branch denominator subtraction was used.
- All five builds passed with zero compiler warning lines under the existing
  strict policy. `git diff --check` and untracked-file whitespace checks passed.
- Principal VRT-01 through VRT-14 regression suites were rerun in the five full
  configurations; the focused Result, Matrix3 multiplication, and API checks
  also passed. No numerical algorithm or CI/static-analysis gate semantics were
  changed.

```text
VRT-01 THROUGH VRT-14:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-15:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-16 THROUGH VRT-19:
OPEN
OVERALL:
NOT REQUALIFIED / Experimental
```

STOP after VRT-15. Do not begin VRT-16. No commit or push.

## VRT-16 — Dimensional correctness of Dynamics rotational operations

### Baseline and scope

Starting HEAD remained `cfbecc6390fb30d857f10f2516f39bb2ef75f996`. Before editing, the complete status, HEAD, diff stat, tracked diff, and SHA-256 manifest for all 154 pre-existing changed/untracked files were saved under the Step 16 evidence directory. The VRT-01 through VRT-15 work remains present; only rotational dimensional behavior, its tests/API declaration, directly relevant documentation, and this appended ledger section changed for VRT-16. No Numerics algorithm, CI gate semantics, sanitizer gate semantics, coverage denominator, or clang-tidy policy changed.

### Finding and selected model

The old overload `Cross(AngularVelocity3<Frame>, Velocity3<Frame>)` extracted twelve scalar component values, multiplied/subtracted them, and constructed `Acceleration3`. Its numeric result for `omega=(0,0,2 rad/s)` and `v=(3,0,0 m/s)` was `(0,6,0)`, but strict dimensional multiplication is `A L T^-2`, not acceleration `L T^-2`. The lost Angle exponent occurred at `.value()` extraction and was never restored in the declared output type.

Generic `Cross` now performs only ordinary typed multiplication and preserves the resulting dimension and shared Frame. Physical rotational transport uses the explicit API `RotationalCross(omega, velocity)`, which computes the generic typed cross and divides by a typed `Quantity<T, RadianUnit>` whose scalar value is one. The existing gyroscopic `RotationalCross(omega, angularMomentum)` and `LieBracket` continue to divide by one radian and produce torque. `RigidBodyDynamicsKernel` now calls `RotationalCross` for its transport acceleration. `EulerIntegrator` forms `(angularVelocity * dt) / one_radian` before using a dimensionless angular increment in quaternion coordinates.

No position/angle-acceleration rotational-cross overloads were added: no corresponding production equation existed to migrate in this step. Generic `Cross(omega, position)` and `Cross(alpha, position)` remain available with their strict Angle-bearing dimensions.

### Dimensional audit and tests

Model B remains authoritative: Angle is an independent base dimension. The verified dimensions are AngularVelocity `A T^-1`, AngularAcceleration `A T^-2`, MomentOfInertia `M L^2 A^-2`, Torque `M L^2 T^-2 A^-1`, and AngularMomentum `M L^2 A^-1 T^-1`. Generic `omega × position` is `A L T^-1`; generic `omega × velocity` and `alpha × position` are each `A L T^-2`. The implemented physical `RotationalCross(omega, velocity)` is acceleration `L T^-2`; the implemented `RotationalCross(omega, angularMomentum)` is torque. Physical `omega × position`, `alpha × position`, and nested centripetal operations are not currently implemented and were not added.

Twenty VRT-16 compile-time assertions check base/derived dimensions, preservation of generic Angle exponents, the physical acceleration result, Frame identity, cross-Frame rejection, wrong-dimension/scalar rejection, and the deliberate absence of unused position overloads. Four new runtime tests cover explicit one-radian numerical equivalence, mixed signs, zero/parallel/anti-parallel/orthogonal cases, and finite large/small component products for float and double. Existing inertia/torque/angular-momentum identities and coupled rigid-body tests remain in the suite.

The Dynamics source contains 110 `.value()` occurrences before this step. Two formulas were identified as unit-erasing: the old special cross implementation and the angular-velocity/time quaternion boundary. Both are corrected; no unnormalized Angle-erasing cross or quaternion conversion remains. The 97 post-fix `.value()` occurrences are limited to validation/finite inspection, the documented same-unit scalar solver adapter that reifies AngularAcceleration in its typed signature, Result payload access, and extraction after the typed angular increment has become dimensionless.

### Compatibility and documentation

The misleading special `Cross(AngularVelocity3, Velocity3) -> Acceleration3` overload was removed. Callers relying on that overload must use `RotationalCross`; generic `Cross` now reports its true Angle-bearing result. `LieBracket` remains supported. The API is header-only; this changes source behavior/type selection but not object layout. No binary or ABI stability guarantee is introduced.

The Engineering Standard, Units guide, Dynamics guide, API manifest, and VRT-10 verifier now state and check the distinction between generic and physical rotational cross semantics. The declared Dynamics API contract is compiled and its positive test is executed by the API gate.

### Verification

- Debug 298/298; Release 296/296; ASan 298/298; UBSan 298/298; ASan+UBSan 298/298. Sanitizer test runs used fail-fast options.
- Numerics HeaderIsolation: 58 standalone public headers plus 2 include-order translation units. Dynamics HeaderIsolation: 11 standalone public headers plus 2 include-order translation units.
- Public API gate: 58/58 Numerics headers, 18/18 Numerics positive probes, 12/12 negative compile probes, plus the declared Dynamics `RotationalCross` contract probe. VRT-10 self-tests 17/17.
- Exact-TU clang-tidy: 117 expected = 117 eligible = 117 analyzed, 0 failed TUs, 0 diagnostics. VRT-11 self-tests 12/12.
- VRT-05 fail-fast sanitizer gate PASS; VRT-06 CI propagation gate PASS. VRT-12 namespace/header contract was exercised by both HeaderIsolation targets and API gate.
- Fresh Numerics LLVM coverage: functions 198/198 (100.00%), lines 1015/1027 (98.83%), raw branches 395/435 (90.80%). The coverage export excludes VectorisDynamics; these figures do not claim Dynamics runtime/template coverage.
- VRT-15 Release Matrix3 benchmark/oracle recheck completed locally; timings are informational and do not establish a speedup or cross-machine claim. Builds reported zero compiler warning lines. `git diff --check` and the changed-file whitespace audit passed.

```text
VRT-01 THROUGH VRT-15:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-16:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED
VRT-17 THROUGH VRT-19:
OPEN
OVERALL:
NOT REQUALIFIED / Experimental
```

No commit or push was made. VRT-17 was not started.

---

## Step 12 — VRT-17, VRT-18, and VRT-19 only

Starting audited HEAD: `cfbecc6390fb30d857f10f2516f39bb2ef75f996`. The initial
working-tree status, diff summary, full diff, and SHA-256 snapshot were captured
before this step in the external `step-17-19` evidence directory. The recorded
baseline contains 157 file hashes. All captured paths remain present; 16 changed
since that snapshot, all within the VRT-17/VRT-19 test, manifest, implementation,
and documentation scope. No unexpected baseline-file drift was found. The VRT-18
CMake changes and consumer fixture are also task-scoped. VRT-01 through VRT-16
remain implemented locally and not closed. No commit, push, reset, or rebase was
performed.

### VRT-17 — Quaternion and Transform identity factories

Before production changes, the exact Quaternion bypass
`Quaternion<double, From, To>::Identity<From, From>()` compiled and ran, returning
the identity quaternion despite the class representing `From -> To`. This
reproduced the finding. The old Transform3 spelling passed the caller-controlled
factory constraint but then failed while instantiating its body because its nested
cross-frame Quaternion rejected `Identity`; this was a declaration/body mismatch,
not a successfully returned invalid transform. `RotationMatrix3::Identity()` for
cross-frame tags compiled and returned the numerical identity matrix. That
factory has distinct matrix semantics and remains available; documentation now
requires callers to know that the coordinate bases are aligned before treating
that matrix as a valid frame rotation.

Quaternion and Transform3 factories now independently constrain the class's
actual `FrameFrom`/`FrameTo` types to match. For source compatibility, explicit
function-template arguments remain accepted when they themselves name a
same-frame pair, including a pair different from the class's same-frame type;
they never select the class mapping. Same-frame calls compile for float and
double through lowercase and PascalCase spellings. Cross-frame calls, including
explicit same-frame argument pairs, are rejected by `requires` checks. The public API regression
`GeometryIdentityFactoriesRespectClassFrames` checks the returned values, the
float/double and namespace spellings, compile-time rejection, and the deliberately
preserved cross-frame RotationMatrix3 identity behavior.

Other Quaternion factories and RotationMatrix3 factories were audited. Quaternion
`TryCreate` has no caller-supplied frame template parameters; RotationMatrix3
`TryCreate` and `FromQuaternion` likewise have no identity-style frame bypass.
No additional identity bypass was found. Correct same-frame code, including
explicit exact-frame template arguments, remains source-compatible. Code relying
on the invalid cross-frame Quaternion identity is intentionally rejected. No
object data or layout changed.

### VRT-18 — CMake contract

The direct root configure minimum remains CMake 3.14, justified by
`FetchContent_MakeAvailable`. The checked-in preset file uses schema version 6
and declares its separate 3.25 minimum. Both public interface targets,
`Vectoris::Numerics` and `Vectoris::Dynamics`, propagate `cxx_std_20`; Vectoris's
own targets also retain extensions-off policy. An external `add_subdirectory`
consumer linked both targets without choosing a C++ standard and compiled a
`__cplusplus >= 202002L` assertion. Separate minimal consumers linking only
Numerics and only Dynamics also built and ran with that assertion; both compiler
commands used `-std=gnu++20`. With consumer extensions disabled, the consumer
used `-std=c++20` and also built and ran.

A fresh consumer configure with `BUILD_TESTING=OFF` completed cleanly, built and
ran the consumer, registered zero CTest tests, and contained no GoogleTest or
`_deps` directory. GoogleTest, tests, HeaderIsolation, coverage and static-analysis
test infrastructure remain guarded by standard CTest semantics. The five
test-enabled qualification presets still configure and pass. The repository
currently supports the `add_subdirectory` consumption model; this step adds no
`find_package`, install/export, binary-compatibility, or ABI guarantee. One
exploratory configure passed an unused `FETCHCONTENT_FULLY_DISCONNECTED` command
line variable and CMake warned about that unused variable; the final clean
configure omitted it and emitted no CMake warning.

### VRT-19 — Documentation and coverage truthfulness

Current README and specification pages state `NOT REQUALIFIED / Experimental`.
The R1 report and predecessor compliance report remain historical and explicitly
superseded. The R1 report's two inconsistent historical baseline hashes are
disclosed rather than silently reconciled. The README's legacy TODO row no longer
implies a current stable-core qualification.

Current docs distinguish emitted-function LLVM coverage from HeaderIsolation,
the declared Public API surface gate, and the exact-TU clang-tidy gate. Public API
and namespace docs state lowercase canonical Numerics namespaces and PascalCase
compatibility spellings. Quaternion docs describe checked normalization and
conversion, public mutable `[w,x,y,z]` components, construction-time canonical-
sign behavior, and the actual-class-frame Identity restriction. Core docs retain
the VRT-14 negative/NaN/Inf/signed-zero sqrt semantics and the VRT-13 Result
two-state and assignment constraints. Matrix3 docs limit row-major, standard-
layout, trivial-copy and size observations to C++ source-level representation;
they do not promise ABI stability. Dynamics docs retain Angle as an independent
dimension and distinguish generic `Cross` from physical `RotationalCross`.
Current CMake docs state the 3.14 direct-build and 3.25 preset requirements,
C++20 target propagation, and `BUILD_TESTING` behavior.

The active coverage scope now lists the three VRT-12 Namespace.h files: 58 total
headers, 22 runtime-covered headers, 3 template-definition headers, and 36
compile-time-only headers. The tool's names and report text now identify
VectorisNumerics LLVM coverage; its raw numerator/denominator calculation and
thresholds are unchanged. Fresh coverage is 198/198 functions (100.00%),
1015/1027 lines (98.83%), and 395/435 raw branches (90.80%). No denominator
subtraction was used. LLVM coverage covers the emitted Numerics code only and
does not establish complete public-template/API coverage or Dynamics runtime
coverage.

One documentation limitation remains visible: the scope file still carries an
older supplemental branch-classification snapshot whose date, SHA and build
configuration were not recoverable. It is marked historical and is not applied
to the current export because its 32 classified branches do not match the current
40 raw uncovered branches. The raw branch ratio remains the normative threshold.
The historical R1 qualification report also retains its dated old PASS text, but
its prominent superseded warning and current status prevent that text from being
read as qualification of this Vectoris tree. The archived EulerIntegrator review
has no recorded source SHA and retains its old approval-pending wording; it is now
explicitly marked as non-current and points to the authoritative Dynamics spec.
The P0 closure report now records its resolvable full historical SHA and likewise
states that its old closure labels are not current project status.

### Principal regressions and verification

| Finding | Targeted result |
| --- | --- |
| VRT-01 Quaternion | 22/22 focused tests passed (`QuaternionTest`, `QuaternionScaleTest`, `GeometryComparisonTest`, checked conversion). |
| VRT-02 UnitVector3 | 19/19 focused tests passed, including float/double, cross-precision and encapsulation. |
| VRT-03 Result factories | 64/64 Result tests passed, including named factories and forwarding. |
| VRT-04 Matrix3 × Vector3 | 8/8 public multiplication tests passed. |
| VRT-05 sanitizer fail-fast | Gate passed; fail-fast diagnostics and clean combined-sanitizer control verified. |
| VRT-06 CI failure propagation | Pipeline gate passed. |
| VRT-07 Dynamics transactionality | 25/25 Dynamics, propagation and state tests passed. |
| VRT-08 LDLT | 17/17 symmetric solver tests passed. |
| VRT-09 AlmostEqual | 33/33 scalar and geometry wrapper tests passed. |
| VRT-10 API surface | 58/58 headers; 19/19 positive tests; 12/12 negative compile probes; gate passed. |
| VRT-10 self-tests | 17/17 passed. |
| VRT-11 exact-TU clang-tidy | 117 expected = 117 eligible = 117 analyzed; 0 failures and 0 diagnostics. |
| VRT-11 self-tests | 12/12 passed. |
| VRT-12 namespace contract | 60 Numerics and 13 Dynamics HeaderIsolation TUs passed; lowercase namespace probes passed for float/double. |
| VRT-13 Result invariant | 64/64 Result tests passed, including the exception-transition suite. |
| VRT-14 sqrt | 37/37 sqrt/math-wrapper tests passed. |
| VRT-15 Matrix3 and benchmark governance | 30/30 Matrix3 tests passed; Release benchmark built and ran with consumed checksum. Timing is local informational evidence only. |
| VRT-16 Angle / RotationalCross | 6/6 dimension and rotational-cross tests passed. |
| VRT-17 Identity frames | `GeometryIdentityFactoriesRespectClassFrames` passed; old Quaternion bypass reproduced before edits. |

Full test-enabled configurations all passed: Debug 299/299, Release 297/297,
ASan 299/299, UBSan 299/299, and ASan+UBSan 299/299. Sanitizer test runs used
`ASAN_OPTIONS=halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The test-enabled presets
retain `BUILD_TESTING=ON`. The fresh LLVM Numerics coverage run executed 259/259
tests. Builds emitted zero compiler warning lines. HeaderIsolation results are
58 standalone Numerics headers plus two order TUs, and 11 standalone Dynamics
headers plus two order TUs. The Public API gate and both VRT-10/VRT-11 self-test
suites passed. `git diff --check` passed after removing trailing spaces from the
changed metadata lines.

```text
VRT-01 THROUGH VRT-16:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED

VRT-17:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED

VRT-18:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED

VRT-19:
IMPLEMENTED LOCALLY
PENDING CI / INDEPENDENT REVIEW
NOT CLOSED

OVERALL:
NOT REQUALIFIED / Experimental
```

No commit or push was made. No final requalification was performed.
