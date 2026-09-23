#include "Vectoris/Numerics/Core/Math.h"
#include "Vectoris/Numerics/Core/MathFunctions.h"
#include "Vectoris/Numerics/Geometry/UnitVector3.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace {
namespace core = vectoris::numerics::core;
template<class T> using Bits = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;

template<class T> constexpr auto boundary_inputs() {
    using L = std::numeric_limits<T>;
    return std::array<T, 23>{T{0}, -T{0}, T{1}, T{4}, T{0.125}, L::min(),
        L::denorm_min(), L::denorm_min()*T{2}, L::denorm_min()*T{3}, L::denorm_min()*T{17},
        L::min()-L::denorm_min(), L::max(), -T{1}, -T{0.125}, -L::min(),
        -L::denorm_min(), -L::max(), -L::infinity(), L::infinity(), L::quiet_NaN(),
        -L::quiet_NaN(), T{2}, T{9}};
}

template<class T, bool Compatibility> constexpr auto constant_boundaries() {
    auto values = boundary_inputs<T>();
    for (auto& value : values) {
        if constexpr (Compatibility) { value = core::Math::sqrt(value); }
        else { value = core::sqrt(value); }
    }
    return values;
}

template<class T> constexpr T bit_sample(std::uint64_t& state) {
    // Explicit deterministic test state; modulo arithmetic is intentional.
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    constexpr Bits<T> largest = std::bit_cast<Bits<T>>(std::numeric_limits<T>::max());
    const Bits<T> bits = static_cast<Bits<T>>(state) % largest + 1;
    return std::bit_cast<T>(bits);
}

template<class T, std::uint64_t Seed, bool Compatibility> constexpr auto constant_samples() {
    std::array<T, 64> values{};
    std::uint64_t state = Seed;
    for (auto& value : values) {
        const T input = bit_sample<T>(state);
        if constexpr (Compatibility) { value = core::Math::sqrt(input); }
        else { value = core::sqrt(input); }
    }
    return values;
}

struct OracleStats final {
    int comparisons = 0;
    long double worst_ulp = 0;
    void record() const {
        ::testing::Test::RecordProperty("oracle_comparisons", comparisons);
        ::testing::Test::RecordProperty("worst_ulp", std::to_string(static_cast<double>(worst_ulp)));
    }
};

template<class T> void check_positive(T input, T actual, OracleStats& stats) {
    ASSERT_TRUE(std::isfinite(actual));
    ASSERT_GT(actual, T{0});
    const T reference = std::sqrt(input); // Independent libm oracle, not Vectoris code.
    const T ulp = std::nextafter(reference, std::numeric_limits<T>::infinity()) - reference;
    const T error = std::abs(actual - reference);
    EXPECT_LE(error, ulp) << "input=" << input << " actual=" << actual << " reference=" << reference;
    stats.worst_ulp = std::max(stats.worst_ulp, static_cast<long double>(error / ulp));
    ++stats.comparisons;
}

template<class T> void check_domain(T input, T actual) {
    if (std::isnan(input) || input < T{0}) {
        EXPECT_TRUE(std::isnan(actual));
    } else if (input == T{0}) { // Exact zero classification, including its sign.
        EXPECT_EQ(std::fpclassify(actual), FP_ZERO);
        EXPECT_EQ(std::signbit(actual), std::signbit(input));
    } else {
        EXPECT_TRUE(std::isinf(actual));
        EXPECT_FALSE(std::signbit(actual));
    }
}

template<class T> T runtime_core(T value) {
    volatile T input = value; // Force evaluation outside constant evaluation.
    return core::sqrt(input);
}
template<class T> T runtime_compatibility(T value) {
    volatile T input = value;
    return core::Math::sqrt(input);
}

template<class T, std::uint64_t Seed> void check_constant_samples(OracleStats& stats) {
    constexpr auto canonical = constant_samples<T, Seed, false>();
    constexpr auto compatibility = constant_samples<T, Seed, true>();
    std::uint64_t state = Seed;
    for (std::size_t i = 0; i < canonical.size(); ++i) {
        const T input = bit_sample<T>(state);
        check_positive(input, canonical[i], stats);
        check_positive(input, compatibility[i], stats);
    }
}

template<class T> concept CanCoreSqrt = requires(T value) { core::sqrt(value); };
template<class T> concept CanCompatibilitySqrt = requires(T value) { core::Math::sqrt(value); };
struct ConvertibleToDouble { operator double() const { return 4.0; } };
struct Frame {};
} // namespace

template<class T> class SqrtRuntimeTest : public ::testing::Test {};
using RuntimeScalars = ::testing::Types<float, double, long double>;
TYPED_TEST_SUITE(SqrtRuntimeTest, RuntimeScalars);

TYPED_TEST(SqrtRuntimeTest, NegativeDomainAndSignedZero) {
    using T = TypeParam;
    using L = std::numeric_limits<T>;
    constexpr std::array<T, 7> values{T{0}, -T{0}, -L::denorm_min(), -L::min(), -T{1}, -L::max(), -L::infinity()};
    for (T input : values) {
        check_domain(input, runtime_core(input));
        check_domain(input, runtime_compatibility(input));
    }
}

TYPED_TEST(SqrtRuntimeTest, NaNAndInfinity) {
    using T = TypeParam;
    using L = std::numeric_limits<T>;
    constexpr std::array<T, 4> values{L::quiet_NaN(), -L::quiet_NaN(), L::infinity(), -L::infinity()};
    for (T input : values) {
        check_domain(input, runtime_core(input));
        check_domain(input, runtime_compatibility(input));
    }
}

TYPED_TEST(SqrtRuntimeTest, BoundaryAndSubnormalOracle) {
    using T = TypeParam;
    ASSERT_EQ(std::fegetround(), FE_TONEAREST);
    OracleStats stats;
    for (T input : boundary_inputs<T>()) {
        if (input > T{0} && std::isfinite(input)) {
            check_positive(input, runtime_core(input), stats);
            check_positive(input, runtime_compatibility(input), stats);
        }
    }
    stats.record();
}

TYPED_TEST(SqrtRuntimeTest, QualifiedCompatibilityPathsDoNotRecurse) {
    using T = TypeParam;
    static_assert(std::is_same_v<decltype(core::sqrt(T{4})), T>);
    static_assert(std::is_same_v<decltype(core::Math::sqrt(T{4})), T>);
    static_assert(noexcept(core::sqrt(T{4})) && noexcept(core::Math::sqrt(T{4})));
    static_assert(&core::sqrt<T> == &vectoris::numerics::Core::sqrt<T>);
    static_assert(&core::Math::sqrt<T> == &vectoris::numerics::Core::Math::sqrt<T>);
    OracleStats stats;
    volatile T input = T{4};
    check_positive(T{4}, vectoris::numerics::Core::sqrt(input), stats);
    check_positive(T{4}, vectoris::numerics::Core::Math::sqrt(input), stats);
    stats.record();
}

template<class T> class SqrtConstexprTest : public ::testing::Test {};
using BinaryScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(SqrtConstexprTest, BinaryScalars);

TYPED_TEST(SqrtConstexprTest, NegativeDomainSignedZeroAndNonFinite) {
    using T = TypeParam;
    constexpr auto inputs = boundary_inputs<T>();
    constexpr auto canonical = constant_boundaries<T, false>();
    constexpr auto compatibility = constant_boundaries<T, true>();
    static_assert(std::bit_cast<Bits<T>>(core::sqrt(-T{0})) == std::bit_cast<Bits<T>>(-T{0}));
    static_assert(vectoris::numerics::Traits::IsNaN(core::sqrt(-T{1})));
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (!(inputs[i] > T{0}) || !std::isfinite(inputs[i])) {
            check_domain(inputs[i], canonical[i]);
            check_domain(inputs[i], compatibility[i]);
        }
    }
}

TYPED_TEST(SqrtConstexprTest, BoundaryAndSubnormalOracle) {
    using T = TypeParam;
    constexpr auto inputs = boundary_inputs<T>();
    constexpr auto canonical = constant_boundaries<T, false>();
    constexpr auto compatibility = constant_boundaries<T, true>();
    OracleStats stats;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i] > T{0} && std::isfinite(inputs[i])) {
            check_positive(inputs[i], canonical[i], stats);
            check_positive(inputs[i], compatibility[i], stats);
        }
    }
    stats.record();
}

TYPED_TEST(SqrtConstexprTest, DeterministicPositiveBitSamples) {
    OracleStats stats;
    check_constant_samples<TypeParam, 0x123456789abcdef0ULL>(stats);
    check_constant_samples<TypeParam, 0x9e3779b97f4a7c15ULL>(stats);
    check_constant_samples<TypeParam, 0x243f6a8885a308d3ULL>(stats);
    check_constant_samples<TypeParam, 0x13198a2e03707344ULL>(stats);
    stats.record();
}

template<class T> class SqrtPropertyTest : public ::testing::Test {};
TYPED_TEST_SUITE(SqrtPropertyTest, BinaryScalars);

TYPED_TEST(SqrtPropertyTest, EveryPowerOfTwoAndAdjacentValues) {
    using T = TypeParam;
    using L = std::numeric_limits<T>;
    OracleStats stats;
    constexpr int first = L::min_exponent - L::digits;
    constexpr int max_iterations = L::max_exponent - first;
    for (int i = 0; i < max_iterations; ++i) {
        const T power = std::ldexp(T{1}, first + i);
        for (T input : {std::nextafter(power, T{0}), power, std::nextafter(power, L::infinity())}) {
            if (input > T{0} && std::isfinite(input)) {
                check_positive(input, runtime_core(input), stats);
                check_positive(input, runtime_compatibility(input), stats);
                check_positive(input, core::Detail::DigitSqrtPositive(input), stats);
            }
        }
    }
    stats.record();
}

TYPED_TEST(SqrtPropertyTest, DeterministicRuntimeBitSamples) {
    using T = TypeParam;
    OracleStats stats;
    std::uint64_t state = 0xfeedfacecafebeefULL;
    constexpr int max_iterations = 32768;
    for (int i = 0; i < max_iterations; ++i) {
        const T input = bit_sample<T>(state);
        check_positive(input, runtime_core(input), stats);
        check_positive(input, runtime_compatibility(input), stats);
        // Exercise the constant-evaluation algorithm dynamically under sanitizers.
        check_positive(input, core::Detail::DigitSqrtPositive(input), stats);
    }
    stats.record();
}

TYPED_TEST(SqrtPropertyTest, FormerNewtonOscillationInputs) {
    using T = TypeParam;
    const T input = [] {
        if constexpr (std::is_same_v<T, float>) { return 5.16958886e-26f; }
        else { return 8.90029543402880454e-308; }
    }();
    OracleStats stats;
    check_positive(input, runtime_core(input), stats);
    check_positive(input, core::Detail::DigitSqrtPositive(input), stats);
    stats.record();
}

TEST(SqrtTypeTest, SupportedAndRejectedArgumentTypes) {
    static_assert(CanCoreSqrt<float> && CanCoreSqrt<double> && CanCoreSqrt<long double>);
    static_assert(CanCompatibilitySqrt<float> && CanCompatibilitySqrt<double> && CanCompatibilitySqrt<long double>);
    static_assert(CanCoreSqrt<const float> && CanCompatibilitySqrt<const double>);
    static_assert(!CanCoreSqrt<int> && !CanCoreSqrt<unsigned> && !CanCoreSqrt<std::int64_t>);
    static_assert(!CanCompatibilitySqrt<int> && !CanCompatibilitySqrt<unsigned> && !CanCompatibilitySqrt<std::int64_t>);
    static_assert(!CanCoreSqrt<bool> && !CanCompatibilitySqrt<bool>);
    static_assert(!CanCoreSqrt<ConvertibleToDouble> && !CanCompatibilitySqrt<ConvertibleToDouble>);
    EXPECT_TRUE(CanCoreSqrt<long double>);
}

TEST(SqrtLongDoubleTest, DeterministicExponentSamples) {
    using T = long double;
    using L = std::numeric_limits<T>;
    OracleStats stats;
    std::uint64_t state = 0x3c6ef372fe94f82bULL;
    constexpr int first = L::min_exponent - L::digits;
    constexpr int exponent_count = L::max_exponent - first;
    constexpr int max_iterations = 2048;
    for (int i = 0; i < max_iterations; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        const int exponent = first + static_cast<int>(state % static_cast<std::uint64_t>(exponent_count));
        const T mantissa = T{1} + static_cast<T>(state & 0xffffU) / T{65536};
        const T input = std::ldexp(mantissa, exponent);
        ASSERT_GT(input, T{0});
        ASSERT_TRUE(std::isfinite(input));
        check_positive(input, runtime_core(input), stats);
        check_positive(input, runtime_compatibility(input), stats);
    }
    stats.record();
}

TEST(SqrtGeometryCompatibilityTest, LongDoubleNormalizationRemainsSupported) {
    namespace geometry = vectoris::numerics::geometry;
    const auto unit = geometry::UnitVector3<long double, Frame>::TryCreate(
        geometry::Vector3<long double, Frame>{1.0L, -2.0L, 3.0L});
    ASSERT_TRUE(unit.has_value());
    EXPECT_TRUE(unit.value().IsValid());
    const auto narrowed = geometry::UnitVector3<double, Frame>::TryCreate(
        geometry::Vector3<long double, Frame>{1.0L, -2.0L, 3.0L});
    ASSERT_TRUE(narrowed.has_value());
    EXPECT_TRUE(narrowed.value().IsValid());
    const auto quaternion = geometry::Quaternion<long double, Frame, Frame>::TryCreate(1.0L, 2.0L, 3.0L, 4.0L);
    ASSERT_TRUE(quaternion.has_value());
    const auto& q = quaternion.value();
    const long double norm = std::hypot(std::hypot(q.w, q.x), std::hypot(q.y, q.z));
    EXPECT_LE(std::abs(norm - 1.0L), 8 * std::numeric_limits<long double>::epsilon());
}
