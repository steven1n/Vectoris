#pragma once

#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Units/BaseUnits/Mass.h"

namespace AegisMath::Dynamics {

    // 刚体固定物理参数（设计与配置阶段生命周期，强类型物理量）
    template <DynamicsScalar T, Geometry::FrameTag BodyFrame>
    struct RigidBodyParameters final {
        Units::Quantity<T, Units::KilogramUnit> mass;           // 质量 (kg)
        Position3<BodyFrame, T> centerOfMass;                   // 质心位置 (m)
        InertiaTensor3<T, BodyFrame> inertia;                   // 惯量张量 (kg·m^2)

        constexpr RigidBodyParameters(
            Units::Quantity<T, Units::KilogramUnit> m,
            const Position3<BodyFrame, T>& com,
            const InertiaTensor3<T, BodyFrame>& inh
        ) noexcept : mass(m), centerOfMass(com), inertia(inh) {}
    };

} // namespace AegisMath::Dynamics