#include <gtest/gtest.h>
#include <concepts>
#include <type_traits>
#include <utility>
#include <array>
#include <cmath>
#include <limits>
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/RotationMatrix3.h"
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

namespace {
struct C13RotationFrame {};
template<class T> using C13Vector = G::Vector3<T,C13RotationFrame>;
template<class T> using C13Rotation = G::RotationMatrix3<T,C13RotationFrame,C13RotationFrame>;

template<class T> void C13ExpectNaN(const C13Vector<T>& v) {
    EXPECT_TRUE(std::isnan(v.x)); EXPECT_TRUE(std::isnan(v.y)); EXPECT_TRUE(std::isnan(v.z));
}
template<class T> void C13NaNPositions() {
    const T nan=std::numeric_limits<T>::quiet_NaN();
    const auto r=C13Rotation<T>::Identity();
    for(std::size_t axis=0;axis<3;++axis) {
        std::array<T,3> components{T{1},T{2},T{3}};components[axis]=nan;
        const C13Vector<T> v{components[0],components[1],components[2]};
        SCOPED_TRACE(axis);C13ExpectNaN(r*v); // Every row includes a product with NaN.
    }
}
template<class T> void C13MixedNaN() {
    const T nan=std::numeric_limits<T>::quiet_NaN(),inf=std::numeric_limits<T>::infinity();
    const T max=std::numeric_limits<T>::max();const auto r=C13Rotation<T>::Identity();
    const std::array<C13Vector<T>,5> inputs{{{nan,nan,nan},{T{0},nan,-T{0}},
        {max,-max,nan},{inf,nan,-inf},{nan,T{0},T{0}}}};
    for(const auto& v:inputs) C13ExpectNaN(r*v);
}
template<class T> void C13InfinityRows() {
    const auto r=C13Rotation<T>::Identity();
    for(const T inf:{std::numeric_limits<T>::infinity(),-std::numeric_limits<T>::infinity()}) {
        for(std::size_t axis=0;axis<3;++axis) {
            std::array<T,3> components{T{1},T{2},T{3}};components[axis]=inf;
            const auto out=r*C13Vector<T>{components[0],components[1],components[2]};
            const std::array<T,3> values{out.x,out.y,out.z};
            for(std::size_t row=0;row<3;++row) {
                SCOPED_TRACE(row);
                if(row==axis) {EXPECT_TRUE(std::isinf(values[row]));EXPECT_EQ(std::signbit(values[row]),std::signbit(inf));}
                else EXPECT_TRUE(std::isnan(values[row])); // The off-axis row contains 0 * Inf.
            }
        }
    }
}
template<class T> void C13InfinityCancellation() {
    // Exact quarter-turn: x'=-y, y'=x, z'=z. Rows still evaluate zero products.
    const G::Matrix3<T> raw{T{0},T{-1},T{0},T{1},T{0},T{0},T{0},T{0},T{1}};
    const auto result=C13Rotation<T>::TryCreate(raw);ASSERT_TRUE(result.IsSuccess());
    const T inf=std::numeric_limits<T>::infinity();
    C13ExpectNaN(result.Value()*C13Vector<T>{inf,-inf,T{0}});
    // General rotation has negative diagonals and positive off-diagonals.
    const G::Matrix3<T> general{T{-1}/T{3},T{2}/T{3},T{2}/T{3},
        T{2}/T{3},T{-1}/T{3},T{2}/T{3},T{2}/T{3},T{2}/T{3},T{-1}/T{3}};
    const auto rotation=C13Rotation<T>::TryCreate(general);ASSERT_TRUE(rotation.IsSuccess());
    const auto same=rotation.Value()*C13Vector<T>{inf,inf,T{0}};
    EXPECT_TRUE(std::isnan(same.x));EXPECT_TRUE(std::isnan(same.y));
    EXPECT_TRUE(std::isinf(same.z));EXPECT_FALSE(std::signbit(same.z));
    const auto opposite=rotation.Value()*C13Vector<T>{inf,-inf,T{0}};
    EXPECT_TRUE(std::isinf(opposite.x));EXPECT_TRUE(std::signbit(opposite.x));
    EXPECT_TRUE(std::isinf(opposite.y));EXPECT_FALSE(std::signbit(opposite.y));
    EXPECT_TRUE(std::isnan(opposite.z));
}
template<class T> void C13SignedZeros() {
    const auto r=C13Rotation<T>::Identity();
    for(unsigned mask=0;mask<8;++mask) {
        const C13Vector<T> v{mask&1U?-T{0}:T{0},mask&2U?-T{0}:T{0},mask&4U?-T{0}:T{0}};
        const auto out=r*v;
        const std::array<T,3> reference{{(v.x+T{0}*v.y)+T{0}*v.z,
            (T{0}*v.x+v.y)+T{0}*v.z,(T{0}*v.x+T{0}*v.y)+v.z}};
        const std::array<T,3> values{out.x,out.y,out.z};
        for(std::size_t i=0;i<3;++i) {EXPECT_NEAR(values[i],T{0},T{0});EXPECT_EQ(std::signbit(values[i]),std::signbit(reference[i]));}
    }
}
template<class T> void C13FiniteBoundaryControls() {
    const auto r=C13Rotation<T>::Identity();
    for(const T scale:{std::numeric_limits<T>::denorm_min(),std::numeric_limits<T>::min(),
        T{1},std::numeric_limits<T>::max()}) {
        const auto out=r*C13Vector<T>{scale,-scale,scale};
        EXPECT_TRUE(std::isfinite(out.x));EXPECT_TRUE(std::isfinite(out.y));EXPECT_TRUE(std::isfinite(out.z));
        EXPECT_NEAR(out.x,scale,T{0});EXPECT_NEAR(out.y,-scale,T{0});EXPECT_NEAR(out.z,scale,T{0});
    }
}
}
TEST(C13RotationNonFinite,FloatNaNPositions){C13NaNPositions<float>();}
TEST(C13RotationNonFinite,DoubleNaNPositions){C13NaNPositions<double>();}
TEST(C13RotationNonFinite,FloatMixedNaN){C13MixedNaN<float>();}
TEST(C13RotationNonFinite,DoubleMixedNaN){C13MixedNaN<double>();}
TEST(C13RotationNonFinite,FloatInfinityRows){C13InfinityRows<float>();}
TEST(C13RotationNonFinite,DoubleInfinityRows){C13InfinityRows<double>();}
TEST(C13RotationNonFinite,FloatInfinityCancellation){C13InfinityCancellation<float>();}
TEST(C13RotationNonFinite,DoubleInfinityCancellation){C13InfinityCancellation<double>();}
TEST(C13RotationNonFinite,FloatSignedZeros){C13SignedZeros<float>();}
TEST(C13RotationNonFinite,DoubleSignedZeros){C13SignedZeros<double>();}
TEST(C13RotationNonFinite,FloatFiniteBoundaryControls){C13FiniteBoundaryControls<float>();}
TEST(C13RotationNonFinite,DoubleFiniteBoundaryControls){C13FiniteBoundaryControls<double>();}

namespace {
template<class T> void C13CheckedConstructionNonFinite() {
    const auto identity=G::Matrix3<T>::Identity();
    auto unchanged=C13Rotation<T>::Identity();
    for(const T invalid:{std::numeric_limits<T>::quiet_NaN(),std::numeric_limits<T>::infinity(),
                         -std::numeric_limits<T>::infinity()}) {
        for(std::size_t component=0;component<9;++component) {
            auto raw=identity;raw.m[component]=invalid;
            const auto result=C13Rotation<T>::TryCreate(raw);
            ASSERT_FALSE(result.IsSuccess());
            EXPECT_EQ(result.error(),vectoris::numerics::core::MathError::non_finite_input);
            EXPECT_FALSE(C13Rotation<T>::TryCreate(raw,unchanged));
            EXPECT_TRUE(unchanged.ToMatrix().AlmostEqual(identity));
        }
    }
    EXPECT_TRUE(C13Rotation<T>::TryCreate(identity,unchanged));
    EXPECT_TRUE(unchanged.ToMatrix().AlmostEqual(identity));
}
}
TEST(C13RotationNonFinite,FloatCheckedConstructionRejectsNonFinite){C13CheckedConstructionNonFinite<float>();}
TEST(C13RotationNonFinite,DoubleCheckedConstructionRejectsNonFinite){C13CheckedConstructionNonFinite<double>();}

namespace {
template<class Coefficient,class Component> void C13MixedPrecisionNonFinite() {
    const auto r=C13Rotation<Coefficient>::Identity();
    for(std::size_t axis=0;axis<3;++axis) {
        std::array<Component,3> values{Component{1},Component{2},Component{3}};
        values[axis]=std::numeric_limits<Component>::quiet_NaN();
        const auto out=r*C13Vector<Component>{values[0],values[1],values[2]};
        static_assert(std::same_as<std::remove_cvref_t<decltype(out)>,C13Vector<double>>);
        C13ExpectNaN(out);
    }
    const auto inf=r*C13Vector<Component>{std::numeric_limits<Component>::infinity(),Component{1},Component{2}};
    EXPECT_TRUE(std::isinf(inf.x));EXPECT_FALSE(std::signbit(inf.x));
    EXPECT_TRUE(std::isnan(inf.y));EXPECT_TRUE(std::isnan(inf.z));
    const Component max=std::numeric_limits<Component>::max();
    const auto finite=r*C13Vector<Component>{max,-max,max};
    EXPECT_NEAR(finite.x,static_cast<double>(max),0.);
    EXPECT_NEAR(finite.y,-static_cast<double>(max),0.);
    EXPECT_NEAR(finite.z,static_cast<double>(max),0.);
}
}
TEST(C13RotationNonFinite,FloatRotationDoubleComponents){C13MixedPrecisionNonFinite<float,double>();}
TEST(C13RotationNonFinite,DoubleRotationFloatComponents){C13MixedPrecisionNonFinite<double,float>();}
