#pragma once

#include "AegisMath/Core/Result.h"
#include "AegisMath/Core/MathError.h"
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
        static constexpr Core::Result<bool, Core::MathError> Step(
            KinematicState<T, RefFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Units::Quantity<T, Units::SecondUnit> dt
        ) noexcept {
            Acceleration3<BodyFrame, T> lin_accel;
            AngularAcceleration3<BodyFrame, T> ang_accel;

            auto deriv_res = RigidBodyDynamicsKernel<T, RefFrame, BodyFrame>::ComputeDerivative(
                state, params, wrench, lin_accel, ang_accel
            );
            if (!deriv_res.has_value()) {
                return Core::Result<bool, Core::MathError>(deriv_res.error());
            }

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
            //
            // [Quaternion Kinematics Boundary]:
            // Quaternion components are dimensionless unit scalars representing SO(3) rotations.
            // Rotational rate (angularVelocity: rad/s, [A T^-1]) is explicitly extracted as
            // dimensionless radian scalar rates at this audited kinematic boundary.
            const T wx = state.angularVelocity.x.value();
            const T wy = state.angularVelocity.y.value();
            const T wz = state.angularVelocity.z.value();
            const T dt_val = dt.value();

            const T qw = state.attitude.w;
            const T qx = state.attitude.x;
            const T qy = state.attitude.y;
            const T qz = state.attitude.z;

            const T half_dt = static_cast<T>(0.5) * dt_val;

            // q_dot = 0.5 * q * omega
            const T dqw = half_dt * (-qx * wx - qy * wy - qz * wz);
            const T dqx = half_dt * ( qw * wx + qy * wz - qz * wy);
            const T dqy = half_dt * ( qw * wy - qx * wz + qz * wx);
            const T dqz = half_dt * ( qw * wz + qx * wy - qy * wx);

            const T new_w = qw + dqw;
            const T new_x = qx + dqx;
            const T new_y = qy + dqy;
            const T new_z = qz + dqz;

            auto new_att = Geometry::Quaternion<T, BodyFrame, RefFrame>::TryCreate(new_w, new_x, new_y, new_z);
            if (new_att.has_value()) {
                state.attitude = new_att.value();
            }

            return Core::Result<bool, Core::MathError>::success(true);
        }
    };

} // namespace AegisMath::Dynamics
