#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/EulerIntegrator.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Dynamics/Detail/StateTypes.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(RegressionFreeFall, VerticalDrop) {
    double m = 10.0;
    double g = 9.80665;

    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> inertia(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 
        0.0, 0.0, 1.0
    );

    AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame> params(
        m,
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0),
        inertia
    );

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(
        AegisMath::Geometry::Point3<double, WorldFrame>(0, 0, 0),
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1, 0, 0, 0).Value(),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0)
    );

    // 模拟重力向下 (Z轴向下为正，故 Fz = m * g)
    AegisMath::Dynamics::Wrench6<double, BodyFrame> gravity_wrench(
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, m * g),
        AegisMath::Geometry::Vector3<double, BodyFrame>(0, 0, 0)
    );

    double dt = 0.01;
    for (int i = 0; i < 100; ++i) {
        AegisMath::Dynamics::EulerIntegrator::Step(state, params, gravity_wrench, dt);
    }

    // 理论自由落体位移 z = 0.5 * g * t^2 (t = 1.0s)
    double expected_z = 0.5 * g * 1.0 * 1.0;
    EXPECT_NEAR(state.position.z, expected_z, 1e-2);
}