#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/EulerIntegrator.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(PropagationTest, EulerStepVerification) {
    double m = 1000.0;
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> inertia(100,0,0, 0,100,0, 0,0,100);
    AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame> params(m, AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0), inertia);

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(
        AegisMath::Geometry::Point3<double, WorldFrame>(0,0,0),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1,0,0,0).Value(),
        AegisMath::Geometry::Vector3<double, BodyFrame>(10.0, 0, 0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0)
    );

    AegisMath::Dynamics::Wrench6<double, BodyFrame> zero_wrench(
        AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0)
    );

    // 步进 1.0 秒
    AegisMath::Dynamics::EulerIntegrator::Step(state, params, zero_wrench, 1.0);

    EXPECT_DOUBLE_EQ(state.position.x, 10.0); // 速度 10 * 1s = 10m 位移
}

TEST(PropagationTest, FrameTransformationPropagation) {
    double m = 1000.0;
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> inertia(100, 0, 0, 0, 100, 0, 0, 0, 100);
    AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame> params(
        m, AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0), inertia
    );

    // 90度绕Z轴旋转四元数：cos(pi/4) + k*sin(pi/4)
    // 将机体系 +X (前向) 旋转映射到参考系 +Y
    double s = 0.7071067811865475; // sqrt(0.5)
    auto att_result = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(s, 0.0, 0.0, s);
    ASSERT_TRUE(att_result.IsSuccess());

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(
        AegisMath::Geometry::Point3<double, WorldFrame>(0, 0, 0),
        att_result.Value(),
        AegisMath::Geometry::Vector3<double, BodyFrame>(10.0, 0.0, 0.0), // 机体前向速度 10 m/s
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0)
    );

    AegisMath::Dynamics::Wrench6<double, BodyFrame> zero_wrench(
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0)
    );

    // 步进 1.0 秒
    AegisMath::Dynamics::EulerIntegrator::Step(state, params, zero_wrench, 1.0);

    // 在参考系中，位移应当沿着 Y 轴 10m，而不是沿着 X 轴 10m
    EXPECT_NEAR(state.position.x, 0.0, 1e-12);
    EXPECT_NEAR(state.position.y, 10.0, 1e-12);
    EXPECT_NEAR(state.position.z, 0.0, 1e-12);
}

TEST(PropagationTest, AttitudeKinematicsPropagation) {
    double m = 1000.0;
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> inertia(100, 0, 0, 0, 100, 0, 0, 0, 100);
    AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame> params(
        m, AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0), inertia
    );

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(
        AegisMath::Geometry::Point3<double, WorldFrame>(0, 0, 0),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0.0, 0.0, 0.0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0.0, 0.0, 1.0) // 绕 Z 轴角速度 1.0 rad/s
    );

    AegisMath::Dynamics::Wrench6<double, BodyFrame> zero_wrench(
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0)
    );

    double dt = 0.1;
    AegisMath::Dynamics::EulerIntegrator::Step(state, params, zero_wrench, dt);

    // 一阶姿态推进后，四元数必须随角速度发生演化，不能停留在单位四元数
    EXPECT_GT(state.attitude.z, 0.04);
    EXPECT_LT(state.attitude.w, 1.0);
    double expected_z = (0.5 * 1.0 * dt) / std::sqrt(1.0 + std::pow(0.5 * 1.0 * dt, 2.0));
    EXPECT_NEAR(state.attitude.z, expected_z, 1e-6);
}