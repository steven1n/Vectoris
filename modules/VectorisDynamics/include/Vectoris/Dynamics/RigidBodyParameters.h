#pragma once

#include "Vectoris/Dynamics/Concepts.h"
#include "Vectoris/Dynamics/QuantityVector3.h"
#include "Vectoris/Dynamics/InertiaTensor3.h"
#include "Vectoris/Numerics/Units/BaseUnits/Mass.h"

namespace vectoris::dynamics {

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

} // namespace vectoris::dynamics