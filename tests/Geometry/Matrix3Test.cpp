#include <gtest/gtest.h>
#include <cmath>
#include "AegisMath/Geometry/Matrix3.h"
#include "AegisMath/Core/NumericTraits.h"

using namespace AegisMath;
using namespace AegisMath::Geometry;

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

TEST(Matrix3Test, DeterminantOverflowTryInverseRejection) {
    Matrix3<double> big_pos(
        1e200, 0.0, 0.0,
        0.0, 1e200, 0.0,
        0.0, 0.0, 1e200
    );
    Matrix3<double> inv;
    EXPECT_FALSE(big_pos.TryInverse(inv));

    Matrix3<double> big_neg(
        -1e200, 0.0, 0.0,
        0.0, 1e200, 0.0,
        0.0, 0.0, 1e200
    );
    EXPECT_FALSE(big_neg.TryInverse(inv));
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


