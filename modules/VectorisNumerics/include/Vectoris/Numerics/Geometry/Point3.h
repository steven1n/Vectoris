#pragma once
#include "Namespace.h"
#include <type_traits>
#include <utility>
#include <cstddef>
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Detail/ABI.h"
#include "Traits.h"

namespace vectoris::numerics::Geometry {

    template <ScalarArithmetic T, FrameTag Frame = FrameUnknown>
    struct Point3 final {
        T x;
        T y;
        T z;

    private:
        // Preserve reference scalars; otherwise move only when it is safe.
        static constexpr decltype(auto) ComponentArgument(T& value) noexcept {
            if constexpr (std::is_lvalue_reference_v<T>) {
                return (value);
            } else {
                return std::move_if_noexcept(value);
            }
        }

    public:
        constexpr Point3() noexcept(std::is_arithmetic_v<T>) : x{}, y{}, z{} {}
        constexpr Point3(T _x, T _y, T _z) noexcept(std::is_arithmetic_v<T>) : x(ComponentArgument(_x)), y(ComponentArgument(_y)), z(ComponentArgument(_z)) {}

        // Point + Vector = Point
        template <ScalarArithmetic U>
        constexpr auto operator+(const Vector3<U, Frame>& vec) const noexcept(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            return Point3<decltype(x + vec.x), Frame>{x + vec.x, y + vec.y, z + vec.z};
        }

        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, Frame>)
        constexpr auto operator+(const Vector3<U, OtherFrame>&) const = delete;

        // Point - Point = Vector (必须同 Frame)
        template <ScalarArithmetic U>
        constexpr auto operator-(const Point3<U, Frame>& rhs) const noexcept(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            return Vector3<decltype(x - rhs.x), Frame>{x - rhs.x, y - rhs.y, z - rhs.z};
        }

        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, Frame>)
        constexpr auto operator-(const Point3<U, OtherFrame>&) const = delete;

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const Point3& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(const Point3& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return !(*this == rhs);
        }
    };

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <ScalarArithmetic T, FrameTag Frame>
    [[nodiscard]] inline bool AlmostEqual(
        const Point3<T, Frame>& a,
        const Point3<T, Frame>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept(std::is_arithmetic_v<T>) {
        return Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
    }

    template<typename T, FrameTag Frame>
    struct GeometryTraits<Point3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;
    };

    template<typename T, FrameTag Frame>
    struct PointABIContract {
        using P = Point3<T, Frame>;
        static_assert(Detail::GeometryABIValidator<P>::value, "Point3 failed source layout constraints.");

        static_assert(offsetof(P, x) == 0, "Point3 x-offset mismatch");
        static_assert(offsetof(P, y) == sizeof(T), "Point3 y-offset mismatch");
        static_assert(offsetof(P, z) == sizeof(T) * 2, "Point3 z-offset mismatch");
        static_assert(sizeof(P) == sizeof(T) * 3, "Point3 size contains padding");

        static constexpr bool value = true;
    };

} // namespace vectoris::numerics::Geometry
