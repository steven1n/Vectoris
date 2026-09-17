#include <gtest/gtest.h>
#include <cmath>
#include "AegisDynamics/EulerIntegrator.h"
#include "AegisDynamics/RigidBodyParameters.h"
#include "AegisDynamics/Wrench6.h"

struct WorldFrame {};
struct BodyFrame {};

using namespace AegisDynamics;
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

namespace {
    template <typename T, AegisMath::Geometry::FrameTag RefFrame, AegisMath::Geometry::FrameTag BodyFrame>
    void ExpectStateExactlyEqual(
        const KinematicState<T, RefFrame, BodyFrame>& before,
        const KinematicState<T, RefFrame, BodyFrame>& after
    ) {
        EXPECT_EQ(before.position.x.value(), after.position.x.value());
        EXPECT_EQ(before.position.y.value(), after.position.y.value());
        EXPECT_EQ(before.position.z.value(), after.position.z.value());

        EXPECT_EQ(before.linearVelocity.x.value(), after.linearVelocity.x.value());
        EXPECT_EQ(before.linearVelocity.y.value(), after.linearVelocity.y.value());
        EXPECT_EQ(before.linearVelocity.z.value(), after.linearVelocity.z.value());

        EXPECT_EQ(before.angularVelocity.x.value(), after.angularVelocity.x.value());
        EXPECT_EQ(before.angularVelocity.y.value(), after.angularVelocity.y.value());
        EXPECT_EQ(before.angularVelocity.z.value(), after.angularVelocity.z.value());

        EXPECT_EQ(before.attitude.w, after.attitude.w);
        EXPECT_EQ(before.attitude.x, after.attitude.x);
        EXPECT_EQ(before.attitude.y, after.attitude.y);
        EXPECT_EQ(before.attitude.z, after.attitude.z);
    }
} // namespace

TEST(EulerDynamicsTest, TransactionalSafetyOnSingularInertia) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> singular_inertia(
        MI(1.0), MI(1.0), MI::Zero(),
        MI(1.0), MI(1.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(1.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, singular_inertia);

    Position3<WorldFrame> pos(Meter(10.0), Meter(20.0), Meter(30.0));
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(0.5, 0.5, 0.5, 0.5).Value();
    Velocity3<BodyFrame> vel(Velocity(1.0), Velocity(2.0), Velocity(3.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

    Torque3<BodyFrame> tau(Torque(5.0), Torque(5.0), Torque(5.0));
    Wrench6<double, BodyFrame> wrench(Force3<BodyFrame>{}, tau);

    const auto before = state;

    // 积分器单步推进遭遇奇异惯量
    auto step_res = EulerIntegrator::Step(state, params, wrench, Second(0.01));
    ASSERT_FALSE(step_res.has_value());
    EXPECT_EQ(step_res.error(), AegisMath::Core::MathError::singular_matrix);

    // 验证状态未受任何破坏 (原子事务回滚保障，全字段逐项严格恒等)
    ExpectStateExactlyEqual(before, state);
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
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(0.5, 0.5, 0.5, 0.5).Value();
    Velocity3<BodyFrame> vel(Velocity(4.0), Velocity(5.0), Velocity(6.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Wrench6<double, BodyFrame> wrench;

    // 1. 负时间步拦截
    const auto before_neg = state;
    auto res_neg = EulerIntegrator::Step(state, params, wrench, Second(-0.01));
    ASSERT_FALSE(res_neg.has_value());
    EXPECT_EQ(res_neg.error(), AegisMath::Core::MathError::invalid_argument);
    ExpectStateExactlyEqual(before_neg, state);

    // 2. 零时间步拦截
    const auto before_zero = state;
    auto res_zero = EulerIntegrator::Step(state, params, wrench, Second(0.0));
    ASSERT_FALSE(res_zero.has_value());
    EXPECT_EQ(res_zero.error(), AegisMath::Core::MathError::invalid_argument);
    ExpectStateExactlyEqual(before_zero, state);

    // 3. 非有限时间步拦截
    const auto before_nan = state;
    auto res_nan = EulerIntegrator::Step(state, params, wrench, Second(std::numeric_limits<double>::quiet_NaN()));
    ASSERT_FALSE(res_nan.has_value());
    EXPECT_EQ(res_nan.error(), AegisMath::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before_nan, state);
}

TEST(EulerDynamicsTest, DerivativeNonFiniteRollback) {
    Position3<WorldFrame> pos(Meter(1.0), Meter(2.0), Meter(3.0));
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel(Velocity(4.0), Velocity(5.0), Velocity(6.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    const auto before = state;

    InertiaTensor3<double, BodyFrame> inertia(
        MomentOfInertia(10.0), MomentOfInertia::Zero(), MomentOfInertia::Zero(),
        MomentOfInertia::Zero(), MomentOfInertia(20.0), MomentOfInertia::Zero(),
        MomentOfInertia::Zero(), MomentOfInertia::Zero(), MomentOfInertia(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Mass(5.0), Position3<BodyFrame>{}, inertia);

    Wrench6<double, BodyFrame> nan_wrench{
        Force3<BodyFrame>{Force::Zero(), Force::Zero(), Force::Zero()},
        Torque3<BodyFrame>{Torque(std::numeric_limits<double>::quiet_NaN()), Torque::Zero(), Torque::Zero()}
    };

    auto res = EulerIntegrator::Step(state, params, nan_wrench, Second(0.01));
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), AegisMath::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before, state);
}

TEST(EulerDynamicsTest, AttitudeNormalizationFailureRollback) {
    Position3<WorldFrame> pos(Meter(1.0), Meter(2.0), Meter(3.0));
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel(Velocity(4.0), Velocity(5.0), Velocity(6.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    const auto before = state;

    InertiaTensor3<double, BodyFrame> inertia(
        MomentOfInertia(1.0), MomentOfInertia::Zero(), MomentOfInertia::Zero(),
        MomentOfInertia::Zero(), MomentOfInertia(1.0), MomentOfInertia::Zero(),
        MomentOfInertia::Zero(), MomentOfInertia::Zero(), MomentOfInertia(1.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Mass(1.0), Position3<BodyFrame>{}, inertia);

    Wrench6<double, BodyFrame> huge_wrench{
        Force3<BodyFrame>{Force::Zero(), Force::Zero(), Force::Zero()},
        Torque3<BodyFrame>{Torque(1e300), Torque::Zero(), Torque::Zero()}
    };

    auto res = EulerIntegrator::Step(state, params, huge_wrench, Second(1e10));
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), AegisMath::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before, state);
}
