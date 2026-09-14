#include <gtest/gtest.h>
#include <type_traits>
#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Detail/DynamicsABI.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(DynamicsABITest, StandardLayoutAndTriviality) {
    using StateType = AegisMath::Dynamics::KinematicState<double, WorldFrame, BodyFrame>;
    using TwistType = AegisMath::Dynamics::Twist6<double, BodyFrame>;
    using WrenchType = AegisMath::Dynamics::Wrench6<double, BodyFrame>;
    using VelocityType = AegisMath::Dynamics::Velocity3<BodyFrame>;
    using InertiaType = AegisMath::Dynamics::InertiaTensor3<double, BodyFrame>;
    using ParamsType = AegisMath::Dynamics::RigidBodyParameters<double, BodyFrame>;

    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<StateType>::value, "KinematicState ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<TwistType>::value, "Twist6 ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<WrenchType>::value, "Wrench6 ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<VelocityType>::value, "Velocity3 ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<InertiaType>::value, "InertiaTensor3 ABI invalid.");
    static_assert(AegisMath::Dynamics::Detail::DynamicsABIValidator<ParamsType>::value, "RigidBodyParameters ABI invalid.");

    // Size checks (Zero-overhead: identical to primitive storage)
    static_assert(sizeof(VelocityType) == 3 * sizeof(double));
    static_assert(sizeof(TwistType) == 6 * sizeof(double));
    static_assert(sizeof(WrenchType) == 6 * sizeof(double));
    static_assert(sizeof(InertiaType) == 9 * sizeof(double));
    static_assert(sizeof(ParamsType) == sizeof(double) + 3 * sizeof(double) + 9 * sizeof(double));

    SUCCEED();
}