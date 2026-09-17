#include <gtest/gtest.h>
#include "Vectoris/Numerics/Core/Math.h"

TEST(CorePublicTemplateTest, ConstexprMathSqrt) {
    constexpr double sq4 = vectoris::numerics::Core::sqrt(4.0);
    EXPECT_NEAR(sq4, 2.0, 1e-12);

    constexpr double sq9 = vectoris::numerics::Core::sqrt(9.0);
    EXPECT_NEAR(sq9, 3.0, 1e-12);

    constexpr double sq0 = vectoris::numerics::Core::sqrt(0.0);
    EXPECT_DOUBLE_EQ(sq0, 0.0);

    constexpr double sq_neg = vectoris::numerics::Core::sqrt(-1.0);
    EXPECT_DOUBLE_EQ(sq_neg, 0.0);
}

TEST(CorePublicTemplateTest, ConstexprMathAbs) {
    constexpr double pos = vectoris::numerics::Core::abs(5.5);
    constexpr double neg = vectoris::numerics::Core::abs(-5.5);
    EXPECT_DOUBLE_EQ(pos, 5.5);
    EXPECT_DOUBLE_EQ(neg, 5.5);
}
