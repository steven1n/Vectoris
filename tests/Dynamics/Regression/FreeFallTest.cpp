#include <gtest/gtest.h>
#include <cmath>
#include "AegisMath/Dynamics/EulerIntegrator.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/QuantityVector3.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(RegressionFreeFall, VerticalDrop) {
    using namespace AegisMath::Dynamics;
    using namespace AegisMath::Units;

    Kilogram m{10.0};
    double g = 9.80665;

    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(1.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(1.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(1.0)
    );

    RigidBodyParameters<double, BodyFrame> params(
        m,
        Position3<BodyFrame>(),
        inertia
    );

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>(),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1, 0, 0, 0).Value(),
        Velocity3<BodyFrame>(),
        AngularVelocity3<BodyFrame>()
    );

    // 模拟重力向下 (Z轴向下为正，故 Fz = m * g)
    Wrench6<double, BodyFrame> gravity_wrench(
        Force3<BodyFrame>(Force::Zero(), Force::Zero(), Force(m.value() * g)),
        Torque3<BodyFrame>()
    );

    double dt_val = 0.01;
    Second dt{dt_val};
    int steps = 100;
    for (int i = 0; i < steps; ++i) {
        EulerIntegrator::Step(state, params, gravity_wrench, dt);
    }

    // 半隐式欧拉精确离散递推真值:
    // v_k = v_{k-1} + g * dt = k * g * dt
    // z_k = z_{k-1} + v_k * dt = z_{k-1} + k * g * dt^2
    // z_N = z_0 + N * v_0 * dt + g * dt^2 * (N * (N + 1) / 2)
    // 对于 z_0 = 0, v_0 = 0:
    // z_N = 0.5 * g * t^2 + 0.5 * g * t * dt
    double expected_discrete_z = g * (dt_val * dt_val) * (static_cast<double>(steps * (steps + 1)) / 2.0);
    EXPECT_NEAR(state.position.z.value(), expected_discrete_z, 1e-9);
}

TEST(RegressionFreeFall, FirstOrderConvergence) {
    using namespace AegisMath::Dynamics;
    using namespace AegisMath::Units;

    Kilogram m{10.0};
    double g = 9.80665;
    double t_total = 1.0;
    double z_continuous_analytical = 0.5 * g * t_total * t_total;

    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(1.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(1.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(1.0)
    );

    RigidBodyParameters<double, BodyFrame> params(
        m,
        Position3<BodyFrame>(),
        inertia
    );

    auto simulate = [&](double dt_val, int steps) {
        auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
            Position3<WorldFrame>(),
            AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1, 0, 0, 0).Value(),
            Velocity3<BodyFrame>(),
            AngularVelocity3<BodyFrame>()
        );

        Wrench6<double, BodyFrame> gravity_wrench(
            Force3<BodyFrame>(Force::Zero(), Force::Zero(), Force(m.value() * g)),
            Torque3<BodyFrame>()
        );

        Second dt{dt_val};
        for (int i = 0; i < steps; ++i) {
            EulerIntegrator::Step(state, params, gravity_wrench, dt);
        }
        return state.position.z.value();
    };

    // 分别以 dt, dt/2, dt/4 进行仿真，验证误差阶数为 O(dt)
    double dt1 = 0.04;
    double dt2 = 0.02;
    double dt3 = 0.01;

    double z1 = simulate(dt1, 25);
    double z2 = simulate(dt2, 50);
    double z3 = simulate(dt3, 100);

    double e1 = std::abs(z1 - z_continuous_analytical);
    double e2 = std::abs(z2 - z_continuous_analytical);
    double e3 = std::abs(z3 - z_continuous_analytical);

    double ratio1 = e1 / e2;
    double ratio2 = e2 / e3;

    // 半隐式欧拉是一阶收敛方法，步长减半误差应减半 (ratio ≈ 2.0)
    EXPECT_GT(ratio1, 1.8);
    EXPECT_LT(ratio1, 2.2);
    EXPECT_NEAR(ratio1, 2.0, 0.05);

    EXPECT_GT(ratio2, 1.8);
    EXPECT_LT(ratio2, 2.2);
    EXPECT_NEAR(ratio2, 2.0, 0.05);
}