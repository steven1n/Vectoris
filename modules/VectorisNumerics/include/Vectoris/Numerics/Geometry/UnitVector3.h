#pragma once
#include "Namespace.h"
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Detail/ABI.h"
#include "Traits.h"

// 修正：指向真实的 Core 目录，如果你的实际文件在其他位置，请对应修改此路径
#include "../Core/NumericTraits.h"
#include "../Core/MathFunctions.h"
#include "../Core/Result.h"

namespace vectoris::numerics::Geometry {

    // [GEO-UNIT-001] 内部安全构造标签，用于保全 ABI 并封锁非法实例化
    struct UnitValidatedTag final {};

    // [Phase 2.2] UnitVector3 Core Engine
    template <std::floating_point T, FrameTag Frame = FrameUnknown>
    struct UnitVector3 final {
    private:
        T x_;
        T y_;
        T z_;

        // 私有构造，封锁外部直接实例化，确保单位模长不变量
        constexpr UnitVector3(T _x, T _y, T _z, UnitValidatedTag) noexcept
            : x_(_x), y_(_y), z_(_z) {}

    public:
        // 彻底禁用默认构造，防止出现 (0, 0, 0)
        UnitVector3() = delete;

        // 核心只读访问器 (防止外部直接写破坏单位范数不变量)
        [[nodiscard]] constexpr T x() const noexcept { return x_; }
        [[nodiscard]] constexpr T y() const noexcept { return y_; }
        [[nodiscard]] constexpr T z() const noexcept { return z_; }

        // 访问器 (保持兼容)
        [[nodiscard]] constexpr T getX() const noexcept { return x_; }
        [[nodiscard]] constexpr T getY() const noexcept { return y_; }
        [[nodiscard]] constexpr T getZ() const noexcept { return z_; }

        // --- 核心工厂 ---
        // [GEO-UNIT-003] 泛化输入类型，允许 float32 的 Vector3 生成 double 的 UnitVector3
        template <std::floating_point U>
        static Core::Result<UnitVector3> TryCreate(const Vector3<U, Frame>& input) noexcept {
            if (!Traits::IsFinite(input.x) || !Traits::IsFinite(input.y) || !Traits::IsFinite(input.z)) {
                return Core::Result<UnitVector3>::failure(Core::MathError::non_finite_input);
            }
            // Normalize before narrowing; promote float input for double storage.
            using CalcType = std::common_type_t<T, U>;
            const CalcType x = static_cast<CalcType>(input.x);
            const CalcType y = static_cast<CalcType>(input.y);
            const CalcType z = static_cast<CalcType>(input.z);
            const CalcType scale = std::max({Core::Math::abs(x), Core::Math::abs(y), Core::Math::abs(z)});
            // Exact input-zero classification, never an epsilon cutoff on magnitude.
            if (scale == CalcType{0}) {
                return Core::Result<UnitVector3>::failure(Core::MathError::zero_norm);
            }
            // Finite input guarantees finite scale; squared scaled norm is in [1, 3].
            const CalcType ux = x / scale;
            const CalcType uy = y / scale;
            const CalcType uz = z / scale;
            const CalcType norm = Core::Math::sqrt(ux*ux + uy*uy + uz*uz);
            const UnitVector3 candidate(static_cast<T>(ux / norm),
                                        static_cast<T>(uy / norm),
                                        static_cast<T>(uz / norm), UnitValidatedTag{});
            if (!candidate.IsValid()) {
                return Core::Result<UnitVector3>::failure(Core::MathError::normalization_failure);
            }
            return Core::Result<UnitVector3>::success(candidate);
        }

        template <std::floating_point U>
        static bool TryCreate(const Vector3<U, Frame>& input, UnitVector3& out) noexcept {
            auto res = TryCreate(input);
            if (!res.IsSuccess()) {
                return false;
            }
            out = res.Value();
            return true;
        }

        // [GEO-UNIT-005] 显式长度恢复接口 (用于遥测/调试/序列化)
        [[nodiscard]] constexpr Vector3<T, Frame> ToVector() const noexcept {
            return Vector3<T, Frame>{x_, y_, z_};
        }

        // [GEO-UNIT-006] Invariant 运行时/断言检查
        [[nodiscard]] constexpr bool IsValid() const noexcept {
            // hypot remains safe even when inspecting non-unit magnitudes.
            // A finite norm also excludes every NaN/Inf component combination.
            const T norm = std::hypot(x_, y_, z_);
            return Traits::IsFinite(norm) && Traits::AlmostEqual(norm, T{1},
                                       Traits::NumericTraits<T>::epsilon() * T{10},
                                       Traits::NumericTraits<T>::epsilon() * T{10});
        }

        // --- 代数运算 ---

        // 方向乘长度：UnitVector3 * Scalar = Vector3
        template <ScalarArithmetic S>
        constexpr auto operator*(const S& scalar) const noexcept {
            // Pass component values to user-defined scalar operators, not storage references.
            using ResT = decltype(x() * scalar);
            return Vector3<ResT, Frame>{x() * scalar, y() * scalar, z() * scalar};
        }

        constexpr UnitVector3 operator-() const noexcept {
            return UnitVector3(-x_, -y_, -z_, UnitValidatedTag{});
        }

        template <ScalarArithmetic U>
        constexpr auto dot(const UnitVector3<U, Frame>& rhs) const noexcept {
            return (x_ * rhs.x()) + (y_ * rhs.y()) + (z_ * rhs.z());
        }

        template <ScalarArithmetic U>
        constexpr auto dot(const Vector3<U, Frame>& rhs) const noexcept {
            return (x() * rhs.x) + (y() * rhs.y) + (z() * rhs.z);
        }

        // [GEO-UNIT-002] 恢复方向相加减的能力，但在代数闭包上强制返回 Vector3
        template <typename U>
        constexpr auto operator+(const UnitVector3<U, Frame>& rhs) const noexcept {
            using ResT = decltype(x_ + rhs.x());
            return Vector3<ResT, Frame>{x_ + rhs.x(), y_ + rhs.y(), z_ + rhs.z()};
        }

        template <typename U>
        constexpr auto operator-(const UnitVector3<U, Frame>& rhs) const noexcept {
            using ResT = decltype(x_ - rhs.x());
            return Vector3<ResT, Frame>{x_ - rhs.x(), y_ - rhs.y(), z_ - rhs.z()};
        }

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const UnitVector3& rhs) const noexcept {
            return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_;
        }

        constexpr bool operator!=(const UnitVector3& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    // Use only public value accessors; scalar multiplication needs no friendship.
    template <ScalarArithmetic S, std::floating_point T, FrameTag Frame>
    constexpr auto operator*(const S& scalar, const UnitVector3<T, Frame>& v) noexcept {
        using ResT = decltype(scalar * v.x());
        return Vector3<ResT, Frame>{scalar * v.x(), scalar * v.y(), scalar * v.z()};
    }

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <ScalarArithmetic T, FrameTag Frame>
    [[nodiscard]] inline bool AlmostEqual(
        const UnitVector3<T, Frame>& a,
        const UnitVector3<T, Frame>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        return Traits::AlmostEqual(a.x(), b.x(), absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y(), b.y(), absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z(), b.z(), absoluteTolerance, relativeTolerance);
    }

    // --- Geometry traits and source layout checks ---
    template<typename T, FrameTag Frame>
    struct GeometryTraits<UnitVector3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;

        static_assert(Detail::GeometryABIValidator<UnitVector3<T, Frame>>::value,
            "UnitVector3 failed source layout constraints.");

        // No friendship: user specializations of GeometryTraits must not gain
        // mutable access to the private components. Retain observable layout checks.
        using UV = UnitVector3<T, Frame>;

        static_assert(sizeof(UnitVector3<T, Frame>) == sizeof(T) * 3, 
            "UnitVector3 source representation contains unexpected padding.");
    };

} // namespace vectoris::numerics::Geometry
