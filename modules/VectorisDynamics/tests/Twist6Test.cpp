#include <gtest/gtest.h>
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/QuantityVector3.h"

struct BodyFrame {};

TEST(Twist6Test, ComponentAccess) {
    using namespace vectoris::dynamics;
    using namespace vectoris::numerics::Units;

    Velocity3<BodyFrame> lin(Velocity(10.0), Velocity(2.0), Velocity(-1.0));
    AngularVelocity3<BodyFrame> ang(AngularVelocity(0.1), AngularVelocity(0.05), AngularVelocity(0.02));

    Twist6<double, BodyFrame> twist(lin, ang);

    EXPECT_DOUBLE_EQ(twist.linear.x.value(), 10.0);
    EXPECT_DOUBLE_EQ(twist.angular.z.value(), 0.02);
}