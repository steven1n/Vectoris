#include <gtest/gtest.h>
#include <cstdint>

#include "Vectoris/Numerics/Units/BaseUnits/Length.h"
#include "Vectoris/Numerics/Units/BaseUnits/Mass.h"
#include "Vectoris/Numerics/Units/Detail/Ratio.h"
#include "Vectoris/Numerics/Units/Literals.h"
#include "Vectoris/Numerics/Units/QuantityABI.h"
#include "Vectoris/Numerics/Units/UnitCast.h"

TEST(UnitsPublicTemplateTest, LiteralsInstantiation) {
    using namespace vectoris::numerics::Units::Literals;

    auto len_fp = 12.5_m;
    auto len_int = 10_m;
    EXPECT_DOUBLE_EQ(len_fp.value(), 12.5);
    EXPECT_DOUBLE_EQ(len_int.value(), 10.0);

    auto time_fp = 0.5_s;
    auto time_int = 3_s;
    EXPECT_DOUBLE_EQ(time_fp.value(), 0.5);
    EXPECT_DOUBLE_EQ(time_int.value(), 3.0);

    auto ang_fp = 1.5707963267948966_rad;
    auto ang_int = 1_rad;
    EXPECT_NEAR(ang_fp.value(), 1.5707963267948966, 1e-12);
    EXPECT_DOUBLE_EQ(ang_int.value(), 1.0);
}

TEST(UnitsPublicTemplateTest, QuantityArithmeticWithLiterals) {
    using namespace vectoris::numerics::Units::Literals;

    auto l1 = 5.0_m;
    auto l2 = 3.0_m;
    auto sum = l1 + l2;
    auto diff = l1 - l2;

    EXPECT_DOUBLE_EQ(sum.value(), 8.0);
    EXPECT_DOUBLE_EQ(diff.value(), 2.0);
}

TEST(UnitsPublicTemplateTest, UnitCastInstantiationAndExecution) {
    using namespace vectoris::numerics::Units;

    Kilogram kg(2.5);
    // Cross-unit conversion between matching dimensions (Kilogram -> Gram)
    Gram g = unit_cast<GramUnit>(kg);
    EXPECT_DOUBLE_EQ(g.value(), 2500.0);

    // Inverse conversion (Gram -> Kilogram)
    Kilogram kg_rec = unit_cast<KilogramUnit>(g);
    EXPECT_DOUBLE_EQ(kg_rec.value(), 2.5);

    // Identity conversion (same ToUnit and FromUnit)
    Kilogram kg_ident = unit_cast<KilogramUnit>(kg);
    EXPECT_DOUBLE_EQ(kg_ident.value(), 2.5);

    // Single-precision float unit_cast
    Quantity<float, KilogramUnit> kg_f(1.5f);
    Quantity<float, GramUnit> g_f = unit_cast<GramUnit>(kg_f);
    EXPECT_FLOAT_EQ(g_f.value(), 1500.0f);
}

TEST(UnitsPublicTemplateTest, QuantityABIPublicValidatorExecution) {
    using namespace vectoris::numerics::Units;

    EXPECT_TRUE(QuantityABIValidator<Meter>::Validate());
    EXPECT_TRUE(QuantityABIValidator<Kilogram>::Validate());
    EXPECT_TRUE(QuantityABIRegistration<Meter>);
    EXPECT_TRUE(QuantityABIRegistration<Kilogram>);
}

TEST(UnitsPublicTemplateTest, DetailSafeAbsRuntimeExecution) {
    using namespace vectoris::numerics::Units::Detail;

    EXPECT_EQ(SafeAbs(static_cast<std::intmax_t>(100)), 100);
    EXPECT_EQ(SafeAbs(static_cast<std::intmax_t>(-100)), 100);
    EXPECT_EQ(SafeAbs(static_cast<std::intmax_t>(0)), 0);
}
