#pragma once

#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Units/DerivedUnits/Power.h"

namespace AegisMath::Dynamics {

    // 空间力/力矩（Wrench6）：包含力和力矩 (强类型物理量)
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct Wrench6 final {
        Force3<Frame, T> force;    // [Fx, Fy, Fz] (N)
        Torque3<Frame, T> moment;  // [Mx, My, Mz] (N·m)

        constexpr Wrench6() noexcept : force{}, moment{} {}

        constexpr Wrench6(
            const Force3<Frame, T>& f,
            const Torque3<Frame, T>& m
        ) noexcept : force(f), moment(m) {}

        static constexpr Wrench6 Zero() noexcept {
            return Wrench6();
        }

        // 共轭对偶功率计算: P = F^T * xi = f·v + tau·omega (返回强类型 Watt 功率)
        constexpr Units::Quantity<T, Units::WattUnit> Power(const Twist6<T, Frame>& twist) const noexcept {
            Units::Quantity<T, Units::WattUnit> p_trans = Dot(force, twist.linear);
            Units::Quantity<T, Units::WattUnit> p_rot   = Dot(moment, twist.angular);
            return p_trans + p_rot;
        }
    };

} // namespace AegisMath::Dynamics