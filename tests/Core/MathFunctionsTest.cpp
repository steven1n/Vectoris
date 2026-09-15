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

template <typename T>
concept CanSqrt = requires(T val) {
    { Core::sqrt(val) };
};

TEST(CoreSqrtTest, TypeDomainCompileTimeRejection) {
    // 允许的 IEEE-754 标量类型
    static_assert(CanSqrt<float>, "float must be supported");
    static_assert(CanSqrt<double>, "double must be supported");
    static_assert(CanSqrt<const float>, "const float must be supported");
    static_assert(CanSqrt<const double>, "const double must be supported");

    // 拒绝整型与布尔型 (禁止数值开方隐式类型推导)
    static_assert(!CanSqrt<int>, "int must be rejected");
    static_assert(!CanSqrt<unsigned int>, "unsigned int must be rejected");
    static_assert(!CanSqrt<std::int64_t>, "int64 must be rejected");
    static_assert(!CanSqrt<bool>, "bool must be rejected");

    // 拒绝 long double (AegisMath 契约限定标量类型仅为 IEEE-754 binary32 与 binary64)
    static_assert(!CanSqrt<long double>, "long double must be rejected by contract");
}

TEST(CoreSqrtTest, ConstexprEvaluationDouble) {
    static_assert(Core::sqrt(0.0) == 0.0, "sqrt(0) must be 0");
    static_assert(Core::sqrt(-1.0) == 0.0, "sqrt(negative) must be 0 per Aegis domain policy");
    static_assert(Core::sqrt(4.0) == 2.0, "sqrt(4) must be 2");
    static_assert(Core::sqrt(9.0) == 3.0, "sqrt(9) must be 3");
    static_assert(Core::abs(Core::sqrt(2.0) - 1.4142135623730951) < 1e-14, "sqrt(2) approx failed");

    constexpr double c_res = Core::sqrt(100.0);
    EXPECT_DOUBLE_EQ(c_res, 10.0);
}

TEST(CoreSqrtTest, ConstexprEvaluationFloat) {
    static_assert(Core::sqrt(0.0f) == 0.0f, "sqrt(0.0f) must be 0");
    static_assert(Core::sqrt(-1.0f) == 0.0f, "sqrt(-1.0f) must be 0 per Aegis domain policy");
    static_assert(Core::sqrt(4.0f) == 2.0f, "sqrt(4.0f) must be 2");
    static_assert(Core::sqrt(9.0f) == 3.0f, "sqrt(9.0f) must be 3");
    static_assert(Core::abs(Core::sqrt(2.0f) - 1.4142135f) < 1e-6f, "sqrt(2.0f) approx failed");

    constexpr float c_res_f = Core::sqrt(100.0f);
    EXPECT_FLOAT_EQ(c_res_f, 10.0f);
}

TEST(CoreSqrtTest, SignedZeroAndNegativeDomainPolicy) {
    // AegisMath Core::sqrt 专属定义域政策：
    // 非正数一律防御性截断为 +0.0，避免非实数域 NaN 扩散
    EXPECT_DOUBLE_EQ(Core::sqrt(0.0), 0.0);
    EXPECT_FALSE(std::signbit(Core::sqrt(0.0)));

    EXPECT_DOUBLE_EQ(Core::sqrt(-0.0), 0.0);
    EXPECT_FALSE(std::signbit(Core::sqrt(-0.0))); // 保留既有契约：sqrt(-0.0) -> +0.0

    EXPECT_DOUBLE_EQ(Core::sqrt(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-1e20), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-1e300), 0.0);
    EXPECT_DOUBLE_EQ(Core::sqrt(-std::numeric_limits<double>::infinity()), 0.0);

    // float 对应 signed zero 与负数策略
    EXPECT_FLOAT_EQ(Core::sqrt(0.0f), 0.0f);
    EXPECT_FALSE(std::signbit(Core::sqrt(0.0f)));
    EXPECT_FLOAT_EQ(Core::sqrt(-0.0f), 0.0f);
    EXPECT_FALSE(std::signbit(Core::sqrt(-0.0f)));
    EXPECT_FLOAT_EQ(Core::sqrt(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(Core::sqrt(-std::numeric_limits<float>::infinity()), 0.0f);
}

TEST(CoreSqrtTest, BoundaryConditions) {
    // Positive infinity
    EXPECT_TRUE(std::isinf(Core::sqrt(std::numeric_limits<double>::infinity())));
    EXPECT_GT(Core::sqrt(std::numeric_limits<double>::infinity()), 0.0);
    EXPECT_TRUE(std::isinf(Core::sqrt(std::numeric_limits<float>::infinity())));
    EXPECT_GT(Core::sqrt(std::numeric_limits<float>::infinity()), 0.0f);

    // Quiet NaN
    EXPECT_TRUE(std::isnan(Core::sqrt(std::numeric_limits<double>::quiet_NaN())));
    EXPECT_TRUE(std::isnan(Core::sqrt(std::numeric_limits<float>::quiet_NaN())));
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

TEST(CoreSqrtTest, ExtendedCoverageEdgeCases) {
    // 1. Math::abs negative branch
    EXPECT_EQ(Core::Math::abs(-42), 42);
    EXPECT_DOUBLE_EQ(Core::Math::abs(-3.14159), 3.14159);
    EXPECT_FLOAT_EQ(Core::Math::abs(-2.718f), 2.718f);

    // 2. Math::sqrt zero and negative input branches
    EXPECT_DOUBLE_EQ(Core::Math::sqrt(0.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::Math::sqrt(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::Math::sqrt(-100.0), 0.0);
    EXPECT_FLOAT_EQ(Core::Math::sqrt(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Core::Math::sqrt(-4.0f), 0.0f);

    // 3. Math::acos float boundary overshoots (> 1 + tol and < -1 - tol)
    EXPECT_TRUE(std::isnan(Core::Math::acos(1.5f)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(-1.5f)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(2.0)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(-2.0)));

    // 4. Detail::BoundedNewtonSqrt NaN inputs (with and without iterations_out)
    std::size_t iters = 999;
    EXPECT_TRUE(std::isnan(Core::Detail::BoundedNewtonSqrt(std::numeric_limits<double>::quiet_NaN(), &iters)));
    EXPECT_EQ(iters, 0U);
    EXPECT_TRUE(std::isnan(Core::Detail::BoundedNewtonSqrt(std::numeric_limits<double>::quiet_NaN())));

    iters = 999;
    EXPECT_TRUE(std::isnan(Core::Detail::BoundedNewtonSqrt(std::numeric_limits<float>::quiet_NaN(), &iters)));
    EXPECT_EQ(iters, 0U);
    EXPECT_TRUE(std::isnan(Core::Detail::BoundedNewtonSqrt(std::numeric_limits<float>::quiet_NaN())));

    // 5. Detail::BoundedNewtonSqrt non-positive inputs (with and without iterations_out)
    iters = 999;
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(-1.0, &iters), 0.0);
    EXPECT_EQ(iters, 0U);
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(0.0, &iters), 0.0);
    EXPECT_EQ(iters, 0U);
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(0.0), 0.0);

    iters = 999;
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(-1.0f, &iters), 0.0f);
    EXPECT_EQ(iters, 0U);
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(0.0f, &iters), 0.0f);
    EXPECT_EQ(iters, 0U);
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(0.0f), 0.0f);

    // 6. Detail::BoundedNewtonSqrt positive infinity inputs (with and without iterations_out)
    const double inf_d = std::numeric_limits<double>::infinity();
    iters = 999;
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(inf_d, &iters), inf_d);
    EXPECT_EQ(iters, 0U);
    EXPECT_DOUBLE_EQ(Core::Detail::BoundedNewtonSqrt(inf_d), inf_d);

    const float inf_f = std::numeric_limits<float>::infinity();
    iters = 999;
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(inf_f, &iters), inf_f);
    EXPECT_EQ(iters, 0U);
    EXPECT_FLOAT_EQ(Core::Detail::BoundedNewtonSqrt(inf_f), inf_f);

    // 7. Detail::BoundedNewtonSqrt without iterations_out (nullptr branch on valid inputs)
    EXPECT_NEAR(Core::Detail::BoundedNewtonSqrt(9.0), 3.0, 1e-15);
    EXPECT_NEAR(Core::Detail::BoundedNewtonSqrt(9.0f), 3.0f, 1e-7f);

    // 8. Detail::BoundedNewtonSqrt float subnormal input
    const float denorm_min_f = std::numeric_limits<float>::denorm_min();
    iters = 0;
    const float denorm_res_f = Core::Detail::BoundedNewtonSqrt(denorm_min_f, &iters);
    EXPECT_FLOAT_EQ(denorm_res_f, std::sqrt(denorm_min_f));
    EXPECT_LE(iters, 64U);

    // 9. Float value triggering ULP oscillation in Newton iteration
    const float osc_f = 5.16958886e-26f;
    EXPECT_NEAR(Core::Detail::BoundedNewtonSqrt(osc_f), std::sqrt(osc_f), 1e-19f);

    // 10. Math functions: sin, cos, sqrt (float and double)
    EXPECT_DOUBLE_EQ(Core::Math::sin(0.0), 0.0);
    EXPECT_FLOAT_EQ(Core::Math::sin(0.0f), 0.0f);
    EXPECT_DOUBLE_EQ(Core::Math::cos(0.0), 1.0);
    EXPECT_FLOAT_EQ(Core::Math::cos(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(Core::Math::sqrt(4.0f), 2.0f);
}
