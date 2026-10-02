#pragma once

#include <type_traits>
#include <concepts>
#include <cstddef>
#include "Vectoris/Numerics/Core/Precision.h"
#include "Vectoris/Dynamics/Concepts.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Numerics/Units/Core.h"
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"
#include "Vectoris/Numerics/Units/BaseUnits/Time.h"
#include "Vectoris/Numerics/Units/BaseUnits/Mass.h"
#include "Vectoris/Numerics/Units/BaseUnits/Angle.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Velocity.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Acceleration.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularVelocity.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularAcceleration.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Force.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Torque.h"
#include "Vectoris/Numerics/Units/DerivedUnits/MomentOfInertia.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Power.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularMomentum.h"
#include "Vectoris/Numerics/Units/BaseUnits/Angle.h"

namespace vectoris::dynamics {

    // 强类型物理量空间向量 (3D Dimensional Vector with Coordinate Frame Binding)
    template <typename QuantityType, Geometry::FrameTag Frame>
    requires Units::IsQuantity<QuantityType>
    struct QuantityVector3 final {
        using Quantity_t = QuantityType;
        using ValueType  = typename QuantityType::ValueType;
        using UnitType   = typename QuantityType::UnitType;
        using FrameType  = Frame;

        QuantityType x;
        QuantityType y;
        QuantityType z;

        constexpr QuantityVector3() noexcept
            : x(QuantityType::Zero()), y(QuantityType::Zero()), z(QuantityType::Zero()) {}

        constexpr QuantityVector3(QuantityType _x, QuantityType _y, QuantityType _z) noexcept
            : x(_x), y(_y), z(_z) {}

        // 同坐标系下具备兼容量纲的向量隐式转换构造 (例如由乘法导出的中间量纲向量转换为标准量纲向量)
        template <typename OtherQuantity>
        requires Units::IsQuantity<OtherQuantity> &&
                 std::is_constructible_v<QuantityType, OtherQuantity> &&
                 (!std::is_same_v<QuantityType, OtherQuantity>)
        constexpr QuantityVector3(const QuantityVector3<OtherQuantity, Frame>& other) noexcept
            : x(QuantityType(other.x)), y(QuantityType(other.y)), z(QuantityType(other.z)) {}

        // 基础向量代数（必须在同量纲且同坐标系 Frame 下进行）
        [[nodiscard]] constexpr QuantityVector3 operator+(const QuantityVector3& rhs) const noexcept {
            return QuantityVector3(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        template <typename OtherQuantity>
        requires Units::IsQuantity<OtherQuantity> &&
                 std::is_constructible_v<QuantityType, OtherQuantity> &&
                 (!std::is_same_v<QuantityType, OtherQuantity>)
        [[nodiscard]] constexpr QuantityVector3 operator+(const QuantityVector3<OtherQuantity, Frame>& rhs) const noexcept {
            return QuantityVector3(x + QuantityType(rhs.x), y + QuantityType(rhs.y), z + QuantityType(rhs.z));
        }

        [[nodiscard]] constexpr QuantityVector3 operator-(const QuantityVector3& rhs) const noexcept {
            return QuantityVector3(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        template <typename OtherQuantity>
        requires Units::IsQuantity<OtherQuantity> &&
                 std::is_constructible_v<QuantityType, OtherQuantity> &&
                 (!std::is_same_v<QuantityType, OtherQuantity>)
        [[nodiscard]] constexpr QuantityVector3 operator-(const QuantityVector3<OtherQuantity, Frame>& rhs) const noexcept {
            return QuantityVector3(x - QuantityType(rhs.x), y - QuantityType(rhs.y), z - QuantityType(rhs.z));
        }

        constexpr QuantityVector3& operator+=(const QuantityVector3& rhs) noexcept {
            x += rhs.x; y += rhs.y; z += rhs.z; return *this;
        }

        template <typename OtherQuantity>
        requires Units::IsQuantity<OtherQuantity> &&
                 std::is_constructible_v<QuantityType, OtherQuantity>
        constexpr QuantityVector3& operator+=(const QuantityVector3<OtherQuantity, Frame>& rhs) noexcept {
            x += QuantityType(rhs.x); y += QuantityType(rhs.y); z += QuantityType(rhs.z); return *this;
        }

        constexpr QuantityVector3& operator-=(const QuantityVector3& rhs) noexcept {
            x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this;
        }

        template <typename OtherQuantity>
        requires Units::IsQuantity<OtherQuantity> &&
                 std::is_constructible_v<QuantityType, OtherQuantity>
        constexpr QuantityVector3& operator-=(const QuantityVector3<OtherQuantity, Frame>& rhs) noexcept {
            x -= QuantityType(rhs.x); y -= QuantityType(rhs.y); z -= QuantityType(rhs.z); return *this;
        }

        [[nodiscard]] constexpr QuantityVector3 operator-() const noexcept {
            return QuantityVector3(-x, -y, -z);
        }

        // 无量纲纯标量缩放 (Floating-point scalar multiplication & division)
        template <Concepts::FloatingPoint S>
        requires std::same_as<S, ValueType>
        [[nodiscard]] constexpr QuantityVector3 operator*(S scalar) const noexcept {
            return QuantityVector3(x * scalar, y * scalar, z * scalar);
        }

        template <Concepts::FloatingPoint S>
        requires std::same_as<S, ValueType>
        [[nodiscard]] constexpr QuantityVector3 operator/(S scalar) const noexcept {
            return QuantityVector3(x / scalar, y / scalar, z / scalar);
        }

        template <Concepts::FloatingPoint S>
        requires std::same_as<S, ValueType>
        friend constexpr QuantityVector3 operator*(S scalar, const QuantityVector3& v) noexcept {
            return QuantityVector3(scalar * v.x, scalar * v.y, scalar * v.z);
        }

        // 物理量标量乘除 (Dimensional scaling: e.g. Velocity3 * Time -> Position3)
        template <Units::IsUnitTag OtherUnit>
        [[nodiscard]] constexpr auto operator*(const Units::Quantity<ValueType, OtherUnit>& q) const noexcept {
            auto rx = x * q;
            using ResultQ = decltype(rx);
            return QuantityVector3<ResultQ, Frame>(rx, y * q, z * q);
        }

        template <Units::IsUnitTag OtherUnit>
        [[nodiscard]] constexpr auto operator/(const Units::Quantity<ValueType, OtherUnit>& q) const noexcept {
            auto rx = x / q;
            using ResultQ = decltype(rx);
            return QuantityVector3<ResultQ, Frame>(rx, y / q, z / q);
        }
    };

    // 物理量标量左乘 (Quantity * QuantityVector3)
    template <Concepts::FloatingPoint T, Units::IsUnitTag Unit, typename Q, Geometry::FrameTag Frame>
    [[nodiscard]] constexpr auto operator*(const Units::Quantity<T, Unit>& q, const QuantityVector3<Q, Frame>& v) noexcept {
        return v * q;
    }

    // 空间向量点积 (同坐标系，返回代数量纲积)
    template <typename Q1, typename Q2, Geometry::FrameTag Frame>
    [[nodiscard]] constexpr auto Dot(const QuantityVector3<Q1, Frame>& a, const QuantityVector3<Q2, Frame>& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // 空间向量叉积 (通用)
    template <typename Q1, typename Q2, Geometry::FrameTag Frame>
    [[nodiscard]] constexpr auto Cross(const QuantityVector3<Q1, Frame>& a, const QuantityVector3<Q2, Frame>& b) noexcept {
        auto cx = a.y * b.z - a.z * b.y;
        using ResQ = decltype(cx);
        return QuantityVector3<ResQ, Frame>(
            cx,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    // 四元数坐标系变换: q * v_from -> v_to
    // Hamilton 主动旋转, 保留物理量纲, 变换坐标系标签
    template <Concepts::FloatingPoint T, Geometry::FrameTag FrameFrom, Geometry::FrameTag FrameTo, typename Q>
    requires Units::IsQuantity<Q> && std::same_as<T, typename Q::ValueType>
    [[nodiscard]] constexpr QuantityVector3<Q, FrameTo> operator*(
        const Geometry::Quaternion<T, FrameFrom, FrameTo>& q,
        const QuantityVector3<Q, FrameFrom>& v) noexcept
    {
        // Reuse the overflow-safe floating rotation kernel. Unwrap only inside
        // this dimension-preserving adapter; the public input/output retain Q.
        using Value = typename Q::ValueType;
        const Geometry::Vector3<Value, FrameFrom> components{
            v.x.value(), v.y.value(), v.z.value()};
        const auto rotated = q * components;
        return QuantityVector3<Q, FrameTo>{
            Q{rotated.x}, Q{rotated.y}, Q{rotated.z}};
    }

    // 动力学标准化别名 (Dynamics Domain Type Aliases)
    template <Geometry::FrameTag Frame, typename T = Scalar>
    using Position3 = QuantityVector3<Units::Quantity<T, Units::MeterUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using Velocity3 = QuantityVector3<Units::Quantity<T, Units::MeterPerSecondUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using Acceleration3 = QuantityVector3<Units::Quantity<T, Units::MeterPerSecondSquaredUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using AngularVelocity3 = QuantityVector3<Units::Quantity<T, Units::RadianPerSecondUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using AngularAcceleration3 = QuantityVector3<Units::Quantity<T, Units::RadianPerSecondSquaredUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using Force3 = QuantityVector3<Units::Quantity<T, Units::NewtonUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using Torque3 = QuantityVector3<Units::Quantity<T, Units::NewtonMeterUnit>, Frame>;

    template <Geometry::FrameTag Frame, typename T = Scalar>
    using AngularMomentum3 = QuantityVector3<Units::Quantity<T, Units::AngularMomentumUnit>, Frame>;

    // 物理旋转叉积：先执行保留量纲的普通 Cross，再显式除以一个弧度。
    // 只有采用 radian 角坐标约定的旋转运动学/动力学项才应调用本 API。
    // 通用 Cross 始终执行纯量纲代数，不会隐式消去 Angle。
    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Torque3<Frame, T> RotationalCross(
        const AngularVelocity3<Frame, T>& w,
        const AngularMomentum3<Frame, T>& L) noexcept
    {
        auto raw_cross = Cross(w, L);
        constexpr Units::Quantity<T, Units::RadianUnit> one_rad(static_cast<T>(1));
        return raw_cross / one_rad;
    }

    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Torque3<Frame, T> LieBracket(
        const AngularVelocity3<Frame, T>& w,
        const AngularMomentum3<Frame, T>& L) noexcept
    {
        return RotationalCross(w, L);
    }

    // 机体系旋转传输项： (omega x v) / (1 rad) -> Acceleration。
    // 保留 generic Cross 的原始 A L T^-2 量纲，明确由 one_rad 消去 Angle。
    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Acceleration3<Frame, T> RotationalCross(
        const AngularVelocity3<Frame, T>& w,
        const Velocity3<Frame, T>& v) noexcept
    {
        const auto raw_cross = Cross(w, v);
        constexpr Units::Quantity<T, Units::RadianUnit> one_rad(static_cast<T>(1));
        return raw_cross / one_rad;
    }

    // ABI 静态验证
    template <typename QuantityType, Geometry::FrameTag Frame>
    struct QuantityVectorABIContract {
        using V = QuantityVector3<QuantityType, Frame>;
        static_assert(std::is_standard_layout_v<V>, "QuantityVector3 must be standard layout.");
        static_assert(std::is_trivially_copyable_v<V>, "QuantityVector3 must be trivially copyable.");
        static_assert(sizeof(V) == sizeof(QuantityType) * 3, "QuantityVector3 must have no padding.");
        static_assert(alignof(V) == alignof(QuantityType), "QuantityVector3 alignment mismatch.");
        static_assert(offsetof(V, x) == 0, "QuantityVector3 x offset must be 0.");
        static_assert(offsetof(V, y) == sizeof(QuantityType), "QuantityVector3 y offset must be sizeof(Q).");
        static_assert(offsetof(V, z) == sizeof(QuantityType) * 2, "QuantityVector3 z offset must be sizeof(Q)*2.");
        static constexpr bool value = true;
    };

} // namespace vectoris::dynamics
