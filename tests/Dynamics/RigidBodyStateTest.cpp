#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/RigidBodyState.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(RigidBodyStateTest, NewtonEulerAcceleration) {
    double m = 1000.0;
    AegisMath::Geometry::Vector3<double, BodyFrame> com(0,0,0);
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> inertia(100,0,0, 0,200,0, 0,0,300);
    AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame> params(m, com, inertia);

    AegisMath::Geometry::Vector3<double, BodyFrame> force(1000.0, 0.0, 0.0);
    AegisMath::Geometry::Vector3<double, BodyFrame> moment(0.0, 0.0, 0.0);
    AegisMath::Dynamics::Wrench6<double, BodyFrame> wrench(force, moment);

    auto init_pos = AegisMath::Geometry::Point3<double, WorldFrame>(0,0,0);
    auto init_att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1,0,0,0).Value();
    auto init_vel = AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0);
    auto init_rate = AegisMath::Geometry::Vector3<double, BodyFrame>(0,0,0);

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(init_pos, init_att, init_vel, init_rate);

    AegisMath::Geometry::Vector3<double, BodyFrame> lin_accel(0,0,0);
    AegisMath::Geometry::Vector3<double, BodyFrame> ang_accel(0,0,0);

    AegisMath::Dynamics::RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(
        state, params, wrench, lin_accel, ang_accel
    );

    EXPECT_DOUBLE_EQ(lin_accel.x, 1.0); // 1000 N / 1000 kg = 1 m/s^2
}