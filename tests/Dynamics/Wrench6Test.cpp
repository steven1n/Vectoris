#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/Twist6.h"

struct BodyFrame {};

TEST(Wrench6Test, PowerConsistency) {
    AegisMath::Geometry::Vector3<double, BodyFrame> force(100.0, 0.0, 0.0);
    AegisMath::Geometry::Vector3<double, BodyFrame> moment(0.0, 10.0, 0.0);
    AegisMath::Dynamics::Wrench6<double, BodyFrame> wrench(force, moment);

    AegisMath::Geometry::Vector3<double, BodyFrame> lin(10.0, 0.0, 0.0);
    AegisMath::Geometry::Vector3<double, BodyFrame> ang(0.0, 2.0, 0.0);
    AegisMath::Dynamics::Twist6<double, BodyFrame> twist(lin, ang);

    // P = f·v + tau·omega = (100*10) + (10*2) = 1000 + 20 = 1020
    double power = wrench.Power(twist);
    EXPECT_DOUBLE_EQ(power, 1020.0);
}