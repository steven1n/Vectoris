#pragma once

#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Dynamics/Concepts.h"

namespace AegisMath::Dynamics {

    // 刚体固定物理参数（设计与配置阶段生命周期）
    template <DynamicsScalar T, Geometry::FrameTag BodyFrame>
    struct RigidBodyParameters final {
        T mass;                                   // 质量 (kg)
        Geometry::Vector3<T, BodyFrame> centerOfMass; // 质心位置
        InertiaTensor3<T, BodyFrame> inertia;     // 惯量张量

        constexpr RigidBodyParameters(
            T m, 
            const Geometry::Vector3<T, BodyFrame>& com, 
            const InertiaTensor3<T, BodyFrame>& inh
        ) noexcept : mass(m), centerOfMass(com), inertia(inh) {}
    };

} // namespace AegisMath::Dynamics