#include <gtest/gtest.h>
#include <type_traits>
#include "AegisDynamics/Detail/StateTypes.h"
#include "AegisDynamics/Twist6.h"
#include "AegisDynamics/Wrench6.h"
#include "AegisDynamics/QuantityVector3.h"
#include "AegisDynamics/InertiaTensor3.h"
#include "AegisDynamics/RigidBodyParameters.h"
#include "AegisDynamics/Detail/DynamicsABI.h"

struct WorldFrame {};
struct BodyFrame {};

TEST(DynamicsABITest, StandardLayoutAndTriviality) {
    using StateType = AegisDynamics::KinematicState<double, WorldFrame, BodyFrame>;
    using TwistType = AegisDynamics::Twist6<double, BodyFrame>;
    using WrenchType = AegisDynamics::Wrench6<double, BodyFrame>;
    using VelocityType = AegisDynamics::Velocity3<BodyFrame>;
    using InertiaType = AegisDynamics::InertiaTensor3<double, BodyFrame>;
    using ParamsType = AegisDynamics::RigidBodyParameters<double, BodyFrame>;

    static_assert(AegisDynamics::Detail::DynamicsABIValidator<StateType>::value, "KinematicState ABI invalid.");
    static_assert(AegisDynamics::Detail::DynamicsABIValidator<TwistType>::value, "Twist6 ABI invalid.");
    static_assert(AegisDynamics::Detail::DynamicsABIValidator<WrenchType>::value, "Wrench6 ABI invalid.");
    static_assert(AegisDynamics::Detail::DynamicsABIValidator<VelocityType>::value, "Velocity3 ABI invalid.");
    static_assert(AegisDynamics::Detail::DynamicsABIValidator<InertiaType>::value, "InertiaTensor3 ABI invalid.");
    static_assert(AegisDynamics::Detail::DynamicsABIValidator<ParamsType>::value, "RigidBodyParameters ABI invalid.");

    // Size checks (Zero-overhead: identical to primitive storage)
    static_assert(sizeof(VelocityType) == 3 * sizeof(double));
    static_assert(sizeof(TwistType) == 6 * sizeof(double));
    static_assert(sizeof(WrenchType) == 6 * sizeof(double));
    static_assert(sizeof(InertiaType) == 9 * sizeof(double));
    static_assert(sizeof(ParamsType) == sizeof(double) + 3 * sizeof(double) + 9 * sizeof(double));

    SUCCEED();
}