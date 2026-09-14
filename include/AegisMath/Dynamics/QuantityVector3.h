#pragma once

#include <type_traits>
#include <concepts>
#include <cstddef>
#include "AegisMath/Core/Precision.h"
#include "AegisMath/Core/Concepts.h"
#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Geometry/Quaternion.h"
#include "AegisMath/Units/Core.h"
#include "AegisMath/Units/BaseUnits/Length.h"
#include "AegisMath/Units/BaseUnits/Time.h"
#include "AegisMath/Units/BaseUnits/Mass.h"
#include "AegisMath/Units/BaseUnits/Angle.h"
#include "AegisMath/Units/DerivedUnits/Velocity.h"
#include "AegisMath/Units/DerivedUnits/Acceleration.h"
#include "AegisMath/Units/DerivedUnits/AngularVelocity.h"
#include "AegisMath/Units/DerivedUnits/AngularAcceleration.h"
#include "AegisMath/Units/DerivedUnits/Force.h"
#include "AegisMath/Units/DerivedUnits/Torque.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"
#include "AegisMath/Units/DerivedUnits/Power.h"
#include "AegisMath/Units/DerivedUnits/AngularMomentum.h"
#include "AegisMath/Units/BaseUnits/Angle.h"

namespace AegisMath::Dynamics {

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
    [[nodiscard]] constexpr QuantityVector3<Q, FrameTo> operator*(
        const Geometry::Quaternion<T, FrameFrom, FrameTo>& q,
        const QuantityVector3<Q, FrameFrom>& v) noexcept
    {
        T qw = q.w;
        T qx = q.x;
        T qy = q.y;
        T qz = q.z;

        // uv = q_vec x v
        Q uv_x = qy * v.z - qz * v.y;
        Q uv_y = qz * v.x - qx * v.z;
        Q uv_z = qx * v.y - qy * v.x;

        // uuv = q_vec x uv
        Q uuv_x = qy * uv_z - qz * uv_y;
        Q uuv_y = qz * uv_x - qx * uv_z;
        Q uuv_z = qx * uv_y - qy * uv_x;

        T two = static_cast<T>(2);
        T two_w = two * qw;

        return QuantityVector3<Q, FrameTo>(
            v.x + two_w * uv_x + two * uuv_x,
            v.y + two_w * uv_y + two * uuv_y,
            v.z + two_w * uv_z + two * uuv_z
        );
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

    // 旋转 Lie 括号与动力学截面算子 (Rotational Cross / so(3) Lie Bracket):
    // tau = omega x L / (1 rad)
    // 在 Angle 作为独立基本量纲时，旋转截面算子包含显式 1/rad 规范化因子
    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Torque3<Frame, T> RotationalCross(
        const AngularVelocity3<Frame, T>& w,
        const AngularMomentum3<Frame, T>& L) noexcept
    {
        auto raw_cross = Cross(w, L);
        Units::Quantity<T, Units::RadianUnit> one_rad(static_cast<T>(1.0));
        return raw_cross / one_rad;
    }

    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Torque3<Frame, T> LieBracket(
        const AngularVelocity3<Frame, T>& w,
        const AngularMomentum3<Frame, T>& L) noexcept
    {
        return RotationalCross(w, L);
    }

    // 弧度作为角度量纲在科氏力项交叉时的自然无量纲解算:
    // omega (AngularVelocity) x v (Velocity) -> Acceleration
    template <Geometry::FrameTag Frame, typename T = Scalar>
    [[nodiscard]] constexpr Acceleration3<Frame, T> Cross(
        const AngularVelocity3<Frame, T>& w,
        const Velocity3<Frame, T>& v) noexcept
    {
        using AccelQ = Units::Quantity<T, Units::MeterPerSecondSquaredUnit>;
        return Acceleration3<Frame, T>(
            AccelQ(w.y.value() * v.z.value() - w.z.value() * v.y.value()),
            AccelQ(w.z.value() * v.x.value() - w.x.value() * v.z.value()),
            AccelQ(w.x.value() * v.y.value() - w.y.value() * v.x.value())
        );
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

} // namespace AegisMath::Dynamics
