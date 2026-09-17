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
#include "AegisMath/Units/DerivedUnits/Frequency.h"
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

TEST(UnitsSystemRevisionB2Test, FrequencyCompileTimeContractAndAlgebra) {
    // 1. Frequency dimension & unit contract
    static_assert(std::same_as<typename HertzUnit::Dimension, FrequencyDimension>);
    static_assert(HertzUnit::Dimension::time == -1);
    static_assert(HertzUnit::Dimension::length == 0);
    static_assert(HertzUnit::Dimension::mass == 0);
    static_assert(HertzUnit::Dimension::current == 0);
    static_assert(HertzUnit::Dimension::temperature == 0);
    static_assert(HertzUnit::Dimension::amount == 0);
    static_assert(HertzUnit::Dimension::luminosity == 0);
    static_assert(HertzUnit::Dimension::angle == 0);

    // 2. Strong type binding and ABI safety
    static_assert(std::same_as<Frequency, Quantity<Scalar, HertzUnit>>);
    static_assert(std::same_as<Hertz, Frequency>);
    static_assert(IsQuantity<Frequency>);
    static_assert(sizeof(Frequency) == sizeof(double));
    static_assert(alignof(Frequency) == alignof(double));
    static_assert(std::is_trivially_copyable_v<Frequency>);

    // 3. Regression against duplicate NewtonUnit collision
    static_assert(!std::same_as<NewtonUnit, HertzUnit>);
    static_assert(!std::same_as<Force, Frequency>);
    static_assert(!DimensionEqual<NewtonUnit::Dimension, HertzUnit::Dimension>);

    // 4. Dimensional algebra: Frequency (T^-1) * Time (T^1) -> Dimensionless
    Frequency freq{50.0};
    Second time_span{0.1};
    auto cycles = freq * time_span;
    static_assert(DimensionEqual<decltype(cycles)::DimensionType, Dimensionless>);
    EXPECT_DOUBLE_EQ(cycles.value(), 5.0);

    // 5. Dimension algebra: Dimensionless / Time (T^1) -> Frequency (T^-1)
    auto recovered_rate = cycles / time_span;
    static_assert(DimensionEqual<decltype(recovered_rate)::DimensionType, FrequencyDimension>);
    EXPECT_DOUBLE_EQ(recovered_rate.value(), 50.0);
}

TEST(UnitsSystemCoverageTest, LiteralsRuntimeExercise) {
    using namespace AegisMath::Units::Literals;

    volatile long double d_val = 2.5L;
    volatile unsigned long long u_val = 3ULL;

    auto m_flt = operator""_m(d_val);
    auto m_int = operator""_m(u_val);
    EXPECT_DOUBLE_EQ(m_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(m_int.value(), 3.0);

    auto s_flt = operator""_s(d_val);
    auto s_int = operator""_s(u_val);
    EXPECT_DOUBLE_EQ(s_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(s_int.value(), 3.0);

    auto rad_flt = operator""_rad(d_val);
    auto rad_int = operator""_rad(u_val);
    EXPECT_DOUBLE_EQ(rad_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(rad_int.value(), 3.0);

    auto kg_flt = operator""_kg(d_val);
    auto kg_int = operator""_kg(u_val);
    EXPECT_DOUBLE_EQ(kg_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(kg_int.value(), 3.0);

    auto n_flt = operator""_N(d_val);
    auto n_int = operator""_N(u_val);
    EXPECT_DOUBLE_EQ(n_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(n_int.value(), 3.0);

    auto nm_flt = operator""_Nm(d_val);
    auto nm_int = operator""_Nm(u_val);
    EXPECT_DOUBLE_EQ(nm_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(nm_int.value(), 3.0);

    auto w_flt = operator""_W(d_val);
    auto w_int = operator""_W(u_val);
    EXPECT_DOUBLE_EQ(w_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(w_int.value(), 3.0);

    auto mps_flt = operator""_mps(d_val);
    auto mps_int = operator""_mps(u_val);
    EXPECT_DOUBLE_EQ(mps_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(mps_int.value(), 3.0);

    auto rad_s_flt = operator""_rad_s(d_val);
    auto rad_s_int = operator""_rad_s(u_val);
    EXPECT_DOUBLE_EQ(rad_s_flt.value(), 2.5);
    EXPECT_DOUBLE_EQ(rad_s_int.value(), 3.0);
}

TEST(UnitsSystemCoverageTest, DetailABIRuntimeExercise) {
    EXPECT_TRUE(AegisMath::Units::Detail::QuantityABIValidator<Meter>::Validate());
    EXPECT_TRUE(AegisMath::Units::Detail::ValidateQuantityABI<Meter>());
    EXPECT_TRUE(AegisMath::Units::Detail::QuantityABIValidator<Second>::Validate());
    EXPECT_TRUE(AegisMath::Units::Detail::ValidateQuantityABI<Second>());

    struct DummyNonMatchingSize {
        using ValueType = double;
        double a;
        double b;
    };
    EXPECT_FALSE(AegisMath::Units::Detail::QuantityABIValidator<DummyNonMatchingSize>::Validate());

    struct DummyNonTrivial {
        using ValueType = double;
        double a;
        DummyNonTrivial() : a(0.0) {}
        DummyNonTrivial(const DummyNonTrivial& o) : a(o.a) {}
        DummyNonTrivial& operator=(const DummyNonTrivial& o) { a = o.a; return *this; }
        ~DummyNonTrivial() = default;
        DummyNonTrivial(DummyNonTrivial&&) = default;
        DummyNonTrivial& operator=(DummyNonTrivial&&) = default;
    };
    EXPECT_FALSE(AegisMath::Units::Detail::QuantityABIValidator<DummyNonTrivial>::Validate());
}

TEST(UnitsSystemCoverageTest, UnaryNegationAndABINonStandardLayout) {
    using namespace AegisMath::Units;
    Quantity<double, MeterUnit> q(5.0);
    auto neg = -q;
    EXPECT_DOUBLE_EQ(neg.value(), -5.0);
    auto pos = -neg;
    EXPECT_DOUBLE_EQ(pos.value(), 5.0);

    auto (*zero_fn)() = &Quantity<double, MeterUnit>::Zero;
    auto zero_m = zero_fn();
    EXPECT_DOUBLE_EQ(zero_m.value(), 0.0);

    struct NonStandardLayoutBase { int x; };
    struct DummyNonStandardLayout : NonStandardLayoutBase {
        using ValueType = double;
        int y;
    };
    EXPECT_FALSE(AegisMath::Units::Detail::QuantityABIValidator<DummyNonStandardLayout>::Validate());
}

