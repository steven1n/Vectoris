#include <gtest/gtest.h>
#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Geometry/Point3.h"
#include "AegisMath/Geometry/Quaternion.h"
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Detail/StateTypes.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(StateTypesTest, ConstructionAndBinding) {
    AegisMath::Geometry::Point3<double, WorldFrame> pos(0.0, 0.0, 1000.0);
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    AegisMath::Geometry::Vector3<double, BodyFrame> vel(100.0, 0.0, 0.0);
    AegisMath::Geometry::Vector3<double, BodyFrame> rate(0.0, 0.0, 0.0);

    auto state = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, rate);
    EXPECT_DOUBLE_EQ(state.position.z, 1000.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.x, 100.0);
}