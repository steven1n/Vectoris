#include <gtest/gtest.h>
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/QuantityVector3.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(StateTypesTest, ConstructionAndBinding) {
    using namespace vectoris::dynamics;
    using namespace vectoris::numerics::Units;

    Position3<WorldFrame> pos(Meter::Zero(), Meter::Zero(), Meter(1000.0));
    auto att = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel(Velocity(100.0), Velocity::Zero(), Velocity::Zero());
    AngularVelocity3<BodyFrame> rate;

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, rate);
    EXPECT_DOUBLE_EQ(state.position.z.value(), 1000.0);
    EXPECT_DOUBLE_EQ(state.linearVelocity.x.value(), 100.0);

    // Cover TryCreate validation branches for BodyFrame->WorldFrame instantiation
    EXPECT_FALSE((vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(0.0, 0.0, 0.0, 0.0).IsSuccess()));
    EXPECT_FALSE((vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0).IsSuccess()));
    auto q_neg = vectoris::numerics::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(-1.0, 0.0, 0.0, 0.0);
    EXPECT_TRUE(q_neg.IsSuccess());
}