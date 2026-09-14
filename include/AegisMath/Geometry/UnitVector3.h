#pragma once
#include <cstddef>
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Detail/ABI.h"
#include "Traits.h"

// 修正：指向真实的 Core 目录，如果你的实际文件在其他位置，请对应修改此路径
#include "../Core/NumericTraits.h"
#include "../Core/MathFunctions.h"
#include "../Core/Result.h"

namespace AegisMath::Geometry {

    // [GEO-UNIT-001] 内部安全构造标签，用于保全 ABI 并封锁非法实例化
    struct UnitValidatedTag final {};

    // [Phase 2.2] UnitVector3 Core Engine
    template <ScalarArithmetic T, FrameTag Frame = FrameUnknown>
    struct UnitVector3 final {
    public:
        // [GEO-UNIT-001] 恢复 public 成员以满足严格的 standard-layout 和 offsetof 校验
        T x;
        T y;
        T z;

    private:
        // 私有构造，封锁外部直接实例化，确保单位模长不变量
        constexpr UnitVector3(T _x, T _y, T _z, UnitValidatedTag) noexcept
            : x(_x), y(_y), z(_z) {}

    public:
        // 彻底禁用默认构造，防止出现 (0, 0, 0)
        UnitVector3() = delete;

        // --- 核心工厂 ---
        // [GEO-UNIT-003] 泛化输入类型，允许 float32 的 Vector3 生成 double 的 UnitVector3
        template <ScalarArithmetic U>
        static Core::Result<UnitVector3> TryCreate(const Vector3<U, Frame>& input) noexcept {
            if (!Traits::IsFinite(input.x) || !Traits::IsFinite(input.y) || !Traits::IsFinite(input.z)) {
                return Core::Result<UnitVector3>::failure(Core::MathError::non_finite_input);
            }
            using CalcType = decltype(U{} / U{});
            CalcType sq_len = input.dot(input);

            if (Traits::IsZero(sq_len)) {
                return Core::Result<UnitVector3>::failure(Core::MathError::zero_norm);
            }

            CalcType inv_len = CalcType{1} / Core::Math::sqrt(sq_len);
            return Core::Result<UnitVector3>::success(
                UnitVector3(
                    static_cast<T>(input.x * inv_len),
                    static_cast<T>(input.y * inv_len),
                    static_cast<T>(input.z * inv_len),
                    UnitValidatedTag{}
                )
            );
        }

        template <ScalarArithmetic U>
        static bool TryCreate(const Vector3<U, Frame>& input, UnitVector3& out) noexcept {
            auto res = TryCreate(input);
            if (!res.IsSuccess()) {
                return false;
            }
            out = res.Value();
            return true;
        }

        // [GEO-UNIT-005] 显式长度恢复接口 (用于遥测/调试/序列化)
        constexpr Vector3<T, Frame> ToVector() const noexcept {
            return Vector3<T, Frame>{x, y, z};
        }

        // [GEO-UNIT-006] Invariant 运行时/断言检查
        constexpr bool IsValid() const noexcept {
            T sq_len = x*x + y*y + z*z;
            return Traits::AlmostEqual(sq_len, T{1},
                                       Traits::NumericTraits<T>::epsilon() * T{10},
                                       Traits::NumericTraits<T>::epsilon() * T{10});
        }

        // 访问器 (保持兼容)
        constexpr T getX() const noexcept { return x; }
        constexpr T getY() const noexcept { return y; }
        constexpr T getZ() const noexcept { return z; }

        // --- 代数运算 ---

        // 方向乘长度：UnitVector3 * Scalar = Vector3
        template <ScalarArithmetic S>
        constexpr auto operator*(const S& scalar) const noexcept {
            using ResT = decltype(x * scalar);
            return Vector3<ResT, Frame>{x * scalar, y * scalar, z * scalar};
        }

        template <ScalarArithmetic S>
        friend constexpr auto operator*(const S& scalar, const UnitVector3& v) noexcept {
            using ResT = decltype(scalar * v.x);
            return Vector3<ResT, Frame>{scalar * v.x, scalar * v.y, scalar * v.z};
        }

        constexpr UnitVector3 operator-() const noexcept {
            return UnitVector3(-x, -y, -z, UnitValidatedTag{});
        }

        template <ScalarArithmetic U>
        constexpr auto dot(const UnitVector3<U, Frame>& rhs) const noexcept {
            return (x * rhs.x) + (y * rhs.y) + (z * rhs.z);
        }

        template <ScalarArithmetic U>
        constexpr auto dot(const Vector3<U, Frame>& rhs) const noexcept {
            return (x * rhs.x) + (y * rhs.y) + (z * rhs.z);
        }

        // [GEO-UNIT-002] 恢复方向相加减的能力，但在代数闭包上强制返回 Vector3
        template <typename U>
        constexpr auto operator+(const UnitVector3<U, Frame>& rhs) const noexcept {
            using ResT = decltype(x + rhs.x);
            return Vector3<ResT, Frame>{x + rhs.x, y + rhs.y, z + rhs.z};
        }

        template <typename U>
        constexpr auto operator-(const UnitVector3<U, Frame>& rhs) const noexcept {
            using ResT = decltype(x - rhs.x);
            return Vector3<ResT, Frame>{x - rhs.x, y - rhs.y, z - rhs.z};
        }

        // 精确结构相等性判定
        constexpr bool operator==(const UnitVector3& rhs) const noexcept {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(const UnitVector3& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <ScalarArithmetic T, FrameTag Frame>
    [[nodiscard]] inline bool AlmostEqual(
        const UnitVector3<T, Frame>& a,
        const UnitVector3<T, Frame>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        return Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
    }

    // --- Geometry Traits 与 ABI Contract 联合注册 ---
    template<typename T, FrameTag Frame>
    struct GeometryTraits<UnitVector3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;

        static_assert(Detail::GeometryABIValidator<UnitVector3<T, Frame>>::value,
            "UnitVector3 failed base ABI constraints.");

        // 精确的 offsetof 验证，防止私有继承或优化器带来的内存缝隙
        using UV = UnitVector3<T, Frame>;
        static_assert(offsetof(UV, x) == 0, "x offset mismatch");
        static_assert(offsetof(UV, y) == sizeof(T), "y offset mismatch");
        static_assert(offsetof(UV, z) == sizeof(T) * 2, "z offset mismatch");

        static_assert(sizeof(UnitVector3<T, Frame>) == sizeof(T) * 3, 
            "UnitVector3 memory layout contains padding, which violates DMA alignment.");
    };

} // namespace AegisMath::Geometry