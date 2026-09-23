#include <gtest/gtest.h>
#include <concepts>
#include <type_traits>
#include <utility>
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Units/Quantity.h"
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"

namespace {
namespace G = vectoris::numerics::Geometry;
namespace U = vectoris::numerics::Units;
struct MatrixWorldFrame {};
struct MatrixBodyFrame {};
struct UnsupportedScalar {};

template <typename A, typename B>
concept Multipliable = requires(const A& a, const B& b) { a * b; };
template <typename A, typename B>
concept Addable = requires(const A& a, const B& b) { a + b; };

template <typename T, typename F>
constexpr bool multiplication_contract() {
    using M = G::Matrix3<T>;
    using V = G::Vector3<T, F>;
    static_assert(std::same_as<decltype(M{} * V{}), V>);
    static_assert(std::same_as<decltype(M{} * M{}), M>);
    static_assert(std::same_as<decltype(M{} * T{}), M>);
    static_assert(std::same_as<decltype(T{} * M{}), M>);
    static_assert(noexcept(M{} * V{}));
    static_assert(noexcept(M{} * M{}));
    static_assert(noexcept(M{} * T{}));
    static_assert(noexcept(T{} * M{}));
    static_assert(!G::ScalarArithmetic<M>);
    static_assert(!G::ScalarArithmetic<const V&>);
    static_assert(!Multipliable<M, UnsupportedScalar>);
    static_assert(!Multipliable<UnsupportedScalar, M>);
    static_assert(!Multipliable<M, G::Point3<T, F>>);
    static_assert(!Multipliable<V, M>);
    static_assert(!Multipliable<V, V>);
    static_assert(!std::is_convertible_v<V, G::Vector3<T, G::FrameUnknown>>);
    constexpr M matrix{T{1}, T{2}, T{3}, T{4}, T{5}, T{6}, T{7}, T{8}, T{9}};
    constexpr auto result = matrix * V{T{1}, T{2}, T{3}};
    // Exactly representable integer-valued arithmetic; zero tolerance oracle.
    return result.x == T{14} && result.y == T{32} && result.z == T{50};
}

static_assert(multiplication_contract<float, MatrixWorldFrame>());
static_assert(multiplication_contract<double, MatrixWorldFrame>());
static_assert(multiplication_contract<float, MatrixBodyFrame>());
static_assert(multiplication_contract<double, MatrixBodyFrame>());
static_assert(!Multipliable<G::Matrix3<float>, G::Matrix3<double>>);
static_assert(!Multipliable<G::Matrix3<double>, G::Matrix3<float>>);
static_assert(!Addable<G::Vector3<double, MatrixWorldFrame>, G::Vector3<double, MatrixBodyFrame>>);
static_assert(std::same_as<decltype(G::Matrix3<float>{} * G::Vector3<double, MatrixBodyFrame>{}),
                           G::Vector3<double, MatrixBodyFrame>>);
static_assert(std::same_as<decltype(G::Matrix3<double>{} * G::Vector3<float, MatrixWorldFrame>{}),
                           G::Vector3<double, MatrixWorldFrame>>);
using Length = U::Quantity<double, U::MeterUnit>;
static_assert(G::ScalarArithmetic<Length>);
static_assert(std::same_as<decltype(G::Matrix3<double>{} * std::declval<G::Vector3<Length, MatrixWorldFrame>>()),
                           G::Vector3<Length, MatrixWorldFrame>>);
static_assert(!std::is_convertible_v<G::Vector3<Length, MatrixWorldFrame>, G::Vector3<double, MatrixWorldFrame>>);

template <typename T>
class Matrix3PublicMultiplicationTest : public ::testing::Test {};
using MatrixScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(Matrix3PublicMultiplicationTest, MatrixScalars);

template <typename T, typename F>
void expect_vector(const G::Vector3<T, F>& actual, T x, T y, T z) {
    EXPECT_NEAR(actual.x, x, T{0});
    EXPECT_NEAR(actual.y, y, T{0});
    EXPECT_NEAR(actual.z, z, T{0});
}

TYPED_TEST(Matrix3PublicMultiplicationTest, IdentityZeroAndTwoFrames) {
    using T = TypeParam;
    using M = G::Matrix3<T>;
    const G::Vector3<T, MatrixWorldFrame> world{T{1}, T{-2}, T{3}};
    const G::Vector3<T, MatrixBodyFrame> body{T{-4}, T{5}, T{6}};
    expect_vector(M::Identity() * world, T{1}, T{-2}, T{3});
    expect_vector(M::Zero() * world, T{0}, T{0}, T{0});
    expect_vector(M::Identity() * body, T{-4}, T{5}, T{6});
    expect_vector(M::Zero() * body, T{0}, T{0}, T{0});
}

TYPED_TEST(Matrix3PublicMultiplicationTest, RowMajorIndependentOracle) {
    using T = TypeParam;
    const G::Matrix3<T> matrix{T{1}, T{2}, T{3}, T{4}, T{5}, T{6}, T{7}, T{8}, T{9}};
    expect_vector(matrix * G::Vector3<T, MatrixWorldFrame>{T{1}, T{2}, T{3}}, T{14}, T{32}, T{50});
    expect_vector(matrix * G::Vector3<T, MatrixBodyFrame>{T{1}, T{2}, T{3}}, T{14}, T{32}, T{50});
}

TYPED_TEST(Matrix3PublicMultiplicationTest, DiagonalAndSignedOracle) {
    using T = TypeParam;
    const G::Matrix3<T> diagonal{T{2}, T{0}, T{0}, T{0}, T{-3}, T{0}, T{0}, T{0}, T{4}};
    const G::Matrix3<T> signed_matrix{T{1}, T{-2}, T{3}, T{-4}, T{5}, T{-6}, T{7}, T{-8}, T{9}};
    const G::Vector3<T, MatrixWorldFrame> vector{T{-1}, T{2}, T{-3}};
    expect_vector(diagonal * vector, T{-2}, T{-6}, T{-12});
    expect_vector(signed_matrix * vector, T{-14}, T{32}, T{-50});
}

TYPED_TEST(Matrix3PublicMultiplicationTest, MatrixAndBothScalarOrders) {
    using T = TypeParam;
    using M = G::Matrix3<T>;
    const M matrix{T{1}, T{2}, T{3}, T{4}, T{5}, T{6}, T{7}, T{8}, T{9}};
    const M squared{T{30}, T{36}, T{42}, T{66}, T{81}, T{96}, T{102}, T{126}, T{150}};
    const M scaled{T{-2}, T{-4}, T{-6}, T{-8}, T{-10}, T{-12}, T{-14}, T{-16}, T{-18}};
    EXPECT_TRUE((matrix * matrix).AlmostEqual(squared, T{0}, T{0}));
    EXPECT_TRUE((matrix * T{-2}).AlmostEqual(scaled, T{0}, T{0}));
    EXPECT_TRUE((T{-2} * matrix).AlmostEqual(scaled, T{0}, T{0}));
    EXPECT_TRUE((matrix * M::Identity()).AlmostEqual(matrix, T{0}, T{0}));
    EXPECT_TRUE((M::Identity() * matrix).AlmostEqual(matrix, T{0}, T{0}));
}

TEST(Matrix3MixedMultiplicationTest, PreservesUsualArithmeticPromotion) {
    const auto wide_vector = G::Matrix3<float>::Identity() * G::Vector3<double, MatrixWorldFrame>{1.0, -2.0, 3.0};
    const auto narrow_vector = G::Matrix3<double>::Identity() * G::Vector3<float, MatrixBodyFrame>{1.0F, -2.0F, 3.0F};
    expect_vector(wide_vector, 1.0, -2.0, 3.0);
    expect_vector(narrow_vector, 1.0, -2.0, 3.0);
    static_assert(std::same_as<decltype(G::Matrix3<float>{} * 2.0), G::Matrix3<double>>);
    static_assert(std::same_as<decltype(2.0 * G::Matrix3<float>{}), G::Matrix3<double>>);
    const auto expected = G::Matrix3<double>::Identity() * 2.0;
    EXPECT_TRUE((G::Matrix3<float>::Identity() * 2.0).AlmostEqual(expected, 0.0, 0.0));
    EXPECT_TRUE((2.0 * G::Matrix3<float>::Identity()).AlmostEqual(expected, 0.0, 0.0));
}
TEST(Matrix3MixedMultiplicationTest, PreservesUnitValuedComponents) {
    const G::Matrix3<double> matrix{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    const G::Vector3<Length, MatrixWorldFrame> vector{Length{1.0}, Length{2.0}, Length{3.0}};
    const auto result = matrix * vector;
    EXPECT_NEAR(result.x.value(), 14.0, 0.0);
    EXPECT_NEAR(result.y.value(), 32.0, 0.0);
    EXPECT_NEAR(result.z.value(), 50.0, 0.0);
}
} // namespace
