#include <gtest/gtest.h>
#include <cmath>
#include "AegisMath/Dynamics/EulerIntegrator.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"

struct WorldFrame {};
struct BodyFrame {};

using namespace AegisMath::Dynamics;
using namespace AegisMath::Units;

TEST(EulerDynamicsTest, TorqueFreeConservationAndConvergence) {
    using MI = MomentOfInertia;
    // 三主轴惯量互不相同的三轴非对称刚体
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(100.0), Position3<BodyFrame>{}, inertia);
    Wrench6<double, BodyFrame> zero_wrench;

    auto run_sim = [&](double dt_sec, int steps) {
        Position3<WorldFrame> pos;
        auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
        Velocity3<BodyFrame> vel;
        AngularVelocity3<BodyFrame> omega(AngularVelocity(1.0), AngularVelocity(0.5), AngularVelocity(0.2));

        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

        // 计算初始动能与角动量模长
        auto L0 = inertia.Multiply(state.angularVelocity);
        double L0_norm = std::sqrt(L0.x.value()*L0.x.value() + L0.y.value()*L0.y.value() + L0.z.value()*L0.z.value());
        double E0 = 0.5 * (10.0 * omega.x.value()*omega.x.value() +
                           20.0 * omega.y.value()*omega.y.value() +
                           30.0 * omega.z.value()*omega.z.value());

        Second dt(dt_sec);
        for (int i = 0; i < steps; ++i) {
            auto step_res = EulerIntegrator::Step(state, params, zero_wrench, dt);
            EXPECT_TRUE(step_res.has_value());
        }

        auto L1 = inertia.Multiply(state.angularVelocity);
        double L1_norm = std::sqrt(L1.x.value()*L1.x.value() + L1.y.value()*L1.y.value() + L1.z.value()*L1.z.value());
        double E1 = 0.5 * (10.0 * state.angularVelocity.x.value()*state.angularVelocity.x.value() +
                           20.0 * state.angularVelocity.y.value()*state.angularVelocity.y.value() +
                           30.0 * state.angularVelocity.z.value()*state.angularVelocity.z.value());

        return std::pair<double, double>{std::abs(E1 - E0), std::abs(L1_norm - L0_norm)};
    };

    // 仿真总时长 T = 0.1 s
    // dt = 0.01 s (10 步)
    auto [dE_coarse, dL_coarse] = run_sim(0.01, 10);
    // dt = 0.001 s (100 步)
    auto [dE_fine, dL_fine] = run_sim(0.001, 100);

    // 1. 短时间内物理量有界漂移
    EXPECT_LT(dE_coarse, 1e-3);
    EXPECT_LT(dL_coarse, 2e-3);
    EXPECT_LT(dE_fine, 1e-4);
    EXPECT_LT(dL_fine, 2e-4);

    // 2. 一阶欧拉收敛性验证：步长缩小 10 倍，漂移误差减小约 10 倍 (O(dt^1))
    EXPECT_LT(dE_fine, dE_coarse * 0.15);
    EXPECT_LT(dL_fine, dL_coarse * 0.15);
}

TEST(EulerDynamicsTest, IntegratorAbortsOnSingularInertiaWithoutCorruptingState) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> singular_inertia(
        MI(1.0), MI(1.0), MI::Zero(),
        MI(1.0), MI(1.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(1.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, singular_inertia);

    Position3<WorldFrame> pos(Meter(10.0), Meter(20.0), Meter(30.0));
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel(Velocity(1.0), Velocity(2.0), Velocity(3.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

    Torque3<BodyFrame> tau(Torque(5.0), Torque(5.0), Torque(5.0));
    Wrench6<double, BodyFrame> wrench(Force3<BodyFrame>{}, tau);

    // 积分器单步推进必须返回错误，且严禁修改 state
    auto step_res = EulerIntegrator::Step(state, params, wrench, Second(0.01));
    ASSERT_FALSE(step_res.has_value());
    EXPECT_EQ(step_res.error(), AegisMath::Core::MathError::singular_matrix);

    // 验证状态未受任何破坏 (未写入 NaN 或不当更新)
    EXPECT_DOUBLE_EQ(state.position.x.value(), 10.0);
    EXPECT_DOUBLE_EQ(state.position.y.value(), 20.0);
    EXPECT_DOUBLE_EQ(state.position.z.value(), 30.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 1.0);
    EXPECT_DOUBLE_EQ(state.angularVelocity.x.value(), 0.1);
}
