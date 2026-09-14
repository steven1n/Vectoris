#include <gtest/gtest.h>
#include <cmath>
#include <numbers>
#include "AegisMath/Core/MathFunctions.h"
#include "AegisMath/Core/Constants.h"
#include "AegisMath/Core/NumericTraits.h"

using namespace AegisMath;
using namespace AegisMath::Core::Math;

TEST(MathFunctionsTest, AcosStandardDomain) {
    const double pi = std::numbers::pi_v<double>;
    const double half_pi = pi / 2.0;

    // acos(1) = 0
    EXPECT_DOUBLE_EQ(Core::Math::acos(1.0), 0.0);

    // acos(0) = pi / 2
    EXPECT_NEAR(Core::Math::acos(0.0), half_pi, 1e-15);

    // acos(-1) = pi (CRITICAL: reproduction target)
    EXPECT_NEAR(Core::Math::acos(-1.0), pi, 1e-15);

    // Interior points
    EXPECT_NEAR(Core::Math::acos(0.5), pi / 3.0, 1e-15);
    EXPECT_NEAR(Core::Math::acos(-0.5), 2.0 * pi / 3.0, 1e-15);
}

TEST(MathFunctionsTest, AcosNearBoundaryPoints) {
    const double pi = std::numbers::pi_v<double>;

    // nextafter(1, 0) is strictly < 1.0
    double just_below_one = std::nextafter(1.0, 0.0);
    double val_below_one = Core::Math::acos(just_below_one);
    EXPECT_TRUE(std::isfinite(val_below_one));
    EXPECT_GT(val_below_one, 0.0);
    EXPECT_LT(val_below_one, 1e-7);

    // nextafter(-1, 0) is strictly > -1.0
    double just_above_minus_one = std::nextafter(-1.0, 0.0);
    double val_above_minus_one = Core::Math::acos(just_above_minus_one);
    EXPECT_TRUE(std::isfinite(val_above_minus_one));
    EXPECT_LT(val_above_minus_one, pi);
    EXPECT_GT(val_above_minus_one, pi - 1e-7);
}

TEST(MathFunctionsTest, AcosBoundaryRoundoffClamping) {
    const double pi = std::numbers::pi_v<double>;
    const double eps = Traits::NumericTraits<double>::epsilon();

    // 1.0 + small_roundoff (e.g. from dot product of normalized vectors)
    double slightly_above_one = 1.0 + eps;
    EXPECT_DOUBLE_EQ(Core::Math::acos(slightly_above_one), 0.0);

    // -1.0 - small_roundoff
    double slightly_below_minus_one = -1.0 - eps;
    EXPECT_NEAR(Core::Math::acos(slightly_below_minus_one), pi, 1e-15);
}

TEST(MathFunctionsTest, AcosDomainOverflowReturnsNaN) {
    // Distinct invalid inputs outside the small roundoff boundary should follow IEEE-754
    double invalid_pos = 1.5;
    double invalid_neg = -2.0;

    EXPECT_TRUE(std::isnan(Core::Math::acos(invalid_pos)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(invalid_neg)));
}

TEST(MathFunctionsTest, AcosFloat32) {
    const float pi_f = std::numbers::pi_v<float>;

    EXPECT_FLOAT_EQ(Core::Math::acos(1.0f), 0.0f);
    EXPECT_NEAR(Core::Math::acos(0.0f), pi_f / 2.0f, 1e-6f);
    EXPECT_NEAR(Core::Math::acos(-1.0f), pi_f, 1e-6f);
}
