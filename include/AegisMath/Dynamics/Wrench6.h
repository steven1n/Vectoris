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

        constexpr Wrench6(
            const Force3<Frame, T>& f,
            const Torque3<Frame, T>& m
        ) noexcept : force(f), moment(m) {}

        // 共轭对偶功率计算: P = F^T * xi = f·v + tau·omega (返回强类型 Watt 功率)
        constexpr Units::Quantity<T, Units::WattUnit> Power(const Twist6<T, Frame>& twist) const noexcept {
            T p_trans = force.x.value() * twist.linear.x.value() +
                        force.y.value() * twist.linear.y.value() +
                        force.z.value() * twist.linear.z.value();
            T p_rot   = moment.x.value() * twist.angular.x.value() +
                        moment.y.value() * twist.angular.y.value() +
                        moment.z.value() * twist.angular.z.value();
            return Units::Quantity<T, Units::WattUnit>(p_trans + p_rot);
        }
    };

} // namespace AegisMath::Dynamics