#include <gtest/gtest.h>
#include <cmath>
#include "AegisDynamics/EulerIntegrator.h"
#include "AegisDynamics/RigidBodyParameters.h"
#include "AegisDynamics/Wrench6.h"
#include "AegisDynamics/QuantityVector3.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(PropagationTest, EulerStepVerification) {
    using namespace AegisDynamics;
    using namespace AegisMath::Units;

    Kilogram m{1000.0};
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(100.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(100.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(100.0)
    );
    RigidBodyParameters<double, BodyFrame> params(m, Position3<BodyFrame>(), inertia);

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>(),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        Velocity3<BodyFrame>(Velocity(10.0), Velocity::Zero(), Velocity::Zero()),
        AngularVelocity3<BodyFrame>()
    );

    Force3<BodyFrame> zero_force;
    Torque3<BodyFrame> zero_moment;
    Wrench6<double, BodyFrame> zero_wrench(zero_force, zero_moment);

    // 步进 1.0 秒
    EulerIntegrator::Step(state, params, zero_wrench, Second(1.0));

    EXPECT_DOUBLE_EQ(state.position.x.value(), 10.0); // 速度 10 * 1s = 10m 位移
}

TEST(PropagationTest, FrameTransformationPropagation) {
    using namespace AegisDynamics;
    using namespace AegisMath::Units;

    Kilogram m{1000.0};
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(100.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(100.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(100.0)
    );
    RigidBodyParameters<double, BodyFrame> params(m, Position3<BodyFrame>(), inertia);

    // 90度绕Z轴旋转四元数：cos(pi/4) + k*sin(pi/4)
    // 将机体系 +X (前向) 旋转映射到参考系 +Y
    double s = 0.7071067811865475; // sqrt(0.5)
    auto att_result = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(s, 0.0, 0.0, s);
    ASSERT_TRUE(att_result.IsSuccess());

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>(),
        att_result.Value(),
        Velocity3<BodyFrame>(Velocity(10.0), Velocity::Zero(), Velocity::Zero()), // 机体前向速度 10 m/s
        AngularVelocity3<BodyFrame>()
    );

    Force3<BodyFrame> zero_force;
    Torque3<BodyFrame> zero_moment;
    Wrench6<double, BodyFrame> zero_wrench(zero_force, zero_moment);

    // 步进 1.0 秒
    EulerIntegrator::Step(state, params, zero_wrench, Second(1.0));

    // 在参考系中，位移应当沿着 Y 轴 10m，而不是沿着 X 轴 10m
    EXPECT_NEAR(state.position.x.value(), 0.0, 1e-12);
    EXPECT_NEAR(state.position.y.value(), 10.0, 1e-12);
    EXPECT_NEAR(state.position.z.value(), 0.0, 1e-12);
}

TEST(PropagationTest, AttitudeKinematicsPropagation) {
    using namespace AegisDynamics;
    using namespace AegisMath::Units;

    Kilogram m{1000.0};
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(100.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(100.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(100.0)
    );
    RigidBodyParameters<double, BodyFrame> params(m, Position3<BodyFrame>(), inertia);

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>(),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        Velocity3<BodyFrame>(),
        AngularVelocity3<BodyFrame>(AngularVelocity::Zero(), AngularVelocity::Zero(), AngularVelocity(1.0)) // 绕 Z 轴角速度 1.0 rad/s
    );

    Force3<BodyFrame> zero_force;
    Torque3<BodyFrame> zero_moment;
    Wrench6<double, BodyFrame> zero_wrench(zero_force, zero_moment);

    Second dt{0.1};
    EulerIntegrator::Step(state, params, zero_wrench, dt);

    // 一阶姿态推进后，四元数必须随角速度发生演化，不能停留在单位四元数
    EXPECT_GT(state.attitude.z, 0.04);
    EXPECT_LT(state.attitude.w, 1.0);
    double expected_z = (0.5 * 1.0 * dt.value()) / std::sqrt(1.0 + std::pow(0.5 * 1.0 * dt.value(), 2.0));
    EXPECT_NEAR(state.attitude.z, expected_z, 1e-6);
}