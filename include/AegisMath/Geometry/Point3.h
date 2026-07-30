#pragma once
#include <cstddef>
#include "AegisMath/Dynamics/Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Detail/ABI.h"
#include "Traits.h"

namespace AegisMath::Geometry {

    template <ScalarArithmetic T, FrameTag Frame = FrameUnknown>
    struct Point3 final {
        T x;
        T y;
        T z;

        constexpr Point3() noexcept : x{}, y{}, z{} {}
        constexpr Point3(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z) {}

        // Point + Vector = Point
        template <ScalarArithmetic U>
        constexpr auto operator+(const Vector3<U, Frame>& vec) const noexcept {
            return Point3<decltype(x + vec.x), Frame>{x + vec.x, y + vec.y, z + vec.z};
        }

        // Point - Point = Vector (必须同 Frame)
        template <ScalarArithmetic U>
        constexpr auto operator-(const Point3<U, Frame>& rhs) const noexcept {
            return Vector3<decltype(x - rhs.x), Frame>{x - rhs.x, y - rhs.y, z - rhs.z};
        }
    };

    template<typename T, FrameTag Frame>
    struct GeometryTraits<Point3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;
    };

    template<typename T, FrameTag Frame>
    struct PointABIContract {
        using P = Point3<T, Frame>;
        static_assert(Detail::GeometryABIValidator<P>::value, "Point3 failed base ABI.");

        static_assert(offsetof(P, x) == 0, "Point3 x-offset mismatch");
        static_assert(offsetof(P, y) == sizeof(T), "Point3 y-offset mismatch");
        static_assert(offsetof(P, z) == sizeof(T) * 2, "Point3 z-offset mismatch");
        static_assert(sizeof(P) == sizeof(T) * 3, "Point3 size contains padding");

        static constexpr bool value = true;
    };

} // namespace AegisMath::Geometry