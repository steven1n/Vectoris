#pragma once
#include "Namespace.h"
#include <cstddef> // for offsetof
#include <utility>
#include "Concepts.h"
#include "FrameTags.h"
#include "Detail/ABI.h"
#include "Traits.h"
#include "../Core/NumericTraits.h"

namespace vectoris::numerics::Geometry {

    template <ScalarArithmetic T, FrameTag Frame = FrameUnknown>
    struct Vector3 final {
        T x;
        T y;
        T z;

    private:
        static constexpr decltype(auto) ComponentArgument(T& value) noexcept {
            if constexpr (std::is_lvalue_reference_v<T>) {
                return (value);
            } else {
                return std::move_if_noexcept(value);
            }
        }

    public:
        // [GEO-001] 强制值初始化，杜绝随机未定义状态
        constexpr Vector3() noexcept(std::is_nothrow_default_constructible_v<T>) : x{}, y{}, z{} {}
        constexpr Vector3(T _x, T _y, T _z)
            noexcept(std::is_nothrow_constructible_v<T, decltype(ComponentArgument(_x))>)
            : x(ComponentArgument(_x)), y(ComponentArgument(_y)), z(ComponentArgument(_z)) {}

        // [GEO-004] Unary minus
        constexpr auto operator-() const
            noexcept(noexcept(Vector3<decltype(-x), Frame>{-x, -y, -z})) {
            return Vector3<decltype(-x), Frame>{-x, -y, -z};
        }

        template <ScalarArithmetic U>
        constexpr auto operator+(const Vector3<U, Frame>& rhs) const
            noexcept(noexcept(Vector3<decltype(x + rhs.x), Frame>{x + rhs.x, y + rhs.y, z + rhs.z})) {
            return Vector3<decltype(x + rhs.x), Frame>{x + rhs.x, y + rhs.y, z + rhs.z};
        }

        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, Frame>)
        constexpr auto operator+(const Vector3<U, OtherFrame>&) const = delete;

        template <ScalarArithmetic U>
        constexpr auto operator-(const Vector3<U, Frame>& rhs) const
            noexcept(noexcept(Vector3<decltype(x - rhs.x), Frame>{x - rhs.x, y - rhs.y, z - rhs.z})) {
            return Vector3<decltype(x - rhs.x), Frame>{x - rhs.x, y - rhs.y, z - rhs.z};
        }

        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, Frame>)
        constexpr auto operator-(const Vector3<U, OtherFrame>&) const = delete;

        template <ScalarArithmetic S>
        constexpr auto operator*(const S& scalar) const
            noexcept(noexcept(Vector3<decltype(x * scalar), Frame>{x * scalar, y * scalar, z * scalar})) {
            return Vector3<decltype(x * scalar), Frame>{x * scalar, y * scalar, z * scalar};
        }

        // [GEO-005] 标量左乘 (友元函数)
        template <ScalarArithmetic S>
        friend constexpr auto operator*(const S& scalar, const Vector3& v)
            noexcept(noexcept(Vector3<decltype(scalar * v.x), Frame>{scalar * v.x, scalar * v.y, scalar * v.z})) {
            return Vector3<decltype(scalar * v.x), Frame>{scalar * v.x, scalar * v.y, scalar * v.z};
        }

        // 点积代数运算 (必须在同坐标系 Frame 下进行)
        template <ScalarArithmetic U>
        constexpr auto dot(const Vector3<U, Frame>& rhs) const
            noexcept(noexcept(std::remove_cvref_t<decltype(x * rhs.x + y * rhs.y + z * rhs.z)>(
                x * rhs.x + y * rhs.y + z * rhs.z))) {
            return x * rhs.x + y * rhs.y + z * rhs.z;
        }

        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, Frame>)
        constexpr auto dot(const Vector3<U, OtherFrame>&) const = delete;

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const Vector3& rhs) const
            noexcept(noexcept(static_cast<bool>(x == rhs.x && y == rhs.y && z == rhs.z))) {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(const Vector3& rhs) const noexcept(noexcept(*this == rhs)) {
            return !(*this == rhs);
        }
    };

    template <ScalarArithmetic T, FrameTag Frame>
    inline constexpr bool is_geometry_aggregate_v<Vector3<T, Frame>> = true;

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <Concepts::FloatingPoint T, FrameTag Frame>
    [[nodiscard]] inline bool AlmostEqual(
        const Vector3<T, Frame>& a,
        const Vector3<T, Frame>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept(noexcept(Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance))) {
        return Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
    }

    // [GEO-006] Traits 注册
    template<typename T, FrameTag Frame>
    struct GeometryTraits<Vector3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;
    };

    // Source-level layout checks; not a cross-build ABI guarantee.
    template<typename T, FrameTag Frame>
    struct VectorABIContract {
        using V = Vector3<T, Frame>;
        static_assert(Detail::GeometryABIValidator<V>::value, "Vector3 failed source layout constraints.");

        // 精确验证内存布局无缝隙
        static_assert(offsetof(V, x) == 0, "Vector3 x-offset mismatch");
        static_assert(offsetof(V, y) == sizeof(T), "Vector3 y-offset mismatch");
        static_assert(offsetof(V, z) == sizeof(T) * 2, "Vector3 z-offset mismatch");
        static_assert(sizeof(V) == sizeof(T) * 3, "Vector3 size contains padding");

        static constexpr bool value = true;
    };

} // namespace vectoris::numerics::Geometry
