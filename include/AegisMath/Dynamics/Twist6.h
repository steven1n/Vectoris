#pragma once

#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Concepts.h"

namespace AegisMath::Dynamics {

    // 空间速度（Body Twist）：包含线速度与角速度
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct Twist6 final {
        Geometry::Vector3<T, Frame> linear;   // [u, v, w]
        Geometry::Vector3<T, Frame> angular;  // [p, q, r]

        constexpr Twist6(
            const Geometry::Vector3<T, Frame>& lin,
            const Geometry::Vector3<T, Frame>& ang
        ) noexcept : linear(lin), angular(ang) {}
    };

} // namespace AegisMath::Dynamics