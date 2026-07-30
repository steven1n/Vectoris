#pragma once

#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/Twist6.h"

namespace AegisMath::Dynamics {

    // 6DOF 刚体动力学微分方程封装 (Newton-Euler Equations)
    template <DynamicsScalar T, Geometry::FrameTag ReferenceFrame, Geometry::FrameTag BodyFrame>
    struct RigidBodyDynamicsKernel {
        
        // 计算状态导数 / 驱动 Newton-Euler 动力学更新
        // 平动: m * dv = F - omega x (m * v)
        // 转动: I * d_omega + omega x (I * omega) = Tau
        static constexpr void ComputeDerivative(
            const KinematicState<T, ReferenceFrame, BodyFrame>& state,
            const RigidBodyParameters<T, BodyFrame>& params,
            const Wrench6<T, BodyFrame>& wrench,
            Geometry::Vector3<T, BodyFrame>& linearAccel,
            Geometry::Vector3<T, BodyFrame>& angularAccel
        ) noexcept {
            // 简化的牛顿欧拉核心映射（供仿真引擎调用）
            // m * a = F - w x (m * v)
            T inv_m = static_cast<T>(1.0) / params.mass;
            
            // 科氏力交叉项跨乘
            T cross_x = state.angularVelocity.y * state.linearVelocity.z - state.angularVelocity.z * state.linearVelocity.y;
            T cross_y = state.angularVelocity.z * state.linearVelocity.x - state.angularVelocity.x * state.linearVelocity.z;
            T cross_z = state.angularVelocity.x * state.linearVelocity.y - state.angularVelocity.y * state.linearVelocity.x;

            linearAccel.x = (wrench.force.x - params.mass * cross_x) * inv_m;
            linearAccel.y = (wrench.force.y - params.mass * cross_y) * inv_m;
            linearAccel.z = (wrench.force.z - params.mass * cross_z) * inv_m;

            // 转动惯量欧拉动力学近似：I * alpha = Tau - w x (I * w)
            auto Iw = params.inertia.Multiply(state.angularVelocity);
            T w_cross_Iw_x = state.angularVelocity.y * Iw.z - state.angularVelocity.z * Iw.y;
            T w_cross_Iw_y = state.angularVelocity.z * Iw.x - state.angularVelocity.x * Iw.z;
            T w_cross_Iw_z = state.angularVelocity.x * Iw.y - state.angularVelocity.y * Iw.x;

            // 简化输出
            angularAccel.x = (wrench.moment.x - w_cross_Iw_x) / params.inertia.ixx;
            angularAccel.y = (wrench.moment.y - w_cross_Iw_y) / params.inertia.iyy;
            angularAccel.z = (wrench.moment.z - w_cross_Iw_z) / params.inertia.izz;
        }
    };

} // namespace AegisMath::Dynamics