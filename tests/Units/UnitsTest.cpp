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
    static_assert(IsQuantity<AngularMomentum>);

    // ABI checks
    static_assert(sizeof(AngularVelocity) == sizeof(double));
    static_assert(sizeof(AngularAcceleration) == sizeof(double));
    static_assert(sizeof(Torque) == sizeof(double));
    static_assert(sizeof(MomentOfInertia) == sizeof(double));
    static_assert(sizeof(Power) == sizeof(double));
    static_assert(sizeof(AngularMomentum) == sizeof(double));

    // Force * Length yields Energy (M L^2 T^-2 A^0), dimensionally distinct from Torque (M L^2 T^-2 A^-1)
    Force force{50.0};
    Meter arm{2.0};
    auto work_or_energy = force * arm;
    static_assert(DimensionEqual<decltype(work_or_energy)::DimensionType, EnergyDimension>);
    static_assert(!std::is_constructible_v<Torque, decltype(work_or_energy)>);
    EXPECT_DOUBLE_EQ(work_or_energy.value(), 100.0);

    // Inertia * AngularAcceleration -> Torque (via operator* and helper)
    MomentOfInertia inertia{5.0};
    AngularAcceleration alpha{4.0};
    Torque dyn_torque = inertia * alpha;
    EXPECT_DOUBLE_EQ(dyn_torque.value(), 20.0);
    Torque dyn_torque_fn = RotationalTorque(inertia, alpha);
    EXPECT_DOUBLE_EQ(dyn_torque_fn.value(), 20.0);

    // Torque / Inertia -> AngularAcceleration (via operator/ and helper)
    AngularAcceleration alpha_calc = dyn_torque / inertia;
    EXPECT_DOUBLE_EQ(alpha_calc.value(), 4.0);
    AngularAcceleration alpha_calc_fn = RotationalAcceleration(dyn_torque, inertia);
    EXPECT_DOUBLE_EQ(alpha_calc_fn.value(), 4.0);

    // Power calculations
    Velocity vel{10.0};
    Power p_trans = force * vel;
    EXPECT_DOUBLE_EQ(p_trans.value(), 500.0);

    AngularVelocity omega{2.0};
    Power p_rot = dyn_torque * omega;
    EXPECT_DOUBLE_EQ(p_rot.value(), 40.0);
    Power p_rot_fn = RotationalPower(dyn_torque, omega);
    EXPECT_DOUBLE_EQ(p_rot_fn.value(), 40.0);

    // Power / AngularVelocity -> Torque
    Torque tau_from_power = p_rot / omega;
    EXPECT_DOUBLE_EQ(tau_from_power.value(), 20.0);

    // Inertia * AngularVelocity -> AngularMomentum
    AngularMomentum L = inertia * omega;
    EXPECT_DOUBLE_EQ(L.value(), 10.0);
    AngularVelocity omega_rec = L / inertia;
    EXPECT_DOUBLE_EQ(omega_rec.value(), 2.0);
    MomentOfInertia I_rec_from_L = L / omega;
    EXPECT_DOUBLE_EQ(I_rec_from_L.value(), 5.0);

    // Torque / AngularAcceleration -> MomentOfInertia
    MomentOfInertia inertia_from_tau = dyn_torque / alpha;
    EXPECT_DOUBLE_EQ(inertia_from_tau.value(), 5.0);
    MomentOfInertia inertia_from_tau_fn = RotationalInertia(dyn_torque, alpha);
    EXPECT_DOUBLE_EQ(inertia_from_tau_fn.value(), 5.0);

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