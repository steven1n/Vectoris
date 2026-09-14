#pragma once

#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/QuantityVector3.h"

namespace AegisMath::Dynamics {

    // 空间速度（Body Twist）：包含线速度与角速度 (强类型物理量)
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct Twist6 final {
        Velocity3<Frame, T> linear;          // [u, v, w] (m/s)
        AngularVelocity3<Frame, T> angular;  // [p, q, r] (rad/s)

        constexpr Twist6(
            const Velocity3<Frame, T>& lin,
            const AngularVelocity3<Frame, T>& ang
        ) noexcept : linear(lin), angular(ang) {}
    };

} // namespace AegisMath::Dynamics