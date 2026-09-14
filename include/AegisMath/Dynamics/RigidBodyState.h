#pragma once

#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Dynamics/QuantityVector3.h"

namespace AegisMath::Dynamics {

    // 6DOF 刚体动力学微分方程封装 (Newton-Euler Equations, 强类型物理量)
    template <DynamicsScalar T, Geometry::FrameTag ReferenceFrame, Geometry::FrameTag BodyFrame>
    struct RigidBodyDynamicsKernel {
        
        // 计算状态导数 / 驱动 Newton-Euler 动力学更新
        // 平动: m * dv = F - omega x (m * v) ==> dv = F / m - omega x v
        // 转动: I * d_omega + omega x (I * omega) = Tau
        static constexpr void ComputeDerivative(
            const KinematicState<T, ReferenceFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Acceleration3<BodyFrame, T>& linearAccel,
            AngularAcceleration3<BodyFrame, T>& angularAccel
        ) noexcept {
            // 1. 平动加速度：a = F / m - omega x v
            Acceleration3<BodyFrame, T> w_cross_v = Cross(state.angularVelocity, state.linearVelocity);
            Acceleration3<BodyFrame, T> f_over_m = wrench.force / params.mass;
            linearAccel = f_over_m - w_cross_v;

            // 2. 转动加速度近似求解 (保持原有对角近似算法，待 HIGH-003 升级完整 3x3 求解)
            auto Iw = params.inertia.Multiply(state.angularVelocity);
            T w_cross_Iw_x = state.angularVelocity.y.value() * Iw.z.value() - state.angularVelocity.z.value() * Iw.y.value();
            T w_cross_Iw_y = state.angularVelocity.z.value() * Iw.x.value() - state.angularVelocity.x.value() * Iw.z.value();
            T w_cross_Iw_z = state.angularVelocity.x.value() * Iw.y.value() - state.angularVelocity.y.value() * Iw.x.value();

            using AngAccelQ = Units::Quantity<T, Units::RadianPerSecondSquaredUnit>;
            angularAccel.x = AngAccelQ((wrench.moment.x.value() - w_cross_Iw_x) / params.inertia.ixx.value());
            angularAccel.y = AngAccelQ((wrench.moment.y.value() - w_cross_Iw_y) / params.inertia.iyy.value());
            angularAccel.z = AngAccelQ((wrench.moment.z.value() - w_cross_Iw_z) / params.inertia.izz.value());
        }
    };

} // namespace AegisMath::Dynamics