#include <gtest/gtest.h>
#include <type_traits>
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/QuantityVector3.h"
#include "Vectoris/Dynamics/InertiaTensor3.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Detail/DynamicsABI.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(DynamicsABITest, StandardLayoutAndTriviality) {
    using StateType = vectoris::dynamics::KinematicState<double, WorldFrame, BodyFrame>;
    using TwistType = vectoris::dynamics::Twist6<double, BodyFrame>;
    using WrenchType = vectoris::dynamics::Wrench6<double, BodyFrame>;
    using VelocityType = vectoris::dynamics::Velocity3<BodyFrame>;
    using InertiaType = vectoris::dynamics::InertiaTensor3<double, BodyFrame>;
    using ParamsType = vectoris::dynamics::RigidBodyParameters<double, BodyFrame>;

    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<StateType>::value, "KinematicState ABI invalid.");
    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<TwistType>::value, "Twist6 ABI invalid.");
    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<WrenchType>::value, "Wrench6 ABI invalid.");
    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<VelocityType>::value, "Velocity3 ABI invalid.");
    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<InertiaType>::value, "InertiaTensor3 ABI invalid.");
    static_assert(vectoris::dynamics::Detail::DynamicsABIValidator<ParamsType>::value, "RigidBodyParameters ABI invalid.");

    // Size checks (Zero-overhead: identical to primitive storage)
    static_assert(sizeof(VelocityType) == 3 * sizeof(double));
    static_assert(sizeof(TwistType) == 6 * sizeof(double));
    static_assert(sizeof(WrenchType) == 6 * sizeof(double));
    static_assert(sizeof(InertiaType) == 9 * sizeof(double));
    static_assert(sizeof(ParamsType) == sizeof(double) + 3 * sizeof(double) + 9 * sizeof(double));

    SUCCEED();
}