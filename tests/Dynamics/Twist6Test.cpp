#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Twist6.h"

struct BodyFrame {};

TEST(Twist6Test, ComponentAccess) {
    AegisMath::Geometry::Vector3<double, BodyFrame> lin(10.0, 2.0, -1.0);
    AegisMath::Geometry::Vector3<double, BodyFrame> ang(0.1, 0.05, 0.02);

    AegisMath::Dynamics::Twist6<double, BodyFrame> twist(lin, ang);

    EXPECT_DOUBLE_EQ(twist.linear.x, 10.0);
    EXPECT_DOUBLE_EQ(twist.angular.z, 0.02);
}