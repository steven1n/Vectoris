#pragma once

#include "Vectoris/Numerics/Core/Result.h"
#include "Vectoris/Numerics/Core/MathError.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/QuantityVector3.h"

namespace vectoris::dynamics {

    // 刚体导数结构体 (支持纯函数式与零堆分配)
    template <DynamicsScalar T, Geometry::FrameTag BodyFrame>
    struct DynamicsDerivative final {
        Acceleration3<BodyFrame, T> linear;
        AngularAcceleration3<BodyFrame, T> angular;
    };

    // 6DOF 刚体动力学微分方程封装 (Newton-Euler Equations, 强类型物理量)
    template <DynamicsScalar T, Geometry::FrameTag ReferenceFrame, Geometry::FrameTag BodyFrame>
    struct RigidBodyDynamicsKernel {

        // 计算状态导数 / 驱动 Newton-Euler 动力学更新 (函数式强类型接口)
        // 平动: m * dv = F - (omega x (m * v)) / (1 rad)
        //       ==> dv = F / m - (omega x v) / (1 rad)
        // 转动: I * alpha + LieBracket(omega, I * omega) = tau
        //       LieBracket(omega, L) = (omega x L) / (1 rad)
        static constexpr Core::Result<DynamicsDerivative<T, BodyFrame>, Core::MathError>
        ComputeDerivative(
            const KinematicState<T, ReferenceFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench
        ) noexcept {
            using ResType = Core::Result<DynamicsDerivative<T, BodyFrame>, Core::MathError>;

            // 校验质量参数 (必须有限且严格正定)
            if (!Traits::IsFinite(params.mass.value())) {
                return ResType(Core::MathError::non_finite_input);
            }
            if (params.mass.value() <= static_cast<T>(0)) {
                return ResType(Core::MathError::invalid_argument);
            }

            // 校验外力和力矩输入有限性
            if (!Traits::IsFinite(wrench.force.x.value()) ||
                !Traits::IsFinite(wrench.force.y.value()) ||
                !Traits::IsFinite(wrench.force.z.value()) ||
                !Traits::IsFinite(wrench.moment.x.value()) ||
                !Traits::IsFinite(wrench.moment.y.value()) ||
                !Traits::IsFinite(wrench.moment.z.value())) {
                return ResType(Core::MathError::non_finite_input);
            }

            // 校验当前机体速度输入有限性
            if (!Traits::IsFinite(state.linearVelocity.x.value()) ||
                !Traits::IsFinite(state.linearVelocity.y.value()) ||
                !Traits::IsFinite(state.linearVelocity.z.value()) ||
                !Traits::IsFinite(state.angularVelocity.x.value()) ||
                !Traits::IsFinite(state.angularVelocity.y.value()) ||
                !Traits::IsFinite(state.angularVelocity.z.value())) {
                return ResType(Core::MathError::non_finite_input);
            }

            // 1. 平动加速度：a = F / m - (omega x v) / (1 rad)
            Acceleration3<BodyFrame, T> w_cross_v = RotationalCross(state.angularVelocity, state.linearVelocity);
            Acceleration3<BodyFrame, T> f_over_m = wrench.force / params.mass;
            Acceleration3<BodyFrame, T> linearAccel = f_over_m - w_cross_v;

            // 校验计算出的平动加速度有限性 (防止数值溢出)
            if (!Traits::IsFinite(linearAccel.x.value()) ||
                !Traits::IsFinite(linearAccel.y.value()) ||
                !Traits::IsFinite(linearAccel.z.value())) {
                return ResType(Core::MathError::non_finite_input);
            }

            // 2. 完整耦合旋转动力学方程 (Remediating AML-HIGH-003):
            // L = I * omega
            // tau_gyro = LieBracket(omega, L)
            // tau_net = tau_ext - tau_gyro
            // alpha = SolveSPD(I, tau_net)
            // 所有物理量全程强类型绑定，杜绝对角近似与裸标量绕过
            auto Iw = params.inertia.Multiply(state.angularVelocity);
            auto gyro = LieBracket(state.angularVelocity, Iw);
            auto tau_net = wrench.moment - gyro;

            auto solve_res = SolveSPD(params.inertia, tau_net);
            if (!solve_res.has_value()) {
                return Core::Result<DynamicsDerivative<T, BodyFrame>, Core::MathError>(solve_res.error());
            }

            return Core::Result<DynamicsDerivative<T, BodyFrame>, Core::MathError>::success(
                DynamicsDerivative<T, BodyFrame>{linearAccel, solve_res.value()}
            );
        }

        // 计算状态导数 (出参重载，保证现有调用代码完全向后兼容)
        static constexpr Core::Result<AngularAcceleration3<BodyFrame, T>, Core::MathError>
        ComputeDerivative(
            const KinematicState<T, ReferenceFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Acceleration3<BodyFrame, T>& linearAccel,
            AngularAcceleration3<BodyFrame, T>& angularAccel
        ) noexcept {
            auto res = ComputeDerivative(state, params, wrench);
            if (!res.has_value()) {
                return Core::Result<AngularAcceleration3<BodyFrame, T>, Core::MathError>(res.error());
            }
            linearAccel = res.value().linear;
            angularAccel = res.value().angular;
            return Core::Result<AngularAcceleration3<BodyFrame, T>, Core::MathError>::success(angularAccel);
        }
    };

} // namespace vectoris::dynamics
