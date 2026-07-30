#pragma once
#include <cstddef> // for offsetof
#include "AegisMath/Dynamics/Concepts.h"
#include "FrameTags.h"
#include "Detail/ABI.h"
#include "Traits.h"

namespace AegisMath::Geometry {

    template <ScalarArithmetic T, FrameTag Frame = FrameUnknown>
    struct Vector3 final {
        T x;
        T y;
        T z;

        // [GEO-001] 强制值初始化，杜绝随机未定义状态
        constexpr Vector3() noexcept : x{}, y{}, z{} {}
        constexpr Vector3(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z) {}

        // [GEO-004] Unary minus
        constexpr auto operator-() const noexcept {
            return Vector3<decltype(-x), Frame>{-x, -y, -z};
        }

        template <ScalarArithmetic U>
        constexpr auto operator+(const Vector3<U, Frame>& rhs) const noexcept {
            return Vector3<decltype(x + rhs.x), Frame>{x + rhs.x, y + rhs.y, z + rhs.z};
        }

        template <ScalarArithmetic U>
        constexpr auto operator-(const Vector3<U, Frame>& rhs) const noexcept {
            return Vector3<decltype(x - rhs.x), Frame>{x - rhs.x, y - rhs.y, z - rhs.z};
        }

        template <ScalarArithmetic S>
        constexpr auto operator*(const S& scalar) const noexcept {
            return Vector3<decltype(x * scalar), Frame>{x * scalar, y * scalar, z * scalar};
        }

        // [GEO-005] 标量左乘 (友元函数)
        template <ScalarArithmetic S>
        friend constexpr auto operator*(const S& scalar, const Vector3& v) noexcept {
            return Vector3<decltype(scalar * v.x), Frame>{scalar * v.x, scalar * v.y, scalar * v.z};
        }
    };

    // [GEO-006] Traits 注册
    template<typename T, FrameTag Frame>
    struct GeometryTraits<Vector3<T, Frame>> {
        static constexpr size_t Dimension = 3;
        using ScalarType = T;
        using FrameType = Frame;
    };

    // [GEO-002, GEO-003] ABI Contract
    template<typename T, FrameTag Frame>
    struct VectorABIContract {
        using V = Vector3<T, Frame>;
        static_assert(Detail::GeometryABIValidator<V>::value, "Vector3 failed base ABI.");

        // 精确验证内存布局无缝隙
        static_assert(offsetof(V, x) == 0, "Vector3 x-offset mismatch");
        static_assert(offsetof(V, y) == sizeof(T), "Vector3 y-offset mismatch");
        static_assert(offsetof(V, z) == sizeof(T) * 2, "Vector3 z-offset mismatch");
        static_assert(sizeof(V) == sizeof(T) * 3, "Vector3 size contains padding");

        static constexpr bool value = true;
    };

} // namespace AegisMath::Geometry