#pragma once

#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/RigidBodyState.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Units/BaseUnits/Time.h"

namespace AegisMath::Dynamics {

    // 确定性一阶欧拉积分器（支持硬实时无堆分配，强类型物理量推进）
    class EulerIntegrator final {
    public:
        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        static constexpr void Step(
            KinematicState<T, RefFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Units::Quantity<T, Units::SecondUnit> dt
        ) noexcept {
            Acceleration3<BodyFrame, T> lin_accel;
            AngularAcceleration3<BodyFrame, T> ang_accel;

            RigidBodyDynamicsKernel<T, RefFrame, BodyFrame>::ComputeDerivative(
                state, params, wrench, lin_accel, ang_accel
            );

            // 1. 半隐式欧拉：优先更新机体线速度与角速度 (v += a * dt, w += alpha * dt)
            state.linearVelocity += lin_accel * dt;
            state.angularVelocity += ang_accel * dt;

            // 2. 坐标系安全变换：将机体线速度投影至参考坐标系 (vel_ref = q * vel_body)
            auto vel_ref = state.attitude * state.linearVelocity;

            // 3. 更新参考系位置 (pos += vel_ref * dt)
            state.position += vel_ref * dt;

            // 4. 刚体姿态四元数一阶运动学推进：
            // dq/dt = 0.5 * q ⊗ omega_body
            // q_{k+1} = normalize(q_k + dq/dt * dt)
            T dt_sec = dt.value();
            T half_dt = static_cast<T>(0.5) * dt_sec;
            T qw = state.attitude.w;
            T qx = state.attitude.x;
            T qy = state.attitude.y;
            T qz = state.attitude.z;

            T wx = state.angularVelocity.x.value();
            T wy = state.angularVelocity.y.value();
            T wz = state.angularVelocity.z.value();

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