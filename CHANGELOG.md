# Changelog

## [1.0.0] - 2026-10-03 - VectorisNumerics

- Formal owner-accepted numerical/runtime 1.0 release based on frozen C15.
- Preserve numerical hardening, exact-SHA GCC/Clang/MSVC qualification, sanitizer,
  static-analysis, coverage and fresh release-smoke evidence.
- VRT-01 through VRT-19 formally CLOSED on preserved substantive repair evidence.
- AFA5-001 and AFA5-002 remain OPEN-DEFERRED / OWNER ACCEPTED RELEASE DEBT.
  C15 independent re-audit FAIL is preserved; release uses an explicit owner
  acceptance of two qualification-tooling MAJOR findings.
- Attempt #1 ABORTED due to external probe error; Attempt #2 PASS.
- No production algorithm/API changes. CMake project and VERSION now identify 1.0.0.
- No install/config-package support added; Dynamics maturity remains separate.
- [Release notes and scope](docs/releases/VectorisNumerics_1.0.0.md).

## [1.0.0-RC1] - 2026-09-17 - Vectoris Global Rename Migration

### Changed
- **Global Rebranding**: Entire software suite rebranded from `AegisMathLib` to **`Vectoris`**.
- **Module Architecture**:
  - `modules/AegisMathLib` renamed to `modules/VectorisNumerics` (`VectorisNumerics`, alias `Vectoris::Numerics`).
  - `modules/AegisDynamics` renamed to `modules/VectorisDynamics` (`VectorisDynamics`, alias `Vectoris::Dynamics`).
- **Namespace Realignment**:
  - Pure numerics migrated to standard lowercase hierarchy: `namespace vectoris::numerics` (`vectoris::numerics::core`, `units`, `geometry`).
  - Domain physics migrated to `namespace vectoris::dynamics`.
  - Fully resolves and closes formal deviation `AML-DEVIATION-003`.
- **Public Headers Relocated**:
  - Numerics headers moved to `<Vectoris/Numerics/...>`.
  - Dynamics headers moved to `<Vectoris/Dynamics/...>`.
- **Build Infrastructure**:
  - Root project `project(Vectoris LANGUAGES CXX)`.
  - Options converted to `VECTORIS_STRICT_WARNINGS`, `VECTORIS_BUILD_DYNAMICS`, `VECTORIS_ENABLE_*`.
  - CMake presets updated with `pure-numerics` (preserving `pure-math` legacy alias).

---

# Historical Development Notes (Pre-Vectoris Migration)

# AegisMathLib Geometry Foundation v1.0

> [!NOTE]
> **Status**: HISTORICAL / ARCHIVE (Non-Authoritative)  
> This file contains pre-governance historical development notes. For authoritative release and qualification status, consult [`README.md`](README.md) and [`docs/VECTORIS_RENAME_MIGRATION.md`](docs/VECTORIS_RENAME_MIGRATION.md).

### Added

#### Quaternion Attitude Engine

完成 `Quaternion` 姿态核心引擎，实现航空航天级四元数姿态表示：

- 固定 Hamilton Quaternion Convention
- 固定元素布局：

```
[w, x, y, z]
```

- 固定 Active Rotation:

```
v_target = q * v_source * q*
```

- 支持 FrameTag 编译期坐标安全约束
- 支持 q/-q Canonicalization
- 支持：
    - Quaternion composition
    - Vector rotation
    - Slerp interpolation
    - Quaternion normalization


#### Transform3 SE(3) Rigid Body Transform Engine

新增六自由度刚体空间变换：

\[
SE(3)=SO(3)\ltimes R^3
\]


支持：

- Point3 坐标变换
- Vector3 方向变换
- Transform composition
- Transform inverse


Transform Convention:

```
Transform<A,B>
```

表示：

> 将 A Frame 下表达的数据映射到 B Frame。


组合规则：

```
T_AC = T_AB * T_BC
```

对应数学：

\[
T_{AC}=T_{BC}\circ T_{AB}
\]


---

# Architecture Corrections

## Quaternion Composition

修正 Hamilton Product 方向问题。

此前：

- API 顺序
- 数学乘法顺序

存在语义不一致。

现在统一：

C++ API:

```cpp
q_AB * q_BC
```

内部数学：

\[
Q_{AC}=Q_{BC}\otimes Q_{AB}
\]


确保：

- 坐标系流向符合 Source → Target
- FrameTag 与物理意义一致


---

## Frame Safety Improvements

增强 C++20 Concept 约束：

### 禁止：

```cpp
Quaternion<A,B> * Quaternion<C,D>
```

### 允许：

```cpp
Quaternion<A,B> * Quaternion<B,C>
```


禁止：

```cpp
Identity<A,B>()
```

其中：

```
A != B
```

因为不同坐标系不存在无条件恒等旋转。


---

# ABI Contract Improvements

## Transform3 Layout Validation

修正原：

```cpp
sizeof(T)*7
```

的不安全假设。


改为：

```cpp
offsetof()
```

验证：

```
Quaternion
+
Vector3
```

连续布局。


支持未来：

- SIMD alignment
- DMA buffer
- Binary serialization


---

# Build/Test Fixes

修复：

- Concept constrained friend declaration mismatch
- Quaternion cross product dependency
- offsetof template comma macro expansion
- MathFunctions include path
- UnitsTest namespace/header dependency


---

# Current Geometry Architecture

```
AegisMath
|
├ Core
│
├ Traits
│
└ Geometry
   |
   ├ FrameTag
   |
   ├ Vector3
   ├ Point3
   ├ UnitVector3
   |
   ├ Matrix3
   |
   ├ RotationMatrix3
   ├ Quaternion
   |
   └ Transform3
```

Geometry Layer now provides:

\[
R^3
\rightarrow
SO(3)
\rightarrow
SE(3)
\]


Foundation status:

```
Phase 2 Complete
Geometry Foundation v1.0 Frozen
```

---

Next milestone:

Phase 3 — GNC Dynamics Layer

Planned modules:

```
Dynamics
|
├ Twist6
├ Wrench6
├ SpatialVector
├ RigidBody6DOF
├ Propagation
└ Estimation
```