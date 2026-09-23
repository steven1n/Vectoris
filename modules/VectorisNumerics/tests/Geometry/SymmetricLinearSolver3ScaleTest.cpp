#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include "Vectoris/Numerics/Geometry/SymmetricLinearSolver3.h"

namespace {
namespace G = vectoris::numerics::Geometry;
using Error = vectoris::numerics::Core::MathError;
struct SolveScaleFrame {};
template <typename T> using V = G::Vector3<T, SolveScaleFrame>;
template <typename T> using M = G::Matrix3<T>;
template <typename T> class LDLTScaleTest : public ::testing::Test {};
using SolveScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(LDLTScaleTest, SolveScalars);

// Independent long-double, power-of-two normalized original-equation oracle.
// Each term is bounded even when long double has the same range as double.
template <typename T>
long double oracle_eta(const M<T>& a, const V<T>& b, const V<T>& x) {
    long double an = 0;
    for (T v : a.m) an = std::max(an, std::abs(static_cast<long double>(v)));
    const std::array<long double, 3> bv{b.x, b.y, b.z}, xv{x.x, x.y, x.z};
    const long double bn = std::max({std::abs(bv[0]), std::abs(bv[1]), std::abs(bv[2])});
    const long double xn = std::max({std::abs(xv[0]), std::abs(xv[1]), std::abs(xv[2])});
    int ae = 0, be = 0, xe = 0;
    static_cast<void>(std::frexp(an, &ae));
    static_cast<void>(std::frexp(bn, &be));
    static_cast<void>(std::frexp(xn, &xe));
    const int common = std::max(ae + xe, be);
    const long double aw = std::scalbn(1.0L, ae + xe - common);
    const long double bw = std::scalbn(1.0L, be - common);
    long double matrix_norm = 0, residual = 0, vector_norm = 0, rhs_norm = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        long double row_sum = 0, dot = 0;
        for (std::size_t j = 0; j < 3; ++j) {
            const long double av = std::scalbn(static_cast<long double>(a(i, j)), -ae);
            row_sum += std::abs(av);
            dot += av * std::scalbn(xv[j], -xe);
        }
        residual = std::max(residual, std::abs(aw * dot - bw * std::scalbn(bv[i], -be)));
        matrix_norm = std::max(matrix_norm, row_sum);
        vector_norm = std::max(vector_norm, std::abs(std::scalbn(xv[i], -xe)));
        rhs_norm = std::max(rhs_norm, std::abs(std::scalbn(bv[i], -be)));
    }
    const long double denominator = aw * matrix_norm * vector_norm + bw * rhs_norm;
    return denominator > 0 ? residual / denominator : 0;
}

template <typename T>
void check_solution(const M<T>& a, const V<T>& b, const V<T>& expected) {
    const auto result = G::SolveSymmetricPositiveDefinite3x3(a, b);
    ASSERT_TRUE(result.has_value());
    const auto& x = result.value();
    const T eps = std::numeric_limits<T>::epsilon();
    EXPECT_NEAR(x.x, expected.x, T{64} * eps * std::max(T{1}, std::abs(expected.x)));
    EXPECT_NEAR(x.y, expected.y, T{64} * eps * std::max(T{1}, std::abs(expected.y)));
    EXPECT_NEAR(x.z, expected.z, T{64} * eps * std::max(T{1}, std::abs(expected.z)));
    const auto eta = oracle_eta(a, b, x);
    EXPECT_TRUE(std::isfinite(eta));
    EXPECT_LE(eta, 100.0L * static_cast<long double>(eps));
}

template <typename T>
M<T> coupled(T scale, T c) {
    return {scale, c * scale, T{0}, c * scale, scale, T{0}, T{0}, T{0}, scale};
}

TYPED_TEST(LDLTScaleTest, PowerOfTwoAndDecimalScaleInvariance) {
    using T = TypeParam;
    // A0 has eigenvalues (0.5,1,1.5), cond2=3, x=(2,-2,1), b0=(1,-1,1).
    constexpr int first = std::numeric_limits<T>::min_exponent - std::numeric_limits<T>::digits + 1;
    constexpr int last = std::numeric_limits<T>::max_exponent - 1;
    constexpr int stride = 17;
    for (int exponent = first; exponent <= last; exponent += stride) {
        const T scale = std::scalbn(T{1}, exponent);
        SCOPED_TRACE(exponent);
        check_solution(coupled(scale, T{0.5}), V<T>{scale, -scale, scale}, V<T>{T{2}, T{-2}, T{1}});
    }
    const T small = std::same_as<T, float> ? static_cast<T>(1e-30) : static_cast<T>(1e-300);
    const T large = std::same_as<T, float> ? static_cast<T>(1e30) : static_cast<T>(1e300);
    for (T scale : {small, T{1}, large, std::numeric_limits<T>::max()}) {
        SCOPED_TRACE(scale);
        check_solution(coupled(scale, T{0.5}), V<T>{scale, -scale, scale}, V<T>{T{2}, T{-2}, T{1}});
    }
}

TYPED_TEST(LDLTScaleTest, ModerateConditionAuditAndOldNanAcceptance) {
    using T = TypeParam;
    const T huge = std::same_as<T, float> ? static_cast<T>(2e38) : static_cast<T>(1e308);
    for (T scale : {T{1}, std::numeric_limits<T>::min(), huge}) {
        SCOPED_TRACE(scale);
        check_solution(coupled(scale, static_cast<T>(0.9)), V<T>{scale, -scale, scale}, V<T>{T{10}, T{-10}, T{1}});
        check_solution(coupled(scale, T{0.5}), V<T>{scale, -scale, scale}, V<T>{T{2}, T{-2}, T{1}});
    }
}

TYPED_TEST(LDLTScaleTest, DiagonalAndIndependentRightHandSideScales) {
    using T = TypeParam;
    for (T scale : {std::numeric_limits<T>::min(), T{1}, std::numeric_limits<T>::max() / T{8}}) {
        const M<T> a{T{2}*scale,T{0},T{0},T{0},T{3}*scale,T{0},T{0},T{0},T{4}*scale};
        check_solution(a, V<T>{T{2}*scale,T{-6}*scale,T{2}*scale}, V<T>{T{1},T{-2},T{0.5}});
    }
    // Output range extremes and zero RHS. No reciprocal of a tiny scale is formed.
    for (T rhs : {std::numeric_limits<T>::denorm_min(), std::numeric_limits<T>::min(), T{1}, std::numeric_limits<T>::max()}) {
        const auto result = G::SolveSymmetricPositiveDefinite3x3(M<T>::Identity(), V<T>{rhs,-rhs,T{0}});
        ASSERT_TRUE(result);
        EXPECT_NEAR(result.value().x, rhs, T{0});
        EXPECT_NEAR(result.value().y, -rhs, T{0});
    }
    check_solution(M<T>::Identity(), V<T>{}, V<T>{});
    const T tiny = std::numeric_limits<T>::denorm_min();
    const M<T> tiny_a{tiny,T{0},T{0},T{0},tiny,T{0},T{0},T{0},tiny};
    check_solution(tiny_a, V<T>{tiny,-tiny,tiny}, V<T>{T{1},T{-1},T{1}});
}

TYPED_TEST(LDLTScaleTest, CoupledThreeAxesAndUnrepresentableScaleRatios) {
    using T = TypeParam;
    // Tridiagonal eigenvalues 2-sqrt(2),2,2+sqrt(2): cond2=3+2*sqrt(2).
    for (T scale : {std::numeric_limits<T>::min(),T{1},std::numeric_limits<T>::max()/T{4}}) {
        const M<T> a{T{2}*scale,scale,T{0},scale,T{2}*scale,scale,T{0},scale,T{2}*scale};
        check_solution(a,V<T>{T{0},T{0},T{4}*scale},V<T>{T{1},T{-2},T{3}});
    }
    // sb/sa would overflow, but each output is finite. cond2=4, row sums=1.
    const M<T> dense{T{0.5},T{0.25},T{0.25},T{0.25},T{0.5},T{0.25},T{0.25},T{0.25},T{0.5}};
    const T large=std::numeric_limits<T>::max()*T{0.75};
    check_solution(dense,V<T>{large,large,large},V<T>{large,large,large});
    // sb/sa would round to zero, but y=2 restores a nonzero subnormal output.
    const M<T> diagonal{T{2},T{0},T{0},T{0},T{1},T{0},T{0},T{0},T{1}};
    const T tiny=std::numeric_limits<T>::denorm_min();
    const auto r=G::SolveSymmetricPositiveDefinite3x3(diagonal,V<T>{T{0},tiny,T{0}});
    ASSERT_TRUE(r); EXPECT_NEAR(r.value().y,tiny,T{0});
}

TYPED_TEST(LDLTScaleTest, NonFiniteInputsAndNoInputMutation) {
    using T = TypeParam;
    const T nan = std::numeric_limits<T>::quiet_NaN(), inf = std::numeric_limits<T>::infinity();
    for (T bad : {nan, inf, -inf}) {
        for (std::size_t i = 0; i < 9; ++i) {
            M<T> a = M<T>::Identity(); a.m[i] = bad;
            const auto result = G::SolveSymmetricPositiveDefinite3x3(a, V<T>{T{1},T{2},T{3}});
            ASSERT_FALSE(result); EXPECT_EQ(result.error(), Error::non_finite_input);
            EXPECT_TRUE(std::isnan(bad) ? std::isnan(a.m[i]) : a.m[i] == bad);
        }
        for (std::size_t i = 0; i < 3; ++i) {
            std::array<T,3> v{T{1},T{2},T{3}}; v[i] = bad;
            const auto result = G::SolveSymmetricPositiveDefinite3x3(M<T>::Identity(), V<T>{v[0],v[1],v[2]});
            ASSERT_FALSE(result); EXPECT_EQ(result.error(), Error::non_finite_input);
        }
    }
    // Return-by-value API: there is no caller-supplied output parameter.
    const M<T> a = M<T>::Zero(); const V<T> b{T{1},T{2},T{3}};
    const auto result = G::SolveSymmetricPositiveDefinite3x3(a,b);
    EXPECT_FALSE(result);
    EXPECT_NEAR(b.y,T{2},T{0}); EXPECT_NEAR(a.m[0],T{0},T{0});
}

TYPED_TEST(LDLTScaleTest, SymmetryPolicyIsRelativeAndUsesOriginalResidual) {
    using T = TypeParam;
    const T eps = std::numeric_limits<T>::epsilon();
    for (T scale : {std::numeric_limits<T>::min(),T{1},std::numeric_limits<T>::max()/T{4}}) {
        M<T> a = M<T>::Identity()*scale;
        // Accepted approximate input; factor the lower mirror (identity).
        a.m[1] = T{50}*eps*scale;
        check_solution(a,V<T>{scale,T{-2}*scale,T{3}*scale},V<T>{T{1},T{-2},T{3}});
        for (std::size_t index : {std::size_t{1},std::size_t{2},std::size_t{5}}) {
            a = M<T>::Identity()*scale; a.m[index] = T{200}*eps*scale;
            const auto result = G::SolveSymmetricPositiveDefinite3x3(a,V<T>{scale,scale,scale});
            ASSERT_FALSE(result); EXPECT_EQ(result.error(),Error::invalid_argument);
        }
    }
    // A lower-mirror solution must still be checked against the upper triangle.
    const M<T> asymmetric{T{1},T{3},T{0},T{0},T{1},T{0},T{0},T{0},T{1}};
    const std::array<T,3> candidate{T{0},T{1},T{1}}, rhs{T{0},T{1},T{1}};
    EXPECT_TRUE(G::Detail::SolverResidualAccepted(M<T>::Identity(),candidate,rhs));
    EXPECT_FALSE(G::Detail::SolverResidualAccepted(asymmetric,candidate,rhs));
}

TYPED_TEST(LDLTScaleTest, RejectsNonSpdUnusableAndUnrepresentableSolutions) {
    using T = TypeParam;
    const V<T> b{T{1},T{1},T{1}};
    auto zero = G::SolveSymmetricPositiveDefinite3x3(M<T>::Zero(),b);
    ASSERT_FALSE(zero); EXPECT_EQ(zero.error(),Error::singular_matrix);
    for (std::size_t axis = 0; axis < 3; ++axis) {
        M<T> a=M<T>::Identity(); a(axis,axis)=T{-1};
        auto negative=G::SolveSymmetricPositiveDefinite3x3(a,b);
        ASSERT_FALSE(negative); EXPECT_EQ(negative.error(),Error::invalid_state);
        a(axis,axis)=T{0};
        auto singular=G::SolveSymmetricPositiveDefinite3x3(a,b);
        ASSERT_FALSE(singular); EXPECT_EQ(singular.error(),Error::singular_matrix);
    }
    M<T> bad=M<T>::Identity(); bad(2,2)=T{50}*std::numeric_limits<T>::epsilon();
    const auto spread=G::SolveSymmetricPositiveDefinite3x3(bad,b);
    ASSERT_FALSE(spread); EXPECT_EQ(spread.error(),Error::ill_conditioned);
    const auto overflow=G::SolveSymmetricPositiveDefinite3x3(M<T>::Identity()*T{0.5},V<T>{std::numeric_limits<T>::max(),T{0},T{0}});
    ASSERT_FALSE(overflow); EXPECT_EQ(overflow.error(),Error::ill_conditioned);
    const auto underflow=G::SolveSymmetricPositiveDefinite3x3(M<T>::Identity()*std::numeric_limits<T>::max(),V<T>{std::numeric_limits<T>::denorm_min(),T{0},T{0}});
    ASSERT_FALSE(underflow); EXPECT_EQ(underflow.error(),Error::ill_conditioned);
}

TYPED_TEST(LDLTScaleTest, NonFiniteDiagnosticsFailClosed) {
    using T = TypeParam;
    const T inf=std::numeric_limits<T>::infinity(), nan=std::numeric_limits<T>::quiet_NaN();
    for (T bad : {nan,inf,-inf}) {
        EXPECT_FALSE(G::Detail::SolverBackwardErrorAccepted(bad,T{1}));
        EXPECT_FALSE(G::Detail::SolverBackwardErrorAccepted(T{0},bad));
        EXPECT_FALSE(G::Detail::SolverPivot(bad));
        for (std::size_t i=0;i<3;++i) {
            std::array<T,3> values{T{1},T{1},T{1}}; values[i]=bad;
            EXPECT_FALSE(G::Detail::SolverResidualAccepted(M<T>::Identity(),values,{T{1},T{1},T{1}}));
            EXPECT_FALSE(G::Detail::SolverResidualAccepted(M<T>::Identity(),{T{1},T{1},T{1}},values));
        }
        M<T> a=M<T>::Identity(); a.m[0]=bad;
        EXPECT_FALSE(G::Detail::SolverResidualAccepted(a,{T{1},T{1},T{1}},{T{1},T{1},T{1}}));
    }
    // Both inputs finite, but eta itself overflows. Must hit explicit !finite(eta).
    EXPECT_FALSE(G::Detail::SolverBackwardErrorAccepted(std::numeric_limits<T>::max(),std::numeric_limits<T>::min()));
    EXPECT_TRUE(G::Detail::SolverBackwardErrorAccepted(T{0},T{0}));
    EXPECT_FALSE(G::Detail::SolverBackwardErrorAccepted(T{1},T{0}));
    EXPECT_FALSE(G::Detail::SolverBackwardErrorAccepted(T{1},T{1}));
    M<T> huge=M<T>::Identity(); huge.m[0]=std::numeric_limits<T>::max(); huge.m[1]=huge.m[0];
    EXPECT_FALSE(G::Detail::SolverResidualAccepted(huge,{T{1},T{1},T{1}},{T{1},T{1},T{1}}));
}

TYPED_TEST(LDLTScaleTest, ConstantEvaluationAlgorithmsMatchStandardLibrary) {
    using T = TypeParam;
    constexpr int minimum = std::numeric_limits<T>::min_exponent - std::numeric_limits<T>::digits;
    constexpr int maximum = std::numeric_limits<T>::max_exponent;
    for (int exponent = minimum - 4; exponent <= maximum + 4; ++exponent) {
        for (T mantissa : {T{0}, T{0.25}, T{-0.5}, T{0.75}, T{1.5}, T{-1.75}}) {
            const T expected = std::scalbn(mantissa, exponent);
            const T actual = G::Detail::SolverScaleExponentFallback(mantissa, exponent);
            SCOPED_TRACE(::testing::Message() << exponent << ':' << mantissa);
            if (std::isfinite(expected)) {
                EXPECT_NEAR(actual, expected, T{0});
                int expected_exp=0, actual_exp=0;
                const T expected_fraction=std::frexp(expected,&expected_exp);
                const T actual_fraction=G::Detail::SolverFractionFallback(expected,actual_exp);
                EXPECT_NEAR(actual_fraction,expected_fraction,T{0});
                EXPECT_EQ(actual_exp,expected_exp);
            } else {
                EXPECT_TRUE(std::isinf(actual));
                EXPECT_EQ(std::signbit(actual),std::signbit(expected));
            }
        }
    }
}

template <typename T>
constexpr bool constexpr_solve() {
    const M<T> a{T{1},T{0.5},T{0},T{0.5},T{1},T{0},T{0},T{0},T{1}};
    const auto r=G::SolveSymmetricPositiveDefinite3x3(a,V<T>{T{1},T{-1},T{1}});
    return r.has_value() && G::Detail::ConstexprAbs(r.value().x-T{2})<T{8}*std::numeric_limits<T>::epsilon();
}
static_assert(constexpr_solve<float>());
static_assert(constexpr_solve<double>());
static_assert(G::Detail::SolverRescale(1.0,std::numeric_limits<double>::denorm_min(),std::numeric_limits<double>::denorm_min())>0.99);
static_assert(G::Detail::SolverRescale(1.0,std::numeric_limits<double>::max(),1.0)>1e308);
} // namespace
