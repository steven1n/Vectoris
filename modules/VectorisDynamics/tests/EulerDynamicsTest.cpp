#include <gtest/gtest.h>
#include <cmath>
#include "Vectoris/Dynamics/EulerIntegrator.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Wrench6.h"

struct WorldFrame {};
struct BodyFrame {};

using namespace vectoris::dynamics;
using namespace vectoris::numerics::Units;

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
        auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
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
    template <typename T, vectoris::numerics::Geometry::FrameTag RefFrame, vectoris::numerics::Geometry::FrameTag BodyFrame>
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
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(0.5, 0.5, 0.5, 0.5).Value();
    Velocity3<BodyFrame> vel(Velocity(1.0), Velocity(2.0), Velocity(3.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

    Torque3<BodyFrame> tau(Torque(5.0), Torque(5.0), Torque(5.0));
    Wrench6<double, BodyFrame> wrench(Force3<BodyFrame>{}, tau);

    const auto before = state;

    // 积分器单步推进遭遇奇异惯量
    auto step_res = EulerIntegrator::Step(state, params, wrench, Second(0.01));
    ASSERT_FALSE(step_res.has_value());
    EXPECT_EQ(step_res.error(), vectoris::numerics::Core::MathError::singular_matrix);

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
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(0.5, 0.5, 0.5, 0.5).Value();
    Velocity3<BodyFrame> vel(Velocity(4.0), Velocity(5.0), Velocity(6.0));
    AngularVelocity3<BodyFrame> omega(AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Wrench6<double, BodyFrame> wrench;

    // 1. 负时间步拦截
    const auto before_neg = state;
    auto res_neg = EulerIntegrator::Step(state, params, wrench, Second(-0.01));
    ASSERT_FALSE(res_neg.has_value());
    EXPECT_EQ(res_neg.error(), vectoris::numerics::Core::MathError::invalid_argument);
    ExpectStateExactlyEqual(before_neg, state);

    // 2. 零时间步拦截
    const auto before_zero = state;
    auto res_zero = EulerIntegrator::Step(state, params, wrench, Second(0.0));
    ASSERT_FALSE(res_zero.has_value());
    EXPECT_EQ(res_zero.error(), vectoris::numerics::Core::MathError::invalid_argument);
    ExpectStateExactlyEqual(before_zero, state);

    // 3. 非有限时间步拦截
    const auto before_nan = state;
    auto res_nan = EulerIntegrator::Step(state, params, wrench, Second(std::numeric_limits<double>::quiet_NaN()));
    ASSERT_FALSE(res_nan.has_value());
    EXPECT_EQ(res_nan.error(), vectoris::numerics::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before_nan, state);
}

TEST(EulerDynamicsTest, DerivativeNonFiniteRollback) {
    Position3<WorldFrame> pos(Meter(1.0), Meter(2.0), Meter(3.0));
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
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
    EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before, state);
}

TEST(EulerDynamicsTest, AttitudeNormalizationFailureRollback) {
    Position3<WorldFrame> pos(Meter(1.0), Meter(2.0), Meter(3.0));
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
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
    EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
    ExpectStateExactlyEqual(before, state);
}

// ============================================================================
// VRT-07 Regressions: Physical Input Validation, Overflow & Transactional Rollback
// ============================================================================

namespace {
    template <typename T, vectoris::numerics::Geometry::FrameTag RefFrame, vectoris::numerics::Geometry::FrameTag BodyFrame>
    void ExpectStatePreserved(
        const KinematicState<T, RefFrame, BodyFrame>& before,
        const KinematicState<T, RefFrame, BodyFrame>& after
    ) {
        auto check_val = [](T b, T a) {
            if (std::isnan(b)) {
                EXPECT_TRUE(std::isnan(a));
            } else {
                EXPECT_EQ(b, a);
            }
        };

        check_val(before.position.x.value(), after.position.x.value());
        check_val(before.position.y.value(), after.position.y.value());
        check_val(before.position.z.value(), after.position.z.value());

        check_val(before.linearVelocity.x.value(), after.linearVelocity.x.value());
        check_val(before.linearVelocity.y.value(), after.linearVelocity.y.value());
        check_val(before.linearVelocity.z.value(), after.linearVelocity.z.value());

        check_val(before.angularVelocity.x.value(), after.angularVelocity.x.value());
        check_val(before.angularVelocity.y.value(), after.angularVelocity.y.value());
        check_val(before.angularVelocity.z.value(), after.angularVelocity.z.value());

        check_val(before.attitude.w, after.attitude.w);
        check_val(before.attitude.x, after.attitude.x);
        check_val(before.attitude.y, after.attitude.y);
        check_val(before.attitude.z, after.attitude.z);
    }
} // namespace

TEST(EulerDynamicsTest, VRT07_MassValidationAndTransactionalRollback) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );

    Position3<WorldFrame> pos{Meter(10.0), Meter(20.0), Meter(30.0)};
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel{Velocity(1.0), Velocity(2.0), Velocity(3.0)};
    AngularVelocity3<BodyFrame> omega{AngularVelocity(0.1), AngularVelocity(0.2), AngularVelocity(0.3)};

    auto base_state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Wrench6<double, BodyFrame> wrench{
        Force3<BodyFrame>{Force(100.0), Force::Zero(), Force::Zero()},
        Torque3<BodyFrame>{Torque::Zero(), Torque::Zero(), Torque::Zero()}
    };
    Second dt(0.01);

    // 1. Zero mass: rejected with invalid_argument
    {
        auto state = base_state;
        RigidBodyParameters<double, BodyFrame> params(Kilogram(0.0), Position3<BodyFrame>{}, inertia);
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::invalid_argument);
        ExpectStatePreserved(base_state, state);
    }

    // 2. Negative mass: rejected with invalid_argument
    {
        auto state = base_state;
        RigidBodyParameters<double, BodyFrame> params(Kilogram(-10.0), Position3<BodyFrame>{}, inertia);
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::invalid_argument);
        ExpectStatePreserved(base_state, state);
    }

    // 3. NaN mass: rejected with non_finite_input
    {
        auto state = base_state;
        RigidBodyParameters<double, BodyFrame> params(Kilogram(std::numeric_limits<double>::quiet_NaN()), Position3<BodyFrame>{}, inertia);
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 4. +Inf mass: rejected with non_finite_input
    {
        auto state = base_state;
        RigidBodyParameters<double, BodyFrame> params(Kilogram(std::numeric_limits<double>::infinity()), Position3<BodyFrame>{}, inertia);
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }
}

TEST(EulerDynamicsTest, VRT07_ForceValidationAndTransactionalRollback) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);

    Position3<WorldFrame> pos{Meter(5.0), Meter(6.0), Meter(7.0)};
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel{Velocity(1.0), Velocity(-1.0), Velocity(2.0)};
    AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};

    auto base_state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Second dt(0.01);

    double qnan = std::numeric_limits<double>::quiet_NaN();
    double pinf = std::numeric_limits<double>::infinity();
    double ninf = -std::numeric_limits<double>::infinity();

    // 1. NaN force
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{Force(qnan), Force(0.0), Force(0.0)},
            Torque3<BodyFrame>{}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 2. +Inf force
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{Force(0.0), Force(pinf), Force(0.0)},
            Torque3<BodyFrame>{}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 3. -Inf force
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{Force(0.0), Force(0.0), Force(ninf)},
            Torque3<BodyFrame>{}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 4. NaN moment
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{},
            Torque3<BodyFrame>{Torque(0.0), Torque(qnan), Torque(0.0)}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 5. +Inf moment
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{},
            Torque3<BodyFrame>{Torque(pinf), Torque(0.0), Torque(0.0)}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }

    // 6. -Inf moment
    {
        auto state = base_state;
        Wrench6<double, BodyFrame> wrench{
            Force3<BodyFrame>{},
            Torque3<BodyFrame>{Torque(0.0), Torque(0.0), Torque(ninf)}
        };
        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(base_state, state);
    }
}

TEST(EulerDynamicsTest, VRT07_InitialStateValidationAndTransactionalRollback) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);
    Wrench6<double, BodyFrame> wrench;
    Second dt(0.01);

    double qnan = std::numeric_limits<double>::quiet_NaN();
    double pinf = std::numeric_limits<double>::infinity();

    auto valid_att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();

    // 1. Non-finite initial position (NaN)
    {
        Position3<WorldFrame> pos{Meter(qnan), Meter(0.0), Meter(0.0)};
        Velocity3<BodyFrame> vel{Velocity(1.0), Velocity(0.0), Velocity(0.0)};
        AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, valid_att, vel, omega);
        const auto before = state;

        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(before, state);
    }

    // 2. Non-finite initial velocity (+Inf)
    {
        Position3<WorldFrame> pos{Meter(0.0), Meter(0.0), Meter(0.0)};
        Velocity3<BodyFrame> vel{Velocity(0.0), Velocity(pinf), Velocity(0.0)};
        AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, valid_att, vel, omega);
        const auto before = state;

        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(before, state);
    }

    // 3. Non-finite initial angular velocity (-Inf)
    {
        Position3<WorldFrame> pos{Meter(0.0), Meter(0.0), Meter(0.0)};
        Velocity3<BodyFrame> vel{Velocity(1.0), Velocity(0.0), Velocity(0.0)};
        AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(-pinf)};
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, valid_att, vel, omega);
        const auto before = state;

        auto res = EulerIntegrator::Step(state, params, wrench, dt);
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(before, state);
    }
}

TEST(EulerDynamicsTest, VRT07_CandidateOverflowValidationAndTransactionalRollback) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    double huge = 1e308;

    // 1. Huge position + huge velocity causing candidate position overflow to +Inf
    {
        Position3<WorldFrame> pos{Meter(huge), Meter(0.0), Meter(0.0)};
        Velocity3<BodyFrame> vel{Velocity(huge), Velocity(0.0), Velocity(0.0)};
        AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
        const auto before = state;

        Wrench6<double, BodyFrame> zero_wrench;
        auto res = EulerIntegrator::Step(state, params, zero_wrench, Second(10.0));
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(before, state);
    }

    // 2. Huge velocity + huge force causing candidate velocity overflow
    {
        Position3<WorldFrame> pos{Meter(0.0), Meter(0.0), Meter(0.0)};
        Velocity3<BodyFrame> vel{Velocity(huge), Velocity(0.0), Velocity(0.0)};
        AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
        const auto before = state;

        Wrench6<double, BodyFrame> huge_force_wrench{
            Force3<BodyFrame>{Force(huge), Force(0.0), Force(0.0)},
            Torque3<BodyFrame>{}
        };
        auto res = EulerIntegrator::Step(state, params, huge_force_wrench, Second(10.0));
        ASSERT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), vectoris::numerics::Core::MathError::non_finite_input);
        ExpectStatePreserved(before, state);
    }
}

TEST(EulerDynamicsTest, VRT07_OrdinaryTranslationIndependentOracle) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    // m = 10.0 kg
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);

    // Initial state: pos = [10.0, 20.0, 30.0], vel = [2.0, 0.0, 0.0], attitude = Identity, omega = [0, 0, 0]
    Position3<WorldFrame> pos{Meter(10.0), Meter(20.0), Meter(30.0)};
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel{Velocity(2.0), Velocity(0.0), Velocity(0.0)};
    AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

    // Applied constant force: Fx = 100.0 N, Fy = 0, Fz = 0
    Wrench6<double, BodyFrame> wrench{
        Force3<BodyFrame>{Force(100.0), Force(0.0), Force(0.0)},
        Torque3<BodyFrame>{Torque(0.0), Torque(0.0), Torque(0.0)}
    };

    // dt = 0.1 s
    Second dt(0.1);

    auto res = EulerIntegrator::Step(state, params, wrench, dt);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res.value());

    // Analytic hand-computed independent oracle:
    // a = F / m = 100.0 / 10.0 = 10.0 m/s^2
    // v_next = v_0 + a * dt = 2.0 + 10.0 * 0.1 = 3.0 m/s
    // x_next = x_0 + v_next * dt = 10.0 + 3.0 * 0.1 = 10.3 m
    // y_next = 20.0 m, z_next = 30.0 m
    // vy_next = 0.0, vz_next = 0.0
    // attitude remains [1, 0, 0, 0], omega remains [0, 0, 0]
    EXPECT_DOUBLE_EQ(state.position.x.value(), 10.3);
    EXPECT_DOUBLE_EQ(state.position.y.value(), 20.0);
    EXPECT_DOUBLE_EQ(state.position.z.value(), 30.0);

    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 3.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.y.value(), 0.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.z.value(), 0.0);

    EXPECT_DOUBLE_EQ(state.attitude.w, 1.0);
    EXPECT_DOUBLE_EQ(state.attitude.x, 0.0);
    EXPECT_DOUBLE_EQ(state.attitude.y, 0.0);
    EXPECT_DOUBLE_EQ(state.attitude.z, 0.0);

    EXPECT_DOUBLE_EQ(state.angularVelocity.x.value(), 0.0);
    EXPECT_DOUBLE_EQ(state.angularVelocity.y.value(), 0.0);
    EXPECT_DOUBLE_EQ(state.angularVelocity.z.value(), 0.0);
}

TEST(EulerDynamicsTest, VRT07_ZeroForceAndConstantVelocity) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(5.0), Position3<BodyFrame>{}, inertia);

    // Initial state: pos = [1.0, 2.0, 3.0], vel = [5.0, -2.0, 1.0], attitude = Identity, omega = [0, 0, 0]
    Position3<WorldFrame> pos{Meter(1.0), Meter(2.0), Meter(3.0)};
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel{Velocity(5.0), Velocity(-2.0), Velocity(1.0)};
    AngularVelocity3<BodyFrame> omega{AngularVelocity(0.0), AngularVelocity(0.0), AngularVelocity(0.0)};

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);
    Wrench6<double, BodyFrame> zero_wrench;
    Second dt(0.2);

    auto res = EulerIntegrator::Step(state, params, zero_wrench, dt);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res.value());

    // Independent analytical calculation:
    // a = 0 m/s^2
    // v_next = [5.0, -2.0, 1.0] m/s
    // x_next = 1.0 + 5.0 * 0.2 = 2.0 m
    // y_next = 2.0 + (-2.0) * 0.2 = 1.6 m
    // z_next = 3.0 + 1.0 * 0.2 = 3.2 m
    EXPECT_DOUBLE_EQ(state.position.x.value(), 2.0);
    EXPECT_DOUBLE_EQ(state.position.y.value(), 1.6);
    EXPECT_DOUBLE_EQ(state.position.z.value(), 3.2);

    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 5.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.y.value(), -2.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.z.value(), 1.0);
}
