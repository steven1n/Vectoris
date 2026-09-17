# AegisMathLib Engineering & Design Standard v1.0

> [!IMPORTANT]
> **Document**: Engineering & Design Standard  
> **Document Version**: 1.0 (V1)  
> **Status**: Normative Single Source of Truth (SSOT)  
> **Code Baseline**: `8ca516e28efc9e94762c8f35acf5d162280aa76d`  
> **Last Updated**: 2026-09-15  
> **Authority**: Single Source of Truth (Top Priority)

## 0. 文档目的

本文档定义 AegisMathLib 的：

- 软件架构规范
- C++ 编码规范
- API 设计规范
- 数值计算规范
- 单位与坐标系规范
- 内存与确定性规范
- 错误处理规范
- 测试与验证规范
- 性能规范
- 文档规范
- CI / 静态分析规范
- 版本与兼容性规范
- 规则偏离管理规范

本规范借鉴：

- NASA Software Engineering / Software Assurance 思想
- NASA/JPL Power of Ten
- Lockheed Martin JSF AV C++ Coding Standards
- MISRA C++ 高可靠软件设计原则
- SEI CERT C++ 安全编码原则
- 现代 C++ 工程实践

但针对 AegisMathLib 的 C++20 科学计算、工程仿真和控制应用进行了重新设计。

---

# 1. 项目定位

AegisMathLib 是一个：

> 面向工程仿真、导航、雷达、跟踪、控制、信号处理和数值计算的高可靠 C++ 数学基础库。

核心目标：

```text
Correct
Deterministic
Numerically Stable
Type Safe
Unit Safe
Testable
Portable
Maintainable
Efficient
Reproducible
```

优先级：

```text
Correctness
    >
Numerical Reliability
    >
Safety
    >
Clarity
    >
Maintainability
    >
Performance
    >
Convenience
```

禁止为了性能或代码长度牺牲正确性。

---

# 2. C++ 语言版本

## LANG-001

AegisMathLib Stable Core 统一使用：

```text
ISO C++20
```

不得要求 C++23 或 C++26。

---

## LANG-002

CMake：

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

或：

```cmake
target_compile_features(AegisMathLib PUBLIC cxx_std_20)
```

---

## LANG-003

禁止依赖：

```text
GNU extensions
MSVC proprietary language extensions
Clang-only language features
experimental C++ features
```

平台适配层除外。

---

# 3. 编译器基线

推荐最低：

```text
GCC   13+
Clang 17+
MSVC  19.38+
```

CI 应覆盖：

```text
Linux + GCC
Linux + Clang
Windows + MSVC
```

有条件增加：

```text
macOS + Apple Clang
```

---

# 4. 总体架构

推荐依赖层：

```text
Core
 ↓
Units
 ↓
Vector / Matrix
 ↓
Geometry / Frames
 ↓
Statistics / Random
 ↓
Numerical Methods
 ↓
Integration / Optimization
 ↓
Estimation
 ↓
Control
 ↓
Signal Processing
```

严格单向依赖。

---

# 5. 模块划分

```text
aegis::math
│
├── core
├── units
├── vector
├── matrix
├── geometry
├── frames
├── statistics
├── random
├── integration
├── interpolation
├── optimization
├── estimation
├── control
├── signal
└── detail
```

业务对象禁止进入数学库。

禁止出现：

```text
Radar
Missile
Aircraft
SPY1
SM2
ESSM
WeaponControl
FireControl
TargetTrack
```

这些属于上层 Simulator。

刚体动力学与领域物理状态（RigidBodyState、InertiaTensor3、Wrench6、Twist6 等）属于下游独立领域模块（如 `AegisDynamics`），严禁进入纯数学库 `AegisMathLib` 核心，具体边界判定与分类准则遵从权威规范：[`docs/PURE_MATH_SCOPE.md`](PURE_MATH_SCOPE.md)。

---

# 6. 文件布局

```text
AegisMathLib/
├── include/
│   └── aegis/
│       └── math/
├── src/
├── tests/
├── benchmarks/
├── examples/
├── docs/
├── cmake/
└── tools/
```

---

# 7. 文件职责

原则：

> One major concept per file.

例如：

```text
vector.hpp
matrix.hpp
quantity.hpp
quaternion.hpp
rk4.hpp
ekf.hpp
imm.hpp
pid.hpp
fft.hpp
```

禁止创建：

```text
utils.hpp
helpers.hpp
misc.hpp
common.hpp
everything.hpp
```

这种文件通常意味着架构已经开始腐烂。

---

# 8. Namespace

公开代码：

```cpp
namespace aegis::math {
}
```

例如：

```cpp
namespace aegis::math::units {}
namespace aegis::math::geometry {}
namespace aegis::math::estimation {}
```

禁止在公共头文件：

```cpp
using namespace std;
using namespace aegis::math;
```

---

# 9. 命名规范

## 类型

```text
PascalCase
```

例如：

```cpp
Vector3
Matrix3
Quaternion
ExtendedKalmanFilter
```

---

## 函数

```text
snake_case
```

```cpp
normalize()
dot_product()
cross_product()
solve_linear_system()
```

---

## 变量

```text
snake_case
```

```cpp
time_step
measurement_noise
angular_velocity
```

---

## 成员变量

统一：

```cpp
state_
covariance_
size_
```

---

## 常量

```cpp
inline constexpr double standard_gravity = 9.80665;
```

禁止：

```cpp
#define PI 3.14159
```

---

# 10. 类型安全原则

禁止让一个 `double` 同时可能表示：

```text
米
秒
弧度
速度
加速度
频率
```

具有物理意义的数据必须优先进入 Units System。

---

# 11. Units System

核心原则：

> Physical quantities shall not be represented by untyped raw floating-point values at public API boundaries.

例如：

```cpp
Length distance;
Time duration;
Velocity velocity;
Acceleration acceleration;
Angle heading;
```

---

# 12. SI 内部标准

核心计算统一使用 SI：

```text
metre
kilogram
second
ampere
kelvin
mole
candela
radian
```

外部输入：

```text
km
ft
nm
knot
Mach
degree
g
```

必须转换后进入核心。

---

# 13. 角度规范

内部：

```text
radian
```

API 可以接受：

```cpp
Degrees
Radians
```

禁止：

```cpp
double heading = 90;
```

优先：

```cpp
Degrees heading{90.0};
```

---

# 14. 浮点类型

默认：

```cpp
double
```

只有经过：

```text
误差分析
benchmark
内存分析
```

才能将关键算法改为 `float`。

---

# 15. IEEE-754

核心浮点实现应遵循 IEEE-754 语义。

不得假定：

```text
不存在 NaN
不存在 Inf
overflow 不会发生
underflow 不重要
```

---

# 16. 浮点比较

禁止：

```cpp
if (a == b)
```

用于一般计算结果。

统一：

```cpp
almost_equal(a, b, rel_tol, abs_tol)
```

条件：

```text
|a-b| <= max(
    abs_tol,
    rel_tol * max(|a|, |b|)
)
```

---

# 17. 数值稳定性

实现时必须考虑：

```text
overflow
underflow
cancellation
conditioning
round-off
singularity
NaN
Inf
loss of significance
```

例如：

优先：

```cpp
std::hypot(x, y)
```

而非：

```cpp
std::sqrt(x*x + y*y)
```

---

# 18. 矩阵逆

原则：

> Solve, do not invert.

禁止默认：

```cpp
inverse(A) * b
```

优先：

```cpp
solve(A, b)
```

除非数学问题明确要求逆矩阵。

---

# 19. Vector 设计

推荐：

```cpp
template<typename T, std::size_t N>
class Vector;
```

例如：

```cpp
Vector<double, 3>
Vector<double, 9>
```

常用别名：

```cpp
using Vector2d = Vector<double, 2>;
using Vector3d = Vector<double, 3>;
```

---

# 20. Matrix 设计

```cpp
template<
    typename T,
    std::size_t Rows,
    std::size_t Cols
>
class Matrix;
```

优先静态尺寸。

例如：

```cpp
Matrix<double, 3, 3>
Matrix<double, 9, 9>
```

动态矩阵仅用于尺寸确实运行时确定的场景。

---

# 21. 矩阵布局

必须明确规定。

AegisMathLib 默认：

```text
Row-major
```

访问：

```cpp
matrix(row, col)
```

内部存储不得成为 Public API。

---

# 22. 坐标系

所有空间数据必须知道所属 Frame。

至少计划支持：

```text
ECI
ECEF
ENU
NED
Body
World
Sensor
```

禁止公共接口只写：

```cpp
Vector3 velocity;
```

而不说明在哪个坐标系。

长期建议：

```cpp
Vector3<double, Frame::ECEF>
```

实现编译期 Frame Safety。

---

# 23. Quaternion

全项目只允许一种 convention。

规定：

```text
[w, x, y, z]
```

必须明确：

```text
active rotation
multiplication order
source frame
destination frame
```

不得不同模块使用不同约定。

---

# 24. Rotation Matrix

旋转矩阵应验证：

```text
R Rᵀ ≈ I
det(R) ≈ +1
```

任何姿态转换必须明确：

```text
From Frame
To Frame
```

---

# 25. 时间模型

数学库使用：

```cpp
Seconds dt;
```

禁止仿真算法内部依赖：

```cpp
std::chrono::system_clock::now()
```

区分：

```text
simulation time
wall-clock time
```

AegisMathLib 只关心前者。

---

# 26. Determinism

给定相同：

```text
initial state
input
dt
configuration
random seed
algorithm
```

结果应可重复。

---

# 27. Random

禁止 hidden RNG。

禁止核心算法内部偷偷：

```cpp
std::random_device
```

应显式：

```cpp
RandomEngine rng{seed};
```

分布：

```cpp
NormalDistribution normal;
normal.sample(rng);
```

RNG 和 distribution 分离。

---

# 28. Monte Carlo

每次实验必须可以记录：

```text
seed
configuration
build version
algorithm version
initial conditions
sample count
```

从而保证实验可复现。

---

# 29. Heap Allocation

对于 Stable Numerical Kernel：

> 初始化完成后原则上不得产生不可控动态分配。

尤其：

```text
EKF predict
EKF update
IMM cycle
controller update
guidance calculation
signal processing loop
simulation integration
```

固定尺寸优先：

```text
stack
std::array
static-sized matrix
```

---

# 30. Dynamic Allocation

禁止：

```cpp
new
delete
malloc
free
```

直接出现在核心算法代码。

如必须动态分配：

```text
初始化阶段完成
预分配 workspace
明确 ownership
提供 documented deviation
```

---

# 31. Recursion

Numerical Core 原则禁止递归。

允许例外必须证明：

```text
最大深度有界
栈占用可证明
不会 stack overflow
```

---

# 32. 循环

关键算法循环应尽可能有上界。

例如迭代求解：

```cpp
for (std::size_t i = 0; i < max_iterations; ++i)
```

禁止：

```cpp
while (!converged)
```

没有最大迭代限制。

---

# 33. 复杂度

Stable Core：

```text
Cyclomatic Complexity <= 15
```

超过：

```text
必须 review
必须记录理由
优先拆函数
```

---

# 34. 函数长度

目标：

```text
<= 80 logical LOC
```

警告：

```text
> 100 LOC
```

硬上限建议：

```text
150 LOC
```

超过需要 deviation。

---

# 35. Pure Functions

能写成纯函数的数学操作应优先纯函数。

例如：

```cpp
dot()
cross()
rotate()
propagate()
interpolate()
transform()
```

---

# 36. State

算法不得隐藏重要状态。

好的 API：

```cpp
auto next_state = propagate(
    current_state,
    control,
    dt
);
```

避免：

```cpp
simulator.do_magic();
```

---

# 37. 参数传递

小对象：

```cpp
double
int
enum class
```

按值。

大对象：

```cpp
const Matrix& matrix
```

按 const reference。

需要 ownership：

```cpp
Matrix matrix
```

按值 + move。

---

# 38. Const

默认：

```cpp
const
```

能不修改就不修改。

成员函数：

```cpp
double norm() const;
```

---

# 39. auto

允许：

```cpp
const auto result = solve(...);
```

但若类型本身携带重要语义，应考虑显式写出。

---

# 40. Concepts

模板公共接口优先 Concepts。

例如：

```cpp
template<typename T>
concept Scalar =
    std::floating_point<T>;
```

避免模板错误信息堆成文学巨著。

---

# 41. constexpr

基础数学类型尽量支持：

```cpp
constexpr
```

例如：

```cpp
constexpr auto square(double x) noexcept -> double;
```

---

# 42. [[nodiscard]]

重要函数必须：

```cpp
[[nodiscard]]
```

包括：

```text
solve
inverse
normalize
integrate
filter result
optimization result
```

---

# 43. noexcept

只有真正保证不抛异常的函数才加。

禁止装饰性 `noexcept`。

---

# 44. Exception Policy

Numerical Core 不使用 exception 表示正常数学失败。

例如：

```text
singular matrix
non-convergence
zero vector normalization
invalid parameter
```

应返回：

```text
Result<T, Error>
```

---

# 45. Result

建议内部实现：

```cpp
template<typename T, typename E>
class Result;
```

未来迁移 C++23 时可以映射：

```cpp
std::expected<T, E>
```

---

# 46. Error

统一错误类型：

```cpp
enum class MathError {
    InvalidArgument,
    DivisionByZero,
    SingularMatrix,
    NonFiniteInput,
    NonConvergence,
    NumericalFailure,
    OutOfRange
};
```

禁止：

```cpp
return -1;
return nullptr;
```

表达数学错误。

---

# 47. assert

`assert()` 只用于：

```text
程序员错误
内部 invariant
理论上不可能状态
```

不能替代正常输入验证。

---

# 48. NaN / Inf

关键边界建议使用：

```cpp
std::isfinite(value)
```

Debug 配置重点检查：

```text
state
covariance
innovation
integration result
control output
```

---

# 49. Global State

原则上禁止：

```text
mutable global variables
global RNG
global numerical configuration
global filter state
```

只允许真正的：

```cpp
inline constexpr
```

常量。

---

# 50. 指针

所有权禁止 raw pointer。

首选：

```text
value
reference
span
unique_ptr
```

`shared_ptr` 必须有明确共享所有权理由。

核心数学层尽量避免 heap ownership。

---

# 51. std::span

连续数据 view 优先：

```cpp
std::span<const double>
```

不要轻易使用：

```cpp
const double*
std::size_t length
```

---

# 52. Pointer Arithmetic

核心代码原则上禁止人工 pointer arithmetic。

除非：

```text
经过性能验证
隔离在 detail 层
具有测试
具有 deviation
```

---

# 53. Casting

禁止 C-style cast：

```cpp
(int)x
```

使用：

```cpp
static_cast<int>(x)
```

并避免：

```text
narrowing
signed/unsigned confusion
float/integer implicit conversion
```

---

# 54. Preprocessor

宏使用限制：

允许：

```text
include guard
platform detection
compiler feature abstraction
```

禁止宏实现数学算法。

---

# 55. Header 设计

公共 header：

```text
minimal dependency
self-contained
compile independently
```

禁止依赖 include 顺序才能编译。

---

# 56. detail

内部 implementation：

```text
aegis::math::detail
```

不属于稳定 API。

用户不得依赖。

---

# 57. Public API 原则

API 必须：

```text
explicit
strongly typed
predictable
documented
minimal
```

用户不应该猜：

```text
单位
Frame
角度制
矩阵尺寸
ownership
error behavior
```

---

# 58. API 参数

禁止这种 API：

```cpp
update(data, 0.1, 1, 0, true, false);
```

优先：

```cpp
UpdateOptions options{
    .time_step = Seconds{0.1},
    .normalize = true
};
```

---

# 59. Hidden Behavior

禁止：

```text
hidden unit conversion
hidden coordinate transform
hidden RNG
hidden allocation
hidden global state
hidden time source
```

---

# 60. Numerical Integration

统一模型：

```cpp
integrate(
    derivative,
    state,
    time,
    dt
);
```

至少支持：

```text
Euler
Semi-Implicit Euler
RK2
RK4
Adaptive RK
```

---

# 61. RK4

RK4 实现必须通过：

```text
解析微分方程测试
阶数收敛测试
constant velocity
constant acceleration
harmonic oscillator
```

验证。

---

# 62. Root Finding

算法必须有：

```text
tolerance
max_iterations
status
residual
```

例如：

```text
Bisection
Newton
Secant
Brent
```

不得无限迭代。

---

# 63. Optimization

结果对象至少返回：

```text
solution
cost
iterations
converged
termination_reason
```

不能只返回最终数值。

---

# 64. Statistics

统计模块必须区分：

```text
population
sample
variance convention
biased
unbiased
```

不得 API 含义不清。

---

# 65. EKF

必须结构化为：

```text
State
Covariance
Process Model
Measurement Model
Prediction
Innovation
Update
Diagnostics
```

---

# 66. EKF 符号

允许算法内部采用：

```text
x
P
F
Q
H
R
z
y
S
K
```

但公共 API 应使用更明确命名。

---

# 67. Covariance Update

优先 Joseph form：

```text
P =
(I-KH)P(I-KH)^T
+
KRK^T
```

避免只使用：

```text
P = (I-KH)P
```

---

# 68. Covariance Properties

协方差矩阵应验证：

```text
symmetric
positive semi-definite
finite
```

必要时：

```text
P = 0.5(P + Pᵀ)
```

处理数值对称性误差。

---

# 69. EKF Verification

至少包含：

```text
RMSE
bias
innovation
NIS
NEES
Monte Carlo consistency
```

不能只盯轨迹图说“挺准”。

---

# 70. IMM

必须拆为：

```text
Mixing
Model Prediction
Model Update
Likelihood
Model Probability Update
State Combination
```

每一部分单独可测试。

---

# 71. Control

控制模块区分：

```text
Plant
Controller
Reference
State
Actuator
Constraint
```

控制算法不应该偷偷依赖模拟器对象。

---

# 72. PID

必须考虑：

```text
dt validation
integral windup
output saturation
derivative filtering
reset behavior
```

---

# 73. Signal Processing

接口必须明确：

```text
sampling rate
sample count
window
normalization
frequency convention
```

FFT 不允许隐式猜 sampling frequency。

---

# 74. FFT

测试至少包括：

```text
single tone
multiple tone
DC
Nyquist
inverse transform
Parseval consistency
```

---

# 75. 性能原则

顺序：

```text
Correct
↓
Test
↓
Benchmark
↓
Profile
↓
Optimize
↓
Regression Test
```

禁止凭感觉优化。

---

# 76. SIMD

只能作为优化层。

必须保留正确的 scalar reference implementation。

例如：

```text
Scalar
AVX2
AVX-512
NEON
```

结果必须在 tolerance 内一致。

---

# 77. Memory Layout

任何依赖：

```text
alignment
SIMD
cache layout
row-major storage
```

的优化必须进入文档。

禁止依赖未定义对象布局。

---

# 78. Platform Independence

不得假定：

```text
endianness
sizeof(long)
pointer width
alignment
object binary layout
```

除非 platform layer 明确处理。

---

# 79. Thread Safety

无状态函数默认 thread-safe。

禁止核心模块使用：

```text
mutable static
shared global RNG
hidden cache with races
```

---

# 80. 测试等级

测试至少分：

```text
Unit Test
Property Test
Golden Test
Regression Test
Monte Carlo Test
Integration Test
Benchmark
```

---

# 81. Unit Test

每个核心算法至少覆盖：

```text
normal
boundary
zero
negative if applicable
very small
very large
invalid
NaN
Inf
```

---

# 82. Golden Test

与可信 reference 比较。

例如：

```text
known analytical solution
MATLAB
Python/NumPy
SciPy
published reference
independent implementation
```

---

# 83. Property-Based Test

例如：

```text
dot(a,b) = dot(b,a)

cross(a,b) = -cross(b,a)

R Rᵀ ≈ I

q q^-1 ≈ identity

A solve(A,b) ≈ b
```

---

# 84. Regression Test

发现过的 bug 必须增加 regression test。

原则：

> A fixed bug shall remain fixed.

---

# 85. Monte Carlo Verification

概率和估计算法必须允许统计验证。

报告：

```text
seed
runs
RMSE
bias
variance
coverage
NIS
NEES
confidence interval
```

---

# 86. Test Coverage

Stable Core 目标：

```text
100% function coverage
>=95% line coverage
>=90% branch coverage
```

对特别关键的状态决策逻辑：

```text
目标 100% branch
```

必要时进行：

```text
MC/DC
```

---

# 87. Static Analysis

必须：

```text
clang-tidy
compiler warnings
```

推荐：

```text
cppcheck
CodeQL
```

---

# 88. Sanitizers

CI 推荐运行：

```text
ASan
UBSan
```

并定期：

```text
TSan
```

---

# 89. Compiler Warnings

目标：

```text
0 warnings
```

推荐：

```text
-Wall
-Wextra
-Wpedantic
-Wconversion
-Wshadow
-Wnull-dereference
-Wdouble-promotion
-Wformat=2
-Wnon-virtual-dtor
-Woverloaded-virtual
```

---

# 90. Warning Policy

禁止通过：

```text
关闭 warning
全局 suppression
```

来“修复”问题。

真正不可避免的 suppression 必须：

```text
最小作用域
记录原因
```

---

# 91. clang-format

全项目统一。

建议：

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
```

格式由工具决定。

程序员无需为大括号宗教战争浪费生命。

---

# 92. 文档要求

所有 public API 必须说明：

```text
purpose
parameters
return value
units
frame
preconditions
postconditions
failure behavior
numerical assumptions
```

---

# 93. 数学文档

复杂算法必须说明公式。

例如 EKF：

```text
Innovation:
y = z - h(x)

Innovation covariance:
S = HPHᵀ + R

Gain:
K = PHᵀ S⁻¹
```

代码结构尽量和公式一致。

---

# 94. References

关键算法必须记录来源。

统一：

```text
docs/references.md
```

例如：

```text
Kalman filtering textbooks
Bar-Shalom
Maybeck
Brown & Hwang
Stevens & Lewis
IEEE papers
NASA technical reports
```

---

# 95. 注释

注释解释：

```text
WHY
assumption
numerical reason
algorithm reference
physical meaning
```

不要写：

```cpp
// increment i
++i;
```

---

# 96. Logging

AegisMathLib Core 禁止：

```cpp
std::cout
std::cerr
printf
```

数学库返回：

```text
result
status
diagnostics
```

上层决定怎么输出。

---

# 97. Diagnostics

复杂算法可以返回诊断数据：

```text
iterations
residual
condition estimate
innovation
likelihood
convergence
```

而不是偷偷吞掉。

---

# 98. API Stability

采用 Semantic Versioning：

```text
MAJOR.MINOR.PATCH
```

例如：

```text
1.0.0
1.1.0
1.1.1
2.0.0
```

---

# 99. API 生命周期

定义：

```text
Experimental
Beta
Stable
Deprecated
```

Experimental 不保证兼容性。

Stable API 不能随便修改。

---

# 100. Dependency Policy

原则：

```text
Minimal Dependencies
```

核心优先仅依赖：

```text
C++ Standard Library
```

第三方依赖必须说明：

```text
为什么需要
许可证
性能影响
维护风险
替代方案
```

---

# 101. 规则偏离 Deviation

允许违反规范，但必须显式记录。

格式：

```text
Deviation ID
Affected rule
Location
Reason
Risk
Mitigation
Verification
Reviewer
```

例如：

```cpp
// AML-DEVIATION: MEM-003
//
// Rule:
// No runtime allocation in deterministic kernel.
//
// Reason:
// FFT workspace uses one initialization-time allocation.
//
// Risk:
// None during processing loop.
//
// Verification:
// allocation_test.cpp
```

---

# 102. 禁止 Silent Deviation

不允许：

```text
“这里特殊”
“这样快一点”
“应该没事”
```

这种口头工程学。

要么符合规则，要么登记 deviation。

---

# 103. Commit 规范

推荐 Conventional Commits：

```text
feat(matrix): add LU decomposition

fix(ekf): correct covariance update

test(units): add dimensional tests

perf(vector): optimize dot product

docs(filter): document EKF equations
```

---

# 104. Pull Request

数学模块合并前检查：

```text
[ ] API reviewed
[ ] units defined
[ ] frame defined
[ ] numerical assumptions documented
[ ] errors defined
[ ] unit tests
[ ] boundary tests
[ ] golden/reference test
[ ] sanitizer pass
[ ] clang-tidy pass
[ ] warnings = 0
[ ] documentation complete
[ ] benchmark if performance-sensitive
```

---

# 105. CI

推荐：

```text
Format
 ↓
Configure
 ↓
Compile GCC
 ↓
Compile Clang
 ↓
Compile MSVC
 ↓
Unit Tests
 ↓
Static Analysis
 ↓
ASan
 ↓
UBSan
 ↓
Coverage
 ↓
Benchmark Regression
```

---

# 106. Performance Regression

关键算法记录 baseline：

```text
Matrix multiply
Matrix solve
Vector operations
Quaternion rotation
RK4
EKF predict
EKF update
IMM cycle
FFT
```

重大性能退化必须说明。

---

# 107. Numerical Regression

性能之外，还要防止精度退化。

CI 可以保存：

```text
RMSE
residual
relative error
orthogonality error
covariance consistency
```

作为 regression 指标。

---

# 108. Security / Undefined Behavior

禁止依赖：

```text
undefined behavior
signed integer overflow
invalid pointer arithmetic
use-after-free
uninitialized data
out-of-bounds access
```

UBSan 必须保持 clean。

---

# 109. Integer Safety

特别关注：

```text
signed vs unsigned
size_t conversion
overflow
narrowing
index calculation
```

优先使用：

```cpp
std::size_t
```

表示容器尺寸。

---

# 110. Initialization

对象必须在构造时形成有效状态。

禁止：

```cpp
Filter filter;
filter.init1();
filter.init2();
filter.finish_init();
```

优先：

```cpp
Filter filter{config};
```

---

# 111. Invalid State

设计原则：

> Make invalid states difficult to represent.

例如物理量：

```cpp
Seconds dt;
```

优于裸：

```cpp
double dt;
```

---

# 112. Config Objects

复杂算法采用明确配置对象：

```cpp
struct EkfConfig {
    ...
};
```

不要拥有十几个构造参数。

---

# 113. Serialization Boundary

Units 等强类型只有在：

```text
file serialization
network
UI
external APIs
```

边界允许转成 primitive。

转换必须集中。

---

# 114. Reproducible Research

任何用于 Technical Report 的结果应保存：

```text
Git commit hash
compiler
compiler version
build type
OS
configuration
seed
simulation parameters
result dataset
```

---

# 115. Benchmark Reproducibility

benchmark 必须记录：

```text
CPU
compiler
flags
build type
sample count
algorithm version
```

---

# 116. Compiler Optimization

正式 benchmark 使用：

```text
Release
```

但数值测试同时运行：

```text
Debug
Release
```

避免优化级别暴露隐藏 UB。

---

# 117. Fast Math

禁止 Stable Core 默认启用：

```text
-ffast-math
/ofast
```

因为可能破坏：

```text
NaN semantics
Inf semantics
associativity assumptions
IEEE behavior
```

任何使用必须单独研究和 deviation。

---

# 118. FMA / SIMD

允许使用硬件 FMA，但应考虑：

```text
结果差异
reproducibility
platform differences
```

Exact bitwise reproducibility 和 numerical reproducibility 必须区分。

---

# 119. Deterministic Mode

建议未来提供：

```text
AML_DETERMINISTIC_MODE
```

确保：

```text
fixed algorithms
fixed seeds
stable execution path
no parallel nondeterministic reduction
```

方便论文和 Monte Carlo 复现。

---

# 120. Parallel Computing

并行优化只能在：

```text
serial reference implementation
```

已经验证后加入。

并行 reduction 必须明确数值误差可能不同。

---

# 121. Design Review

新模块开发顺序：

```text
Requirement
↓
Mathematical Model
↓
API Design
↓
Failure Modes
↓
Reference Solution
↓
Implementation
↓
Unit Verification
↓
Monte Carlo / Validation
↓
Performance
↓
Stable
```

不要反过来先写 2000 行再思考 API。

---

# 122. Module Specification

每个主要模块应有：

```text
docs/<module>.md
```

至少包含：

```text
Purpose
Mathematical Definition
API
Units
Frames
Assumptions
Error Conditions
Numerical Stability
Complexity
Verification
References
```

---

# 123. Experimental 到 Stable

一个模块只能在满足：

```text
数学正确
API review
reference validation
coverage 达标
static analysis clean
sanitizer clean
docs 完整
```

后进入 Stable。

---

# 124. Definition of Done

一个核心数学模块的 DoD：

1. Requirement 明确
2. 数学模型明确
3. API 明确
4. 单位明确
5. Frame 明确
6. Error behavior 明确
7. Numerical limitations 明确
8. Unit Test 完成
9. Boundary Test 完成
10. Golden Test 完成
11. Property Test 完成
12. 必要时 Monte Carlo 完成
13. Benchmark 完成
14. clang-tidy 通过
15. ASan 通过
16. UBSan 通过
17. compiler warnings = 0
18. 文档完成
19. CI 全通过
20. Reviewer approval

否则：

```text
Experimental
```

---

# 125. 核心不可违反原则

AegisMathLib 的十条核心原则：

```text
1. Never hide units.

2. Never hide coordinate frames.

3. Never hide randomness.

4. Never hide important state.

5. Never hide allocation.

6. Never hide numerical failure.

7. Never use unbounded iteration in critical kernels.

8. Never optimize without measurement.

9. Never trust floating-point equality.

10. Never merge mathematical code without verification.
```

---

# 126. 最终设计哲学

AegisMathLib 不追求：

```text
代码最短
模板最炫
C++版本最新
benchmark 数字最好看
```

追求：

```text
数学含义明确
结果可复现
错误可检测
行为可预测
边界可解释
算法可验证
性能可测量
API 可长期维护
```

最终标准不是：

> It compiles.

也不是：

> It runs.

而是：

> It produces a result whose correctness, assumptions, numerical behavior and limitations can be explained and verified.

即：

> **能证明它为什么算得对。**
