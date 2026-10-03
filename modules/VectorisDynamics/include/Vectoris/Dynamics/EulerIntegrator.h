#pragma once

#include "Vectoris/Numerics/Core/Result.h"
#include "Vectoris/Numerics/Core/MathError.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/RigidBodyState.h"
#include "Vectoris/Dynamics/QuantityVector3.h"
#include "Vectoris/Numerics/Units/BaseUnits/Time.h"

namespace vectoris::dynamics {

    // 一阶欧拉积分器：固定状态维度下结构工作量有界，推进路径无堆分配，强类型且事务性提交。
    // 不保证 WCET、可调度性或硬实时安全；这些性质需要目标平台上的单独资格验证。
    class EulerIntegrator final {
    private:
        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        [[nodiscard]] static constexpr bool IsStateFinite(
            const KinematicState<T, RefFrame, BodyFrame>& s
        ) noexcept {
            return Traits::IsFinite(s.position.x.value()) &&
                   Traits::IsFinite(s.position.y.value()) &&
                   Traits::IsFinite(s.position.z.value()) &&
                   Traits::IsFinite(s.linearVelocity.x.value()) &&
                   Traits::IsFinite(s.linearVelocity.y.value()) &&
                   Traits::IsFinite(s.linearVelocity.z.value()) &&
                   Traits::IsFinite(s.angularVelocity.x.value()) &&
                   Traits::IsFinite(s.angularVelocity.y.value()) &&
                   Traits::IsFinite(s.angularVelocity.z.value()) &&
                   Traits::IsFinite(s.attitude.w) &&
                   Traits::IsFinite(s.attitude.x) &&
                   Traits::IsFinite(s.attitude.y) &&
                   Traits::IsFinite(s.attitude.z);
        }

        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        [[nodiscard]] static constexpr Core::Result<bool, Core::MathError> ValidateStepInputs(
            const KinematicState<T, RefFrame, BodyFrame>& state,
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

            // 2. 刚体物理参数校验 (质量必须有限且严格正定)
            if (!Traits::IsFinite(params.mass.value())) {
                return ResultType(Core::MathError::non_finite_input);
            }
            if (params.mass.value() <= static_cast<T>(0)) {
                return ResultType(Core::MathError::invalid_argument);
            }

            // 3. 外力和力矩输入有限性校验
            if (!Traits::IsFinite(wrench.force.x.value()) ||
                !Traits::IsFinite(wrench.force.y.value()) ||
                !Traits::IsFinite(wrench.force.z.value()) ||
                !Traits::IsFinite(wrench.moment.x.value()) ||
                !Traits::IsFinite(wrench.moment.y.value()) ||
                !Traits::IsFinite(wrench.moment.z.value())) {
                return ResultType(Core::MathError::non_finite_input);
            }

            // 4. 输入初始状态有限性校验
            if (!IsStateFinite(state)) {
                return ResultType(Core::MathError::non_finite_input);
            }

            return ResultType::success(true);
        }

    public:
        template <DynamicsScalar T, Geometry::FrameTag RefFrame, Geometry::FrameTag BodyFrame>
        static constexpr Core::Result<bool, Core::MathError> Step(
            KinematicState<T, RefFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Units::Quantity<T, Units::SecondUnit> dt
        ) noexcept {
            using ResultType = Core::Result<bool, Core::MathError>;

            // 1. 输入参数与初始状态前提条件校验
            auto input_check = ValidateStepInputs(state, params, wrench, dt);
            if (!input_check.has_value()) {
                return input_check;
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

            // 校验候选速度是否发生数值溢出
            if (!Traits::IsFinite(candidate.linearVelocity.x.value()) ||
                !Traits::IsFinite(candidate.linearVelocity.y.value()) ||
                !Traits::IsFinite(candidate.linearVelocity.z.value()) ||
                !Traits::IsFinite(candidate.angularVelocity.x.value()) ||
                !Traits::IsFinite(candidate.angularVelocity.y.value()) ||
                !Traits::IsFinite(candidate.angularVelocity.z.value())) {
                return ResultType(Core::MathError::non_finite_input);
            }

            // 4. 坐标系安全变换：将候选机体线速度投影至参考坐标系并推进位置 (pos += vel_ref * dt)
            auto vel_ref = candidate.attitude * candidate.linearVelocity;
            candidate.position += vel_ref * dt;

            // 校验候选位置是否发生数值溢出
            if (!Traits::IsFinite(candidate.position.x.value()) ||
                !Traits::IsFinite(candidate.position.y.value()) ||
                !Traits::IsFinite(candidate.position.z.value())) {
                return ResultType(Core::MathError::non_finite_input);
            }

            // 5. 刚体姿态四元数一阶运动学推进：
            // dq/dt = 0.5 * q ⊗ (omega_body / (1 rad))
            // q_{k+1} = normalize(q_k + 0.5 * q_k ⊗ (omega_body * dt / (1 rad)))
            //
            // [Quaternion Kinematics Boundary]: q uses dimensionless coordinates, so form
            // each angular increment as (omega * dt) / (1 rad) before extracting its scalar.
            constexpr Units::Quantity<T, Units::RadianUnit> one_rad(static_cast<T>(1));
            const T wx_dt_over_rad = ((candidate.angularVelocity.x * dt) / one_rad).value();
            const T wy_dt_over_rad = ((candidate.angularVelocity.y * dt) / one_rad).value();
            const T wz_dt_over_rad = ((candidate.angularVelocity.z * dt) / one_rad).value();

            const T qw = candidate.attitude.w;
            const T qx = candidate.attitude.x;
            const T qy = candidate.attitude.y;
            const T qz = candidate.attitude.z;

            const T half = static_cast<T>(0.5);

            // q_{k+1} = normalize(q_k + 0.5 * q_k ⊗ (omega * dt / 1 rad))
            const T dqw = half * (-qx * wx_dt_over_rad - qy * wy_dt_over_rad - qz * wz_dt_over_rad);
            const T dqx = half * ( qw * wx_dt_over_rad + qy * wz_dt_over_rad - qz * wy_dt_over_rad);
            const T dqy = half * ( qw * wy_dt_over_rad - qx * wz_dt_over_rad + qz * wx_dt_over_rad);
            const T dqz = half * ( qw * wz_dt_over_rad + qx * wy_dt_over_rad - qy * wx_dt_over_rad);

            const T new_w = qw + dqw;
            const T new_x = qx + dqx;
            const T new_y = qy + dqy;
            const T new_z = qz + dqz;

            auto new_att = Geometry::Quaternion<T, BodyFrame, RefFrame>::TryCreate(new_w, new_x, new_y, new_z);
            if (!new_att.has_value()) {
                return ResultType(new_att.error());
            }
            candidate.attitude = new_att.value();

            // 6. 最终全状态完整性校验 (位置、姿态、线速度、角速度全部有效)
            if (!IsStateFinite(candidate)) {
                return ResultType(Core::MathError::non_finite_input);
            }

            // 7. 事务原子提交：全步骤全部成功且所有候选分量严格有效方才变更外部状态
            state = candidate;
            return ResultType::success(true);
        }
    };

} // namespace vectoris::dynamics
