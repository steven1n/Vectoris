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

            // 1. 半隐式欧拉：优先更新机体线速度与角速度
            state.linearVelocity.x += lin_accel.x * dt;
            state.linearVelocity.y += lin_accel.y * dt;
            state.linearVelocity.z += lin_accel.z * dt;

            state.angularVelocity.x += ang_accel.x * dt;
            state.angularVelocity.y += ang_accel.y * dt;
            state.angularVelocity.z += ang_accel.z * dt;

            // 2. 坐标系安全变换：将机体线速度投影至参考坐标系
            auto vel_ref = state.attitude * state.linearVelocity;

            // 3. 更新参考系位置
            state.position.x += vel_ref.x * dt;
            state.position.y += vel_ref.y * dt;
            state.position.z += vel_ref.z * dt;

            // 4. 刚体姿态四元数一阶运动学推进：
            // dq/dt = 0.5 * q ⊗ omega_body
            // q_{k+1} = normalize(q_k + dq/dt * dt)
            T half_dt = static_cast<T>(0.5) * dt;
            T qw = state.attitude.w;
            T qx = state.attitude.x;
            T qy = state.attitude.y;
            T qz = state.attitude.z;

            T wx = state.angularVelocity.x;
            T wy = state.angularVelocity.y;
            T wz = state.angularVelocity.z;

            T new_w = qw + half_dt * (-qx * wx - qy * wy - qz * wz);
            T new_x = qx + half_dt * ( qw * wx + qy * wz - qz * wy);
            T new_y = qy + half_dt * ( qw * wy + qz * wx - qx * wz);
            T new_z = qz + half_dt * ( qw * wz + qx * wy - qy * wx);

            auto q_res = Geometry::Quaternion<T, BodyFrame, RefFrame>::TryCreate(new_w, new_x, new_y, new_z);
            if (q_res.IsSuccess()) {
                state.attitude = q_res.Value();
            }
        }
    };

} // namespace AegisMath::Dynamics