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

        // 强类型代数计算初始物理量: E = 0.5 * w · (I*w), L_sq = L · L
        auto L0 = inertia.Multiply(state.angularVelocity);
        auto L0_sq = Dot(L0, L0);
        auto E0 = 0.5 * Dot(state.angularVelocity, L0);

        Second dt(dt_sec);
        for (int i = 0; i < steps; ++i) {
            auto step_res = EulerIntegrator::Step(state, params, zero_wrench, dt);
            EXPECT_TRUE(step_res.has_value());
        }

        auto L1 = inertia.Multiply(state.angularVelocity);
        auto L1_sq = Dot(L1, L1);
        auto E1 = 0.5 * Dot(state.angularVelocity, L1);

        double dE = std::abs((E1 - E0).value());
        double dL_sq = std::abs((L1_sq - L0_sq).value());

        return std::pair<double, double>{dE, dL_sq};
    };

    // 仿真总时长 T = 0.1 s
    // dt = 0.01 s (10 步)
    auto [dE_coarse, dL_sq_coarse] = run_sim(0.01, 10);
    // dt = 0.001 s (100 步)
    auto [dE_fine, dL_sq_fine] = run_sim(0.001, 100);

    // 1. 短时间内物理量有界漂移
    EXPECT_LT(dE_coarse, 1e-3);
    EXPECT_LT(dL_sq_coarse, 5e-2);
    EXPECT_LT(dE_fine, 1e-4);
    EXPECT_LT(dL_sq_fine, 5e-3);

    // 2. 一阶欧拉收敛性验证：步长缩小 10 倍，漂移误差减小约 10 倍 (O(dt^1))
    EXPECT_LT(dE_fine, dE_coarse * 0.15);
    EXPECT_LT(dL_sq_fine, dL_sq_coarse * 0.15);
}

TEST(EulerDynamicsTest, TransactionalSafetyOnSingularInertia) {
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

    // 积分器单步推进遭遇奇异惯量
    auto step_res = EulerIntegrator::Step(state, params, wrench, Second(0.01));
    ASSERT_FALSE(step_res.has_value());
    EXPECT_EQ(step_res.error(), AegisMath::Core::MathError::singular_matrix);

    // 验证状态未受任何破坏 (原子事务回滚保障)
    EXPECT_DOUBLE_EQ(state.position.x.value(), 10.0);
    EXPECT_DOUBLE_EQ(state.position.y.value(), 20.0);
    EXPECT_DOUBLE_EQ(state.position.z.value(), 30.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 1.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.y.value(), 2.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.z.value(), 3.0);
    EXPECT_DOUBLE_EQ(state.angularVelocity.x.value(), 0.1);
    EXPECT_DOUBLE_EQ(state.angularVelocity.y.value(), 0.2);
    EXPECT_DOUBLE_EQ(state.angularVelocity.z.value(), 0.3);
    EXPECT_DOUBLE_EQ(state.attitude.w, 1.0);
}

TEST(EulerDynamicsTest, TimestepValidationAndTransactionalSafety) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);

    Position3<WorldFrame> pos(Meter(1.0), Meter(2.0), Meter(3.0));
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel(Velocity(4.0), Velocity(5.0), Velocity(6.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Wrench6<double, BodyFrame> wrench;

    // 1. 负时间步拦截
    auto res_neg = EulerIntegrator::Step(state, params, wrench, Second(-0.01));
    ASSERT_FALSE(res_neg.has_value());
    EXPECT_EQ(res_neg.error(), AegisMath::Core::MathError::invalid_argument);

    // 2. 零时间步拦截
    auto res_zero = EulerIntegrator::Step(state, params, wrench, Second(0.0));
    ASSERT_FALSE(res_zero.has_value());
    EXPECT_EQ(res_zero.error(), AegisMath::Core::MathError::invalid_argument);

    // 3. 非有限时间步拦截
    auto res_nan = EulerIntegrator::Step(state, params, wrench, Second(std::numeric_limits<double>::quiet_NaN()));
    ASSERT_FALSE(res_nan.has_value());
    EXPECT_EQ(res_nan.error(), AegisMath::Core::MathError::non_finite_input);

    // 验证状态保持完全一致未被修改
    EXPECT_DOUBLE_EQ(state.position.x.value(), 1.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 4.0);
    EXPECT_DOUBLE_EQ(state.angularVelocity.x.value(), 0.1);
    EXPECT_DOUBLE_EQ(state.attitude.w, 1.0);
}
