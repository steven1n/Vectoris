#pragma once

#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/RigidBodyState.h"

namespace AegisMath::Dynamics {

    // 确定性一阶欧拉积分器（支持硬实时无堆分配）
    class EulerIntegrator final {
    public:
        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        static constexpr void Step(
            KinematicState<T, RefFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            T dt
        ) noexcept {
            Geometry::Vector3<T, BodyFrame> lin_accel(0,0,0);
            Geometry::Vector3<T, BodyFrame> ang_accel(0,0,0);

            RigidBodyDynamicsKernel<T, RefFrame, BodyFrame>::ComputeDerivative(
                state, params, wrench, lin_accel, ang_accel
            );

            // 更新线速度与角速度
            state.linearVelocity.x += lin_accel.x * dt;
            state.linearVelocity.y += lin_accel.y * dt;
            state.linearVelocity.z += lin_accel.z * dt;

            state.angularVelocity.x += ang_accel.x * dt;
            state.angularVelocity.y += ang_accel.y * dt;
            state.angularVelocity.z += ang_accel.z * dt;

            // 更新位置（基于当前机体速度简单推进）
            state.position.x += state.linearVelocity.x * dt;
            state.position.y += state.linearVelocity.y * dt;
            state.position.z += state.linearVelocity.z * dt;
        }
    };

} // namespace AegisMath::Dynamics