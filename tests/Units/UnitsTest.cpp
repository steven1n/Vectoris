#include <gtest/gtest.h>
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Core/Constants.h"
#include "AegisMath/Units/Common.h"
#include "AegisMath/Units/BaseUnits/Length.h"
#include "AegisMath/Units/BaseUnits/Time.h"
#include "AegisMath/Units/BaseUnits/Length.h"  // 确保包含具体的单位定义文件

using namespace AegisMath;
using namespace AegisMath::Units;

TEST(UnitsSystemRevisionB2Test, CRTPABIAndZeroInit) {
    static_assert(sizeof(Meter) == sizeof(double));
    static_assert(alignof(Meter) == alignof(double));
    static_assert(std::is_trivially_copyable_v<Meter>);

    Meter zero_m = Meter::Zero();
    EXPECT_EQ(zero_m.value(), 0.0);
}

TEST(UnitsSystemRevisionB2Test, StrictTypeConceptAndCast) {
    // 验证 IsQuantity 的强类型过滤
    static_assert(IsQuantity<Meter>);
    
    struct FakeQuantity {}; // 简化为空结构体，消除 unused-local-typedef 警告
    static_assert(!IsQuantity<FakeQuantity>);

    Meter m{100.0};
    EXPECT_EQ(m.value(), 100.0);

    Meter distance{200.0};
    Second time_val{10.0};
    auto velocity = distance / time_val;
    EXPECT_TRUE(Traits::AlmostEqual(velocity.value(), static_cast<Scalar>(20.0), 1e-6, 1e-6));
}

#include "AegisMath/Units/BaseUnits/Mass.h"
#include "AegisMath/Units/BaseUnits/Angle.h"
#include "AegisMath/Units/DerivedUnits/Acceleration.h"
#include "AegisMath/Units/DerivedUnits/Force.h"
#include "AegisMath/Units/DerivedUnits/AngularVelocity.h"
#include "AegisMath/Units/DerivedUnits/AngularAcceleration.h"
#include "AegisMath/Units/DerivedUnits/Torque.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"
#include "AegisMath/Units/DerivedUnits/Power.h"
#include "AegisMath/Units/Literals.h"

TEST(UnitsSystemRevisionB2Test, RotationalAndInertiaUnits) {
    using namespace AegisMath::Units::Literals;

    Kilogram mass{10.0};
    Acceleration acc{2.0};
    Force f_calc = mass * acc;
    EXPECT_DOUBLE_EQ(f_calc.value(), 20.0);

    static_assert(IsQuantity<Kilogram>);
    static_assert(IsQuantity<AngularVelocity>);
    static_assert(IsQuantity<AngularAcceleration>);
    static_assert(IsQuantity<Torque>);
    static_assert(IsQuantity<MomentOfInertia>);
    static_assert(IsQuantity<Power>);

    // ABI checks
    static_assert(sizeof(AngularVelocity) == sizeof(double));
    static_assert(sizeof(AngularAcceleration) == sizeof(double));
    static_assert(sizeof(Torque) == sizeof(double));
    static_assert(sizeof(MomentOfInertia) == sizeof(double));
    static_assert(sizeof(Power) == sizeof(double));

    // Force * Length -> Torque
    Force force{50.0};
    Meter arm{2.0};
    Torque torque = force * arm;
    EXPECT_DOUBLE_EQ(torque.value(), 100.0);

    // Inertia * AngularAcceleration -> Torque
    MomentOfInertia inertia{5.0};
    AngularAcceleration alpha{4.0};
    Torque dyn_torque = RotationalTorque(inertia, alpha);
    EXPECT_DOUBLE_EQ(dyn_torque.value(), 20.0);

    // Torque / Inertia -> AngularAcceleration
    AngularAcceleration alpha_calc = RotationalAcceleration(dyn_torque, inertia);
    EXPECT_DOUBLE_EQ(alpha_calc.value(), 4.0);

    // Power calculations
    Velocity vel{10.0};
    Power p_trans = force * vel;
    EXPECT_DOUBLE_EQ(p_trans.value(), 500.0);

    AngularVelocity omega{2.0};
    Power p_rot = RotationalPower(torque, omega);
    EXPECT_DOUBLE_EQ(p_rot.value(), 200.0);

    // Literals check
    auto l_kg = 5.0_kg;
    auto l_N = 100.0_N;
    auto l_Nm = 25.0_Nm;
    auto l_W = 1000.0_W;
    auto l_rad_s = 3.14_rad_s;
    EXPECT_DOUBLE_EQ(l_kg.value(), 5.0);
    EXPECT_DOUBLE_EQ(l_N.value(), 100.0);
    EXPECT_DOUBLE_EQ(l_Nm.value(), 25.0);
    EXPECT_DOUBLE_EQ(l_W.value(), 1000.0);
    EXPECT_DOUBLE_EQ(l_rad_s.value(), 3.14);
}