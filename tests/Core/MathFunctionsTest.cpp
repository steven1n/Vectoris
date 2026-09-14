#include <gtest/gtest.h>
#include <cmath>
#include <numbers>
#include "AegisMath/Core/Math.h"
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

// =========================================================================
// AML-MED-004: Core::sqrt Bounded Newton & Contract Tests
// =========================================================================

TEST(CoreSqrtTest, ConstexprEvaluation) {
    static_assert(Core::sqrt(0.0) == 0.0, "sqrt(0) must be 0");
    static_assert(Core::sqrt(-1.0) == 0.0, "sqrt(negative) must be 0 per Aegis contract");
    static_assert(Core::sqrt(4.0) == 2.0, "sqrt(4) must be 2");
    static_assert(Core::sqrt(9.0) == 3.0, "sqrt(9) must be 3");
    static_assert(Core::abs(Core::sqrt(2.0) - 1.4142135623730951) < 1e-14, "sqrt(2) approx failed");

    constexpr double c_res = Core::sqrt(100.0);
    EXPECT_DOUBLE_EQ(c_res, 10.0);
}

TEST(CoreSqrtTest, BoundaryConditions) {
    // Zero
    EXPECT_DOUBLE_EQ(Core::sqrt(0.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-0.0), 0.0);

    // Negative values defensively clamped to 0
    EXPECT_DOUBLE_EQ(Core::sqrt(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-1e20), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-std::numeric_limits<double>::infinity()), 0.0);

    // Positive infinity
    EXPECT_TRUE(std::isinf(Core::sqrt(std::numeric_limits<double>::infinity())));
    EXPECT_GT(Core::sqrt(std::numeric_limits<double>::infinity()), 0.0);

    // Quiet NaN
    EXPECT_TRUE(std::isnan(Core::sqrt(std::numeric_limits<double>::quiet_NaN())));
}

TEST(CoreSqrtTest, RangeComparisonAgainstReference) {
    const double test_values[] = {
        1e-300, 1e-250, 1e-200, 1e-150, 1e-100, 1e-50, 1e-20, 1e-10, 1e-5,
        0.001, 0.01, 0.1, 0.5, 1.0, 2.0, 3.0, 4.0, 9.0, 16.0, 25.0, 100.0,
        1e5, 1e10, 1e20, 1e50, 1e100, 1e150, 1e200, 1e250, 1e300
    };

    for (double val : test_values) {
        const double ref = std::sqrt(val);

        // Test public Core::sqrt
        const double actual_public = Core::sqrt(val);
        const double rel_err_public = std::abs(actual_public - ref) / ref;
        EXPECT_LE(rel_err_public, 2e-15) << "Failed for public sqrt with val = " << val;

        // Test Detail::BoundedNewtonSqrt
        std::size_t iters = 0;
        const double actual_newton = Core::Detail::BoundedNewtonSqrt(val, &iters);
        const double rel_err_newton = std::abs(actual_newton - ref) / ref;
        EXPECT_LE(rel_err_newton, 2e-15) << "Failed for Newton sqrt with val = " << val;
        EXPECT_LE(iters, 64U) << "Exceeded max iterations for val = " << val;
    }
}

TEST(CoreSqrtTest, SubnormalAndPrecisionEdgeCases) {
    const double min_normal = std::numeric_limits<double>::min();
    const double denorm_min = std::numeric_limits<double>::denorm_min();

    // Normal boundary
    EXPECT_NEAR(Core::sqrt(min_normal), std::sqrt(min_normal), 1e-160);

    // Subnormal smallest value
    std::size_t iters = 0;
    const double denorm_res = Core::Detail::BoundedNewtonSqrt(denorm_min, &iters);
    EXPECT_DOUBLE_EQ(denorm_res, std::sqrt(denorm_min));
    EXPECT_LE(iters, 64U);
}

TEST(CoreSqrtTest, IterationBoundVerification) {
    const double sample_inputs[] = {
        1e-300, 1e-150, 1e-50, 1e-10, 0.01, 0.5, 1.0, 2.0, 4.0, 100.0, 1e10, 1e50, 1e150, 1e300
    };

    std::size_t max_observed_iters = 0;
    for (double val : sample_inputs) {
        std::size_t iters = 0;
        Core::Detail::BoundedNewtonSqrt(val, &iters);
        EXPECT_LE(iters, 64U);
        if (iters > max_observed_iters) {
            max_observed_iters = iters;
        }
    }

    // Mathematical guarantee: quadratic convergence achieves full precision within 5 iterations
    EXPECT_LE(max_observed_iters, 6U);
}

TEST(CoreSqrtTest, Float32Support) {
    const float test_values_f[] = {
        1e-35f, 1e-20f, 1e-10f, 1e-5f, 0.01f, 0.5f, 1.0f, 2.0f, 4.0f, 9.0f, 100.0f, 1e5f, 1e10f, 1e20f, 1e35f
    };

    std::size_t max_observed_iters_f = 0;
    for (float val : test_values_f) {
        const float ref = std::sqrt(val);
        const float actual = Core::sqrt(val);
        const float rel_err = std::abs(actual - ref) / ref;
        EXPECT_LE(rel_err, 2e-7f) << "Failed for float val = " << val;

        std::size_t iters = 0;
        const float actual_newton = Core::Detail::BoundedNewtonSqrt(val, &iters);
        const float rel_err_newton = std::abs(actual_newton - ref) / ref;
        EXPECT_LE(rel_err_newton, 2e-7f) << "Failed for float Newton with val = " << val;
        EXPECT_LE(iters, 64U);
        if (iters > max_observed_iters_f) {
            max_observed_iters_f = iters;
        }
    }
    EXPECT_LE(max_observed_iters_f, 5U);
}
