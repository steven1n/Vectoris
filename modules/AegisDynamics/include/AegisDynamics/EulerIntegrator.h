#pragma once

#include "AegisMath/Core/Result.h"
#include "AegisMath/Core/MathError.h"
#include "AegisMath/Core/NumericTraits.h"
#include "AegisDynamics/Detail/StateTypes.h"
#include "AegisDynamics/RigidBodyParameters.h"
#include "AegisDynamics/Wrench6.h"
#include "AegisDynamics/RigidBodyState.h"
#include "AegisDynamics/QuantityVector3.h"
#include "AegisMath/Units/BaseUnits/Time.h"

namespace AegisDynamics {

    // 确定性一阶欧拉积分器（支持硬实时无堆分配，强类型物理量推进，强事务状态一致性保证）
    class EulerIntegrator final {
    public:
        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        static constexpr Core::Result<bool, Core::MathError> Step(
            KinematicState<T, RefFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Units::Quantity<T, Units::SecondUnit> dt
        ) noexcept {
            using ResultType = Core::Result<bool, Core::MathError>;

            // 1. 时间步参数校验 (严格正向推进且有限)
            if (!Traits::IsFinite(dt.value())) {
                return ResultType(Core::MathError::non_finite_input);
            }
            if (dt.value() <= static_cast<T>(0)) {
                return ResultType(Core::MathError::invalid_argument);
            }

            // 2. 事务性候选状态副本 (栈上轻量值拷贝，零堆分配，杜绝局部提交破坏一致性)
            auto candidate = state;

            Acceleration3<BodyFrame, T> lin_accel;
            AngularAcceleration3<BodyFrame, T> ang_accel;

            auto deriv_res = RigidBodyDynamicsKernel<T, RefFrame, BodyFrame>::ComputeDerivative(
                candidate, params, wrench, lin_accel, ang_accel
            );
            if (!deriv_res.has_value()) {
                return ResultType(deriv_res.error());
            }

            // 3. 半隐式欧拉：更新候选机体速度 (v += a * dt, w += alpha * dt)
            candidate.linearVelocity += lin_accel * dt;
            candidate.angularVelocity += ang_accel * dt;

            // 4. 坐标系安全变换：将候选机体线速度投影至参考坐标系并推进位置 (pos += vel_ref * dt)
            auto vel_ref = candidate.attitude * candidate.linearVelocity;
            candidate.position += vel_ref * dt;

            // 5. 刚体姿态四元数一阶运动学推进：
            // dq/dt = 0.5 * q ⊗ omega_body
            // q_{k+1} = normalize(q_k + dq/dt * dt)
            //
            // [Quaternion Kinematics Boundary]:
            // Quaternion components are dimensionless unit scalars representing SO(3) rotations.
            // Rotational rate (angularVelocity: rad/s, [A T^-1]) is explicitly extracted as
            // dimensionless radian scalar rates at this audited kinematic boundary.
            const T wx = candidate.angularVelocity.x.value();
            const T wy = candidate.angularVelocity.y.value();
            const T wz = candidate.angularVelocity.z.value();
            const T dt_val = dt.value();

            const T qw = candidate.attitude.w;
            const T qx = candidate.attitude.x;
            const T qy = candidate.attitude.y;
            const T qz = candidate.attitude.z;

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
            if (!new_att.has_value()) {
                return ResultType(new_att.error());
            }
            candidate.attitude = new_att.value();

            // 6. 事务原子提交：全步骤全部成功时方才变更外部状态
            state = candidate;
            return ResultType::success(true);
        }
    };

} // namespace AegisDynamics
