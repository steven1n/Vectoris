#include <gtest/gtest.h>
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/QuantityVector3.h"

struct BodyFrame {};

TEST(Wrench6Test, PowerConsistency) {
    using namespace vectoris::dynamics;
    using namespace vectoris::numerics::Units;

    Force3<BodyFrame> force(Force(100.0), Force::Zero(), Force::Zero());
    Torque3<BodyFrame> moment(Torque::Zero(), Torque(10.0), Torque::Zero());
    Wrench6<double, BodyFrame> wrench(force, moment);

    Velocity3<BodyFrame> lin(Velocity(10.0), Velocity::Zero(), Velocity::Zero());
    AngularVelocity3<BodyFrame> ang(AngularVelocity::Zero(), AngularVelocity(2.0), AngularVelocity::Zero());
    Twist6<double, BodyFrame> twist(lin, ang);

    // P = f·v + tau·omega = (100*10) + (10*2) = 1000 + 20 = 1020
    auto power = wrench.Power(twist);
    static_assert(IsQuantity<decltype(power)>);
    EXPECT_DOUBLE_EQ(power.value(), 1020.0);
}