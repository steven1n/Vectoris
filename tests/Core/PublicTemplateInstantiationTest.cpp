#include <gtest/gtest.h>
#include "AegisMath/Core/Math.h"

TEST(CorePublicTemplateTest, ConstexprMathSqrt) {
    constexpr double sq4 = AegisMath::Core::sqrt(4.0);
    EXPECT_NEAR(sq4, 2.0, 1e-12);

    constexpr double sq9 = AegisMath::Core::sqrt(9.0);
    EXPECT_NEAR(sq9, 3.0, 1e-12);

    constexpr double sq0 = AegisMath::Core::sqrt(0.0);
    EXPECT_DOUBLE_EQ(sq0, 0.0);

    constexpr double sq_neg = AegisMath::Core::sqrt(-1.0);
    EXPECT_DOUBLE_EQ(sq_neg, 0.0);
}

TEST(CorePublicTemplateTest, ConstexprMathAbs) {
    constexpr double pos = AegisMath::Core::abs(5.5);
    constexpr double neg = AegisMath::Core::abs(-5.5);
    EXPECT_DOUBLE_EQ(pos, 5.5);
    EXPECT_DOUBLE_EQ(neg, 5.5);
}
