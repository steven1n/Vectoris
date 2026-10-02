#include <gtest/gtest.h>
#include <cmath>
#include <array>
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"

using namespace vectoris::numerics;
using namespace vectoris::numerics::Geometry;

namespace {

template <typename T>
bool MatrixAlmostEqual(const Matrix3<T>& a, const Matrix3<T>& b, T tol = static_cast<T>(1e-9)) {
    for (size_t i = 0; i < 9; ++i) {
        if (std::abs(a.m[i] - b.m[i]) > tol) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(Matrix3Test, InverseIdentity) {
    Matrix3<double> I = Matrix3<double>::Identity();
    Matrix3<double> inv;
    ASSERT_TRUE(I.TryInverse(inv));
    EXPECT_TRUE(MatrixAlmostEqual(inv, I, 1e-15));
}

TEST(Matrix3Test, InverseKnownNonSingular) {
    // Matrix A with det(A) = 1.0
    // [ 1  2  3 ]
    // [ 0  1  4 ]
    // [ 5  6  0 ]
    Matrix3<double> A(
        1.0, 2.0, 3.0,
        0.0, 1.0, 4.0,
        5.0, 6.0, 0.0
    );

    Matrix3<double> inv;
    ASSERT_TRUE(A.TryInverse(inv));

    Matrix3<double> A_times_inv = A * inv;
    Matrix3<double> inv_times_A = inv * A;
    Matrix3<double> expected_I = Matrix3<double>::Identity();

    EXPECT_TRUE(MatrixAlmostEqual(A_times_inv, expected_I, 1e-12));
    EXPECT_TRUE(MatrixAlmostEqual(inv_times_A, expected_I, 1e-12));
}

TEST(Matrix3Test, CofactorIndexRegression_AML_CRIT_002) {
    // Matrix where m[1] != m[2] (m[1]=2.0, m[2]=3.0)
    // det(A) = 1.0
    // Analytical Inverse:
    // C_12 = -(m0*m7 - m1*m6) = -(1*6 - 2*5) = -(-4) = +4.0
    // Faulty formula uses m[2]: -(1*6 - 3*5) = -(-9) = +9.0
    Matrix3<double> A(
        1.0, 2.0, 3.0,
        0.0, 1.0, 4.0,
        5.0, 6.0, 0.0
    );

    Matrix3<double> inv;
    ASSERT_TRUE(A.TryInverse(inv));

    // Element (row 2, col 1) in 0-indexed coords is m[7]
    EXPECT_NEAR(inv(2, 1), 4.0, 1e-12);
    EXPECT_NEAR(inv.m[7], 4.0, 1e-12);
}

TEST(Matrix3Test, InPlaceAliasingSafety) {
    Matrix3<double> A(
        1.0, 2.0, 3.0,
        0.0, 1.0, 4.0,
        5.0, 6.0, 0.0
    );

    Matrix3<double> separate_inv;
    ASSERT_TRUE(A.TryInverse(separate_inv));

    // In-place inversion: A_aliased.TryInverse(A_aliased)
    Matrix3<double> A_aliased = A;
    ASSERT_TRUE(A_aliased.TryInverse(A_aliased));

    // Must match the non-aliased result exactly
    EXPECT_TRUE(MatrixAlmostEqual(A_aliased, separate_inv, 1e-15));
}

TEST(Matrix3Test, SingularMatrixRejection) {
    // Rank 1 matrix
    Matrix3<double> rank1(
        1.0, 2.0, 3.0,
        2.0, 4.0, 6.0,
        3.0, 6.0, 9.0
    );
    Matrix3<double> inv;
    EXPECT_FALSE(rank1.TryInverse(inv));

    // Zero matrix
    Matrix3<double> zero = Matrix3<double>::Zero();
    EXPECT_FALSE(zero.TryInverse(inv));
}

TEST(Matrix3Test, NearSingularMatrixHandling) {
    // Matrix with nearly dependent rows
    Matrix3<double> near_singular(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0 + 1e-18
    );
    Matrix3<double> inv;
    // Should safely report failure or invert without producing NaN/Inf
    bool success = near_singular.TryInverse(inv);
    if (success) {
        for (int i = 0; i < 9; ++i) {
            EXPECT_TRUE(std::isfinite(inv.m[i]));
        }
    }
}

TEST(Matrix3Test, ScaledWellConditionedMatrix) {
    // A = 1e-4 * I. Well conditioned (kappa = 1), det = 1e-12.
    // Scale-aware test should succeed and not falsely declare it singular.
    Matrix3<double> scaled(
        1e-4, 0.0, 0.0,
        0.0, 1e-4, 0.0,
        0.0, 0.0, 1e-4
    );
    Matrix3<double> inv;
    ASSERT_TRUE(scaled.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), 1e4, 1e-6);
    EXPECT_NEAR(inv(1, 1), 1e4, 1e-6);
    EXPECT_NEAR(inv(2, 2), 1e4, 1e-6);
}

TEST(Matrix3Test, NonFiniteInputTryInverseRejection) {
    Matrix3<double> nan_mat(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    Matrix3<double> inv;
    EXPECT_FALSE(nan_mat.TryInverse(inv));

    Matrix3<double> inf_mat(
        std::numeric_limits<double>::infinity(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    EXPECT_FALSE(inf_mat.TryInverse(inv));
}

TEST(Matrix3Test, LargeWellConditionedMatrixScaleRobustnessDouble) {
    // Large well-conditioned matrix where det() would overflow unnormalized scale^3 (1e150^3 = 1e450)
    // Scale-normalized implementation correctly inverts with inv = 1e-150
    Matrix3<double> big_pos(
        1e150, 0.0, 0.0,
        0.0, 1e150, 0.0,
        0.0, 0.0, 1e150
    );
    Matrix3<double> inv;
    ASSERT_TRUE(big_pos.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), 1e-150, 1e-160);
    EXPECT_NEAR(inv(1, 1), 1e-150, 1e-160);
    EXPECT_NEAR(inv(2, 2), 1e-150, 1e-160);

    Matrix3<double> big_neg(
        -1e150, 0.0, 0.0,
        0.0, 1e150, 0.0,
        0.0, 0.0, 1e150
    );
    ASSERT_TRUE(big_neg.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), -1e-150, 1e-160);

    // True mathematical inverse overflow (1 / 1e-310 = 1e310 > double::max ~ 1.8e308)
    Matrix3<double> tiny_overflow(
        1e-310, 0.0, 0.0,
        0.0, 1e-310, 0.0,
        0.0, 0.0, 1e-310
    );
    EXPECT_FALSE(tiny_overflow.TryInverse(inv));
}

TEST(Matrix3Test, FloatComparisonAndAlmostEqual) {
    Matrix3<float> m1 = Matrix3<float>::Identity();
    Matrix3<float> m2 = m1;
    Matrix3<float> m3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.01f
    );
    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 != m2);
    EXPECT_FALSE(m1 == m3);
    EXPECT_TRUE(m1 != m3);
    EXPECT_TRUE(m1.AlmostEqual(m2));
    EXPECT_TRUE(m1.AlmostEqual(m3, 0.05f, 0.05f));
    EXPECT_FALSE(m1.AlmostEqual(m3, 0.001f, 0.001f));
    EXPECT_TRUE(AlmostEqual(m1, m2));
    EXPECT_TRUE(AlmostEqual(m1, m3, 0.05f, 0.05f));
    EXPECT_FALSE(AlmostEqual(m1, m3, 0.001f, 0.001f));
}

TEST(Matrix3Test, FloatTryInverseComprehensive) {
    Matrix3<float> I = Matrix3<float>::Identity();
    Matrix3<float> inv;
    ASSERT_TRUE(I.TryInverse(inv));
    EXPECT_TRUE(MatrixAlmostEqual(inv, I, 1e-6f));

    Matrix3<float> A(
        1.0f, 2.0f, 3.0f,
        0.0f, 1.0f, 4.0f,
        5.0f, 6.0f, 0.0f
    );
    ASSERT_TRUE(A.TryInverse(inv));
    Matrix3<float> A_times_inv = A * inv;
    EXPECT_TRUE(MatrixAlmostEqual(A_times_inv, I, 1e-5f));

    // Aliasing
    Matrix3<float> A_aliased = A;
    ASSERT_TRUE(A_aliased.TryInverse(A_aliased));
    EXPECT_TRUE(MatrixAlmostEqual(A_aliased, inv, 1e-6f));

    // Negative determinant branch
    Matrix3<float> A_neg_det(
        -1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f
    );
    ASSERT_TRUE(A_neg_det.TryInverse(inv));
    EXPECT_FLOAT_EQ(inv(0, 0), -1.0f);

    // Zero matrix
    Matrix3<float> zero = Matrix3<float>::Zero();
    EXPECT_FALSE(zero.TryInverse(inv));

    // Singular rank 1
    Matrix3<float> rank1(
        1.0f, 2.0f, 3.0f,
        2.0f, 4.0f, 6.0f,
        3.0f, 6.0f, 9.0f
    );
    EXPECT_FALSE(rank1.TryInverse(inv));

    // Scale cutoff
    Matrix3<float> near_singular(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1e-8f
    );
    EXPECT_FALSE(near_singular.TryInverse(inv));

    // Scaled well-conditioned
    Matrix3<float> scaled(
        1e-2f, 0.0f, 0.0f,
        0.0f, 1e-2f, 0.0f,
        0.0f, 0.0f, 1e-2f
    );
    ASSERT_TRUE(scaled.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), 1e2f, 1e-3f);

    // Non-finite input
    Matrix3<float> nan_mat(
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    EXPECT_FALSE(nan_mat.TryInverse(inv));

    Matrix3<float> inf_mat(
        std::numeric_limits<float>::infinity(), 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    EXPECT_FALSE(inf_mat.TryInverse(inv));

    // Scale robustness for large well-conditioned matrix in float (1e20f^3 = 1e60f overflows float)
    // Scale-normalized implementation correctly inverts with inv = 1e-20f
    Matrix3<float> big_pos(
        1e20f, 0.0f, 0.0f,
        0.0f, 1e20f, 0.0f,
        0.0f, 0.0f, 1e20f
    );
    ASSERT_TRUE(big_pos.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), 1e-20f, 1e-25f);
    EXPECT_NEAR(inv(1, 1), 1e-20f, 1e-25f);
    EXPECT_NEAR(inv(2, 2), 1e-20f, 1e-25f);

    Matrix3<float> big_neg(
        -1e20f, 0.0f, 0.0f,
        0.0f, 1e20f, 0.0f,
        0.0f, 0.0f, 1e20f
    );
    ASSERT_TRUE(big_neg.TryInverse(inv));
    EXPECT_NEAR(inv(0, 0), -1e-20f, 1e-25f);

    // True mathematical inverse overflow (1 / 1e-40f = 1e40f > float::max ~ 3.4e38)
    Matrix3<float> tiny_overflow(
        1e-40f, 0.0f, 0.0f,
        0.0f, 1e-40f, 0.0f,
        0.0f, 0.0f, 1e-40f
    );
    EXPECT_FALSE(tiny_overflow.TryInverse(inv));

    // Frobenius norm squared for float
    EXPECT_FLOAT_EQ(I.frobenius_norm_squared(), 3.0f);
}



// Candidate #11: inverse needs a non-truncating numeric domain.
namespace {
template<class T> concept C11HasInverse = requires(const Matrix3<T>& a, Matrix3<T>& out) { a.TryInverse(out); };
static_assert(!C11HasInverse<int> && !C11HasInverse<unsigned>);
static_assert(C11HasInverse<float> && C11HasInverse<double>);
}
TEST(C11MatrixDomain, IntegerConstructionAndArithmeticRemainSupported) {
    Matrix3<int> a{2,0,0,0,1,0,0,0,1};
    EXPECT_EQ(a(0,0),2);
    EXPECT_EQ((a + Matrix3<int>::Identity())(0,0),3);
    EXPECT_EQ((a - Matrix3<int>::Identity())(1,1),0);
    EXPECT_EQ((a * Matrix3<int>::Identity())(0,0),2);
    const auto determinant = a.det();
    ASSERT_TRUE(determinant.IsSuccess());
    EXPECT_EQ(determinant.Value(),2);
}
TEST(C11MatrixDomain, IntMinInverseIsUnavailableWithoutEvaluation) {
    Matrix3<int> a{std::numeric_limits<int>::lowest(),0,0,0,1,0,0,0,1};
    static_assert(!C11HasInverse<int>);
    const auto determinant = a.det();
    ASSERT_TRUE(determinant.IsSuccess());
    EXPECT_EQ(determinant.Value(),std::numeric_limits<int>::lowest());
}
TEST(C11MatrixDomain, CheckedIntegerDeterminantRejectsOverflow) {
    const int low=std::numeric_limits<int>::lowest(), high=std::numeric_limits<int>::max();
    for (const auto& a : std::array<Matrix3<int>,6>{
        Matrix3<int>{low,0,0,0,2,0,0,0,1},
        Matrix3<int>{high,0,0,0,2,0,0,0,1},
        Matrix3<int>{1,0,0,0,high,0,0,0,2},
        Matrix3<int>{1,0,0,0,0,low,0,1,0},
        Matrix3<int>{1,0,0,0,high,1,0,-1,1},
        Matrix3<int>{1,0,0,0,low,1,0,1,1}}) {
        const auto determinant=a.det();
        ASSERT_FALSE(determinant.IsSuccess());
        EXPECT_EQ(determinant.error(),Core::MathError::domain_error);
    }
}
TEST(C11MatrixDomain, CheckedIntegerDeterminantSignsAndZero) {
    for (int x : {-7,0,7}) for (int y : {-3,0,3}) for (int z : {-2,0,2}) {
        const Matrix3<int> a{x,0,0,0,y,0,0,0,z};
        const auto determinant=a.det();
        ASSERT_TRUE(determinant.IsSuccess());
        EXPECT_EQ(determinant.Value(),x*y*z);
    }
    const Matrix3<int> negative_term{0,2,0,3,0,0,0,0,1};
    EXPECT_EQ(negative_term.det().Value(),-6);
    const Matrix3<int> third_term{0,0,2,3,0,0,0,1,0};
    EXPECT_EQ(third_term.det().Value(),6);
}
TEST(C11MatrixDomain, UnsignedDeterminantIsChecked) {
    const Matrix3<unsigned> ok{2,0,0,0,3,0,0,0,4};
    EXPECT_EQ(ok.det().Value(),24U);
    const Matrix3<unsigned> overflow{std::numeric_limits<unsigned>::max(),0,0,0,2,0,0,0,1};
    EXPECT_FALSE(overflow.det().IsSuccess());
    const Matrix3<unsigned> negative{0,1,0,1,0,0,0,0,1};
    EXPECT_FALSE(negative.det().IsSuccess());
}
TEST(C11MatrixDomain, FloatingInverseCompatibility) {
    const Matrix3<double> a{2,0,0,0,1,0,0,0,1};Matrix3<double> inverse;
    ASSERT_TRUE(a.TryInverse(inverse));
    EXPECT_DOUBLE_EQ(inverse(0,0),0.5);
    EXPECT_TRUE((a*inverse).AlmostEqual(Matrix3<double>::Identity()));
    const Matrix3<float> f{2,0,0,0,1,0,0,0,1};Matrix3<float> fi;
    ASSERT_TRUE(f.TryInverse(fi));
    EXPECT_FLOAT_EQ(fi(0,0),0.5f);
    EXPECT_TRUE((f*fi).AlmostEqual(Matrix3<float>::Identity()));
}

TEST(C11MatrixDomain, CheckedDeterminantAccumulatorBoundaries) {
    const int low=std::numeric_limits<int>::lowest(), high=std::numeric_limits<int>::max();
    for(const auto& a:std::array<Matrix3<int>,6>{
        Matrix3<int>{high,0,-1,0,1,0,1,0,1},
        Matrix3<int>{low,0,1,0,1,0,1,0,1},
        Matrix3<int>{low,0,0,0,-1,0,0,0,1},
        Matrix3<int>{high,0,0,0,-2,0,0,0,1},
        Matrix3<int>{high,-1,0,1,1,0,0,0,1},
        Matrix3<int>{low,1,0,1,1,0,0,0,1}}) EXPECT_FALSE(a.det().IsSuccess());
    for(int term:{-3,0,3}) {
        const Matrix3<int> a{7,0,term,0,1,0,1,0,1};
        EXPECT_EQ(a.det().Value(),7-term);
    }
    const Matrix3<unsigned> unsigned_sum_overflow{std::numeric_limits<unsigned>::max(),0,1,1,1,0,0,1,1};
    EXPECT_FALSE(unsigned_sum_overflow.det().IsSuccess());
    const Matrix3<unsigned> unsigned_sum{3,0,1,1,1,0,0,1,1};
    EXPECT_EQ(unsigned_sum.det().Value(),4U);
}
