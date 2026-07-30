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