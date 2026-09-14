#include <gtest/gtest.h>
#include "AegisMath/Units/Literals.h"

TEST(UnitsPublicTemplateTest, LiteralsInstantiation) {
    using namespace AegisMath::Units::Literals;

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
    using namespace AegisMath::Units::Literals;

    auto l1 = 5.0_m;
    auto l2 = 3.0_m;
    auto sum = l1 + l2;
    auto diff = l1 - l2;

    EXPECT_DOUBLE_EQ(sum.value(), 8.0);
    EXPECT_DOUBLE_EQ(diff.value(), 2.0);
}
