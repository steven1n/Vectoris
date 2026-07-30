#pragma once

#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Dynamics/Concepts.h"

namespace AegisMath::Dynamics {

    // 空间力/力矩（Wrench6）：包含力和力矩
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct Wrench6 final {
        Geometry::Vector3<T, Frame> force;   // [Fx, Fy, Fz]
        Geometry::Vector3<T, Frame> moment;  // [Mx, My, Mz]

        constexpr Wrench6(
            const Geometry::Vector3<T, Frame>& f,
            const Geometry::Vector3<T, Frame>& m
        ) noexcept : force(f), moment(m) {}

        // 共轭对偶功率计算: P = F^T * xi = f·v + tau·omega
        constexpr T Power(const Twist6<T, Frame>& twist) const noexcept {
            return (force.x * twist.linear.x + force.y * twist.linear.y + force.z * twist.linear.z) +
                   (moment.x * twist.angular.x + moment.y * twist.angular.y + moment.z * twist.angular.z);
        }
    };

} // namespace AegisMath::Dynamics