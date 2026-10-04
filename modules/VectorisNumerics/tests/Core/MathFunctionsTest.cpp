#include <gtest/gtest.h>
#include <cmath>
#include <numbers>
#include "Vectoris/Numerics/Core/Math.h"
#include "Vectoris/Numerics/Core/MathFunctions.h"
#include "Vectoris/Numerics/Core/Constants.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"

using namespace vectoris::numerics;
using namespace vectoris::numerics::Core::Math;

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

// sqrt contract regressions are in SqrtContractTest.cpp (VRT-14).
TEST(MathFunctionsTest, AdditionalRuntimeWrappers) {
    // Keep direct runtime coverage of the helper formerly exercised by Newton sqrt.
    volatile double runtime_input = -5.5;
    EXPECT_DOUBLE_EQ(Core::abs(runtime_input), 5.5);
    runtime_input = 5.5;
    EXPECT_DOUBLE_EQ(Core::abs(runtime_input), 5.5);
    runtime_input = 0.0;
    EXPECT_DOUBLE_EQ(Core::abs(runtime_input), 0.0);

    // 1. Math::abs negative branch
    EXPECT_EQ(Core::Math::abs(-42), 42u);
    EXPECT_DOUBLE_EQ(Core::Math::abs(-3.14159), 3.14159);
    EXPECT_FLOAT_EQ(Core::Math::abs(-2.718f), 2.718f);

    // 3. Math::acos float boundary overshoots (> 1 + tol and < -1 - tol)
    EXPECT_TRUE(std::isnan(Core::Math::acos(1.5f)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(-1.5f)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(2.0)));
    EXPECT_TRUE(std::isnan(Core::Math::acos(-2.0)));

    // Math functions: sin and cos (float and double)
    EXPECT_DOUBLE_EQ(Core::Math::sin(0.0), 0.0);
    EXPECT_FLOAT_EQ(Core::Math::sin(0.0f), 0.0f);
    EXPECT_DOUBLE_EQ(Core::Math::cos(0.0), 1.0);
    EXPECT_FLOAT_EQ(Core::Math::cos(0.0f), 1.0f);
}

namespace {
template <typename T>
concept HasCoreAbs = requires(T value) { Core::abs(value); Core::Math::abs(value); };

// Independent oracle: shift negative values one step toward zero before negation.
template <typename T>
constexpr std::make_unsigned_t<T> SignedMagnitudeReference(T value) noexcept {
    using UInt = std::make_unsigned_t<T>;
    return value < T{0}
        ? static_cast<UInt>(static_cast<UInt>(-(value + T{1})) + UInt{1})
        : static_cast<UInt>(value);
}

template <typename T>
class CoreAbsSigned : public ::testing::Test {};
using AbsSignedTypes = ::testing::Types<signed char, short, int, long, long long>;
TYPED_TEST_SUITE(CoreAbsSigned, AbsSignedTypes);
TYPED_TEST(CoreAbsSigned, SignedBoundaryMagnitude) {
    using T = TypeParam;
    using UInt = std::make_unsigned_t<T>;
    constexpr T minimum = std::numeric_limits<T>::min();
    constexpr T maximum = std::numeric_limits<T>::max();
    static_assert(std::same_as<decltype(Core::abs(T{})), UInt>);
    static_assert(std::same_as<decltype(Core::Math::abs(T{})), UInt>);
    static_assert(noexcept(Core::abs(T{})) && noexcept(Core::Math::abs(T{})));
    static_assert(Core::abs(minimum) == static_cast<UInt>(static_cast<UInt>(maximum) + UInt{1}));
    static_assert(Core::Math::abs(minimum) == Core::abs(minimum));
    static_assert(Core::abs(T{-2}) == UInt{2} && Core::abs(T{2}) == UInt{2});
    static_assert(Core::abs(T{0}) == UInt{0});
    const T inputs[] = {minimum, static_cast<T>(minimum + T{1}), T{-2}, T{-1},
                        T{0}, T{1}, T{2}, static_cast<T>(maximum - T{1}), maximum};
    for (const T input : inputs) {
        volatile T runtime_input = input;
        const UInt expected = SignedMagnitudeReference(input);
        EXPECT_EQ(Core::abs(runtime_input), expected);
        EXPECT_EQ(Core::Math::abs(runtime_input), expected);
    }
}

template <typename T>
class CoreAbsUnsigned : public ::testing::Test {};
using AbsUnsignedTypes = ::testing::Types<unsigned char, unsigned short, unsigned int,
                                         unsigned long, unsigned long long>;
TYPED_TEST_SUITE(CoreAbsUnsigned, AbsUnsignedTypes);
TYPED_TEST(CoreAbsUnsigned, UnsignedIdentity) {
    using T = TypeParam;
    static_assert(std::same_as<decltype(Core::abs(T{})), T>);
    static_assert(std::same_as<decltype(Core::Math::abs(T{})), T>);
    static_assert(Core::abs(std::numeric_limits<T>::max()) == std::numeric_limits<T>::max());
    for (const T input : {T{0}, T{1}, std::numeric_limits<T>::max()}) {
        volatile T runtime_input = input;
        EXPECT_EQ(Core::abs(runtime_input), input);
        EXPECT_EQ(Core::Math::abs(runtime_input), input);
    }
}

template <typename T>
class CoreAbsFloating : public ::testing::Test {};
using AbsFloatingTypes = ::testing::Types<float, double, long double>;
TYPED_TEST_SUITE(CoreAbsFloating, AbsFloatingTypes);
TYPED_TEST(CoreAbsFloating, FloatingContractPreserved) {
    using T = TypeParam;
    static_assert(std::same_as<decltype(Core::abs(T{})), T>);
    static_assert(Core::abs(T{-2}) == T{2} && Core::Math::abs(T{-2}) == T{2});
    for (const T input : {T{-2}, T{0}, -T{0}, T{2}, std::numeric_limits<T>::max(),
                          -std::numeric_limits<T>::max(), std::numeric_limits<T>::denorm_min(),
                          -std::numeric_limits<T>::denorm_min(), std::numeric_limits<T>::infinity(),
                          -std::numeric_limits<T>::infinity(), std::numeric_limits<T>::quiet_NaN()}) {
        volatile T runtime_input = input;
        const T primitive = Core::abs(runtime_input);
        const T wrapper = Core::Math::abs(runtime_input);
        if (std::isnan(input)) {
            EXPECT_TRUE(std::isnan(primitive));
            EXPECT_TRUE(std::isnan(wrapper));
        } else if (input == T{0}) { // Exact signed-zero classification, not a numerical comparison.
            EXPECT_EQ(std::signbit(primitive), std::signbit(input));
            EXPECT_EQ(std::signbit(wrapper), std::signbit(input));
        } else {
            EXPECT_EQ(primitive, std::fabs(input)); // Exact magnitude, no arithmetic rounding.
            EXPECT_EQ(wrapper, std::fabs(input));
        }
    }
}

TEST(CoreAbsContract, NonNumericIntegralTypesRejected) {
    static_assert(!HasCoreAbs<bool> && !HasCoreAbs<char> && !HasCoreAbs<wchar_t>);
    static_assert(!HasCoreAbs<char8_t> && !HasCoreAbs<char16_t> && !HasCoreAbs<char32_t>);
    EXPECT_FALSE(HasCoreAbs<bool>);
    EXPECT_FALSE(HasCoreAbs<char>);
}
} // namespace
