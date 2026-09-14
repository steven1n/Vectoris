#include <gtest/gtest.h>
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/RigidBodyState.h"
#include "AegisMath/Dynamics/QuantityVector3.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(RigidBodyStateTest, NewtonEulerAcceleration) {
    using namespace AegisMath::Dynamics;
    using namespace AegisMath::Units;

    Kilogram m{1000.0};
    Position3<BodyFrame> com;
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(100.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(200.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(300.0)
    );
    RigidBodyParameters<double, BodyFrame> params(m, com, inertia);

    Force3<BodyFrame> force(Force(1000.0), Force::Zero(), Force::Zero());
    Torque3<BodyFrame> moment;
    Wrench6<double, BodyFrame> wrench(force, moment);

    Position3<WorldFrame> init_pos;
    auto init_att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> init_vel;
    AngularVelocity3<BodyFrame> init_rate;

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(init_pos, init_att, init_vel, init_rate);

    Acceleration3<BodyFrame> lin_accel;
    AngularAcceleration3<BodyFrame> ang_accel;

    RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(
        state, params, wrench, lin_accel, ang_accel
    );

    EXPECT_DOUBLE_EQ(lin_accel.x.value(), 1.0); // 1000 N / 1000 kg = 1 m/s^2
}