#include <gtest/gtest.h>
#include <type_traits>
#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/Detail/DynamicsABI.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(DynamicsABITest, StandardLayoutAndTriviality) {
    using StateType = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>;
    using TwistType = AegisMath::Dynamics::Twist6<double, BodyFrame>;
    using WrenchType = AegisMath::Dynamics::Wrench6<double, BodyFrame>;

    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<StateType>::value, "KinematicState ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<TwistType>::value, "Twist6 ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<WrenchType>::value, "Wrench6 ABI invalid.");
    SUCCEED();
}