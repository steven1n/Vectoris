#include <gtest/gtest.h>
#include <cmath>
#include "AegisMath/Geometry/SymmetricLinearSolver3.h"

struct TestFrame {};

using namespace AegisMath::Geometry;
using namespace AegisMath::Core;

// 编译期 constexpr 求解验证
constexpr bool VerifyConstexprSolve() {
    Matrix3<double> A(
        2.0, 0.0, 0.0,
        0.0, 4.0, 0.0,
        0.0, 0.0, 8.0
    );
    Vector3<double, TestFrame> b(2.0, 4.0, 8.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    if (!res.has_value()) return false;
    auto x = res.value();
    return (x.x == 1.0) && (x.y == 1.0) && (x.z == 1.0);
}
static_assert(VerifyConstexprSolve(), "SolveSymmetricPositiveDefinite3x3 must be usable in constexpr context.");

TEST(SymmetricLinearSolver3Test, IdentityMatrix) {
    Matrix3<double> I = Matrix3<double>::Identity();
    Vector3<double, TestFrame> b(3.5, -2.1, 7.8);
    auto res = SolveSymmetricPositiveDefinite3x3(I, b);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res.value().x, 3.5);
    EXPECT_DOUBLE_EQ(res.value().y, -2.1);
    EXPECT_DOUBLE_EQ(res.value().z, 7.8);
}

TEST(SymmetricLinearSolver3Test, DiagonalMatrix) {
    Matrix3<double> A(
        10.0, 0.0, 0.0,
        0.0, 20.0, 0.0,
        0.0, 0.0, 50.0
    );
    Vector3<double, TestFrame> b(5.0, 10.0, -25.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res.value().x, 0.5);
    EXPECT_DOUBLE_EQ(res.value().y, 0.5);
    EXPECT_DOUBLE_EQ(res.value().z, -0.5);
}

TEST(SymmetricLinearSolver3Test, NonDiagonalSPDMatrix) {
    // 经严格验证的物理正定惯量张量矩阵
    // A = [[10, 2, 1], [2, 12, 3], [1, 3, 15]]
    // det(A) = 1650.0
    Matrix3<double> A(
        10.0,  2.0,  1.0,
         2.0, 12.0,  3.0,
         1.0,  3.0, 15.0
    );
    Vector3<double, TestFrame> b(3.0, 0.0, 23.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_TRUE(res.has_value());
    auto x = res.value();

    // 独立基准解 (Exact Adjugate Solve)
    const double ref_x = 0.22727272727272727;
    const double ref_y = -0.43939393939393934;
    const double ref_z = 1.606060606060606;

    EXPECT_NEAR(x.x, ref_x, 1e-14);
    EXPECT_NEAR(x.y, ref_y, 1e-14);
    EXPECT_NEAR(x.z, ref_z, 1e-14);

    // 残差验证: A * x ≈ b
    double Ax = A(0,0)*x.x + A(0,1)*x.y + A(0,2)*x.z;
    double Ay = A(1,0)*x.x + A(1,1)*x.y + A(1,2)*x.z;
    double Az = A(2,0)*x.x + A(2,1)*x.y + A(2,2)*x.z;
    EXPECT_NEAR(Ax, b.x, 1e-14);
    EXPECT_NEAR(Ay, b.y, 1e-14);
    EXPECT_NEAR(Az, b.z, 1e-14);
}

TEST(SymmetricLinearSolver3Test, PropertyTestMultipleVectors) {
    Matrix3<double> A(
        10.0,  2.0,  1.0,
         2.0, 12.0,  3.0,
         1.0,  3.0, 15.0
    );

    const Vector3<double, TestFrame> test_vectors[] = {
        Vector3<double, TestFrame>(1.0, 0.0, 0.0),
        Vector3<double, TestFrame>(0.0, 1.0, 0.0),
        Vector3<double, TestFrame>(0.0, 0.0, 1.0),
        Vector3<double, TestFrame>(1.0, -1.0, 1.0),
        Vector3<double, TestFrame>(-2.5, 3.2, -1.8),
        Vector3<double, TestFrame>(100.0, -50.0, 25.0)
    };

    for (const auto& b : test_vectors) {
        auto res = SolveSymmetricPositiveDefinite3x3(A, b);
        ASSERT_TRUE(res.has_value());
        auto x = res.value();

        // 验证残差 ||A*x - b||_inf <= tol
        double Ax = A(0,0)*x.x + A(0,1)*x.y + A(0,2)*x.z;
        double Ay = A(1,0)*x.x + A(1,1)*x.y + A(1,2)*x.z;
        double Az = A(2,0)*x.x + A(2,1)*x.y + A(2,2)*x.z;

        double b_norm = std::max({std::abs(b.x), std::abs(b.y), std::abs(b.z)});
        double res_norm = std::max({std::abs(Ax - b.x), std::abs(Ay - b.y), std::abs(Az - b.z)});
        EXPECT_LT(res_norm, b_norm * 1e-13);
    }
}

TEST(SymmetricLinearSolver3Test, AsymmetricMatrixRejected) {
    Matrix3<double> A(
        10.0, 2.0, 1.0,
         5.0, 12.0, 3.0, // A(1,0) != A(0,1)
         1.0, 3.0, 15.0
    );
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), MathError::invalid_argument);
}

TEST(SymmetricLinearSolver3Test, SingularMatrixDetected) {
    // 奇异矩阵: 第二行与第一行线性相关
    Matrix3<double> A(
        1.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), MathError::singular_matrix);
}

TEST(SymmetricLinearSolver3Test, IndefiniteMatrixDetected) {
    // 对角为正，但为不定矩阵: det = 1*(1-0) - 2*(2-0) = -3 < 0
    Matrix3<double> A(
        1.0, 2.0, 0.0,
        2.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), MathError::invalid_state);
}

TEST(SymmetricLinearSolver3Test, IllConditionedMatrixDetected) {
    // 双精度病态矩阵: 主元跨度在有效正定但接近奇异边缘 (5e-15 > tol_sing=2.22e-15, 但 <= 100*eps=2.22e-14)
    Matrix3<double> A(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 5e-15
    );
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), MathError::ill_conditioned);
}

TEST(SymmetricLinearSolver3Test, FloatModerateSPDMatrixPasses) {
    // 单精度常规正定矩阵应正常求解，不应被过度保守的启发式门限误拒
    Matrix3<float> A(
        10.0f,  2.0f,  1.0f,
         2.0f, 12.0f,  3.0f,
         1.0f,  3.0f, 15.0f
    );
    Vector3<float, TestFrame> b(3.0f, 0.0f, 23.0f);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_TRUE(res.has_value());
    EXPECT_NEAR(res.value().x, 0.2272727f, 1e-4f);
    EXPECT_NEAR(res.value().y, -0.4393939f, 1e-4f);
    EXPECT_NEAR(res.value().z, 1.6060606f, 1e-4f);

    Vector3<float, TestFrame> b_zero_f(0.0f, 0.0f, 0.0f);
    auto res_zero_f = SolveSymmetricPositiveDefinite3x3(A, b_zero_f);
    ASSERT_TRUE(res_zero_f.has_value());
    EXPECT_FLOAT_EQ(res_zero_f.value().x, 0.0f);
    EXPECT_FLOAT_EQ(res_zero_f.value().y, 0.0f);
    EXPECT_FLOAT_EQ(res_zero_f.value().z, 0.0f);
}

TEST(SymmetricLinearSolver3Test, FloatIllConditionedMatrixDetected) {
    // 单精度病态矩阵检测 (5e-6 > tol_sing=1.19e-6, 但 <= 100*eps=1.19e-5)
    Matrix3<float> A(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 5e-6f
    );
    Vector3<float, TestFrame> b(1.0f, 1.0f, 1.0f);
    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), MathError::ill_conditioned);
}

TEST(SymmetricLinearSolver3Test, NonFiniteInputDetected) {
    Matrix3<double> A_nan(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);
    auto res1 = SolveSymmetricPositiveDefinite3x3(A_nan, b);
    ASSERT_FALSE(res1.has_value());
    EXPECT_EQ(res1.error(), MathError::non_finite_input);

    Matrix3<double> A_ok = Matrix3<double>::Identity();
    Vector3<double, TestFrame> b_inf(1.0, std::numeric_limits<double>::infinity(), 1.0);
    auto res2 = SolveSymmetricPositiveDefinite3x3(A_ok, b_inf);
    ASSERT_FALSE(res2.has_value());
    EXPECT_EQ(res2.error(), MathError::non_finite_input);

    Vector3<double, TestFrame> b_nan_x(std::numeric_limits<double>::quiet_NaN(), 1.0, 1.0);
    auto res_bx = SolveSymmetricPositiveDefinite3x3(A_ok, b_nan_x);
    ASSERT_FALSE(res_bx.has_value());
    EXPECT_EQ(res_bx.error(), MathError::non_finite_input);

    Vector3<double, TestFrame> b_nan_z(1.0, 1.0, std::numeric_limits<double>::quiet_NaN());
    auto res_bz = SolveSymmetricPositiveDefinite3x3(A_ok, b_nan_z);
    ASSERT_FALSE(res_bz.has_value());
    EXPECT_EQ(res_bz.error(), MathError::non_finite_input);
}

TEST(SymmetricLinearSolver3Test, ComprehensiveDefensiveFailureModes) {
    Vector3<double, TestFrame> b(1.0, 2.0, 3.0);

    // 1. Zero scale matrix
    Matrix3<double> A_zero = Matrix3<double>::Zero();
    auto res_zero = SolveSymmetricPositiveDefinite3x3(A_zero, b);
    ASSERT_FALSE(res_zero.has_value());
    EXPECT_EQ(res_zero.error(), MathError::singular_matrix);

    // 2. Asymmetric matrix: (0,1) vs (1,0)
    Matrix3<double> A_asym1(
        1.0, 0.5, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto res_asym1 = SolveSymmetricPositiveDefinite3x3(A_asym1, b);
    ASSERT_FALSE(res_asym1.has_value());
    EXPECT_EQ(res_asym1.error(), MathError::invalid_argument);

    // Asymmetric matrix: (0,2) vs (2,0)
    Matrix3<double> A_asym2(
        1.0, 0.0, 0.5,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto res_asym2 = SolveSymmetricPositiveDefinite3x3(A_asym2, b);
    ASSERT_FALSE(res_asym2.has_value());
    EXPECT_EQ(res_asym2.error(), MathError::invalid_argument);

    // Asymmetric matrix: (1,2) vs (2,1)
    Matrix3<double> A_asym3(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.5,
        0.0, 0.0, 1.0
    );
    auto res_asym3 = SolveSymmetricPositiveDefinite3x3(A_asym3, b);
    ASSERT_FALSE(res_asym3.has_value());
    EXPECT_EQ(res_asym3.error(), MathError::invalid_argument);

    // 3. Negative diagonal pivot d1 < -tol
    Matrix3<double> A_neg1(
        -1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto res_neg1 = SolveSymmetricPositiveDefinite3x3(A_neg1, b);
    ASSERT_FALSE(res_neg1.has_value());
    EXPECT_EQ(res_neg1.error(), MathError::invalid_state);

    // 4. Singular leading pivot d1 <= tol
    Matrix3<double> A_sing1(
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto res_sing1 = SolveSymmetricPositiveDefinite3x3(A_sing1, b);
    ASSERT_FALSE(res_sing1.has_value());
    EXPECT_EQ(res_sing1.error(), MathError::singular_matrix);

    // 5. Indefinite pivot d2 < -tol
    Matrix3<double> A_indef2(
        1.0, 2.0, 0.0,
        2.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto res_indef2 = SolveSymmetricPositiveDefinite3x3(A_indef2, b);
    ASSERT_FALSE(res_indef2.has_value());
    EXPECT_EQ(res_indef2.error(), MathError::invalid_state);

    // 6. Indefinite pivot d3 < -tol
    Matrix3<double> A_indef3(
        1.0, 0.0, 2.0,
        0.0, 1.0, 0.0,
        2.0, 0.0, 1.0
    );
    auto res_indef3 = SolveSymmetricPositiveDefinite3x3(A_indef3, b);
    ASSERT_FALSE(res_indef3.has_value());
    EXPECT_EQ(res_indef3.error(), MathError::invalid_state);

    // 7. Singular pivot d3 <= tol
    Matrix3<double> A_sing3(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 0.0
    );
    auto res_sing3 = SolveSymmetricPositiveDefinite3x3(A_sing3, b);
    ASSERT_FALSE(res_sing3.has_value());
    EXPECT_EQ(res_sing3.error(), MathError::singular_matrix);
}

TEST(SymmetricLinearSolver3Test, ZeroVectorRightHandSideAndOverflow) {
    Matrix3<double> A_ok = Matrix3<double>::Identity();
    Vector3<double, TestFrame> b_zero(0.0, 0.0, 0.0);
    auto res_zero = SolveSymmetricPositiveDefinite3x3(A_ok, b_zero);
    ASSERT_TRUE(res_zero.has_value());
    EXPECT_DOUBLE_EQ(res_zero.value().x, 0.0);
    EXPECT_DOUBLE_EQ(res_zero.value().y, 0.0);
    EXPECT_DOUBLE_EQ(res_zero.value().z, 0.0);

    // Coupled matrix and large b causing intermediate overflow in solution x
    Matrix3<double> A_coupled(
        1.0, -0.999, 0.0,
        -0.999, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    Vector3<double, TestFrame> b_overflow(1e308, 0.0, 0.0);
    auto res_huge = SolveSymmetricPositiveDefinite3x3(A_coupled, b_overflow);
    ASSERT_FALSE(res_huge.has_value());
    EXPECT_EQ(res_huge.error(), MathError::ill_conditioned);

    // Overflow specifically in x1 (x0 finite)
    Matrix3<double> A_x1(
        1.0, 0.0, 0.0,
        0.0, 0.5, 0.0,
        0.0, 0.0, 1.0
    );
    Vector3<double, TestFrame> b_x1(0.0, 1.5e308, 0.0);
    auto res_x1 = SolveSymmetricPositiveDefinite3x3(A_x1, b_x1);
    ASSERT_FALSE(res_x1.has_value());
    EXPECT_EQ(res_x1.error(), MathError::ill_conditioned);

    // Overflow specifically in x2 (x0, x1 finite)
    Matrix3<double> A_x2(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 0.5
    );
    Vector3<double, TestFrame> b_x2(0.0, 0.0, 1.5e308);
    auto res_x2 = SolveSymmetricPositiveDefinite3x3(A_x2, b_x2);
    ASSERT_FALSE(res_x2.has_value());
    EXPECT_EQ(res_x2.error(), MathError::ill_conditioned);
}

TEST(SymmetricLinearSolver3Test, FloatDefensiveFailureModes) {
    Vector3<float, TestFrame> b(1.0f, 1.0f, 1.0f);

    // 1. Asymmetric matrix
    Matrix3<float> A_asym(
        2.0f, 1.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 2.0f
    );
    auto res_asym = SolveSymmetricPositiveDefinite3x3(A_asym, b);
    ASSERT_FALSE(res_asym.has_value());
    EXPECT_EQ(res_asym.error(), MathError::invalid_argument);

    // 2. Zero / singular scale
    Matrix3<float> A_zero = Matrix3<float>::Zero();
    auto res_zero = SolveSymmetricPositiveDefinite3x3(A_zero, b);
    ASSERT_FALSE(res_zero.has_value());
    EXPECT_EQ(res_zero.error(), MathError::singular_matrix);

    // 3. Indefinite pivot d1 < -tol
    Matrix3<float> A_indef1(
        -2.0f, 0.0f, 0.0f,
         0.0f, 2.0f, 0.0f,
         0.0f, 0.0f, 2.0f
    );
    auto res_indef1 = SolveSymmetricPositiveDefinite3x3(A_indef1, b);
    ASSERT_FALSE(res_indef1.has_value());
    EXPECT_EQ(res_indef1.error(), MathError::invalid_state);

    // 4. Singular pivot d1 <= tol
    Matrix3<float> A_sing1(
        0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 2.0f
    );
    auto res_sing1 = SolveSymmetricPositiveDefinite3x3(A_sing1, b);
    ASSERT_FALSE(res_sing1.has_value());
    EXPECT_EQ(res_sing1.error(), MathError::singular_matrix);

    // 5. Indefinite pivot d2 < -tol
    Matrix3<float> A_indef2(
        1.0f, 2.0f, 0.0f,
        2.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    auto res_indef2 = SolveSymmetricPositiveDefinite3x3(A_indef2, b);
    ASSERT_FALSE(res_indef2.has_value());
    EXPECT_EQ(res_indef2.error(), MathError::invalid_state);

    // 6. Indefinite pivot d3 < -tol
    Matrix3<float> A_indef3(
        1.0f, 0.0f, 2.0f,
        0.0f, 1.0f, 0.0f,
        2.0f, 0.0f, 1.0f
    );
    auto res_indef3 = SolveSymmetricPositiveDefinite3x3(A_indef3, b);
    ASSERT_FALSE(res_indef3.has_value());
    EXPECT_EQ(res_indef3.error(), MathError::invalid_state);
}

TEST(SymmetricLinearSolver3Test, AsymmetryAndSingularPivot2And3) {
    Vector3<double, TestFrame> b(1.0, 1.0, 1.0);

    // Asymmetry only in (0, 2) vs (2, 0)
    Matrix3<double> A_asym02(
        2.0, 0.0, 1.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 2.0
    );
    auto r_asym02 = SolveSymmetricPositiveDefinite3x3(A_asym02, b);
    EXPECT_FALSE(r_asym02.has_value());
    EXPECT_EQ(r_asym02.error(), MathError::invalid_argument);

    // Asymmetry only in (1, 2) vs (2, 1)
    Matrix3<double> A_asym12(
        2.0, 0.0, 0.0,
        0.0, 2.0, 1.0,
        0.0, 0.0, 2.0
    );
    auto r_asym12 = SolveSymmetricPositiveDefinite3x3(A_asym12, b);
    EXPECT_FALSE(r_asym12.has_value());
    EXPECT_EQ(r_asym12.error(), MathError::invalid_argument);

    // Singular pivot d2
    Matrix3<double> A_sing_d2(
        2.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 2.0
    );
    auto r_sing_d2 = SolveSymmetricPositiveDefinite3x3(A_sing_d2, b);
    EXPECT_FALSE(r_sing_d2.has_value());
    EXPECT_EQ(r_sing_d2.error(), MathError::singular_matrix);

    // Singular pivot d3 (double)
    Matrix3<double> A_sing_d3(
        2.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 0.0
    );
    auto r_sing_d3 = SolveSymmetricPositiveDefinite3x3(A_sing_d3, b);
    EXPECT_FALSE(r_sing_d3.has_value());
    EXPECT_EQ(r_sing_d3.error(), MathError::singular_matrix);

    // Float versions:
    Vector3<float, TestFrame> bf(1.0f, 1.0f, 1.0f);
    Matrix3<float> Af_asym02(
        2.0f, 0.0f, 1.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 2.0f
    );
    auto rf_asym02 = SolveSymmetricPositiveDefinite3x3(Af_asym02, bf);
    EXPECT_FALSE(rf_asym02.has_value());
    EXPECT_EQ(rf_asym02.error(), MathError::invalid_argument);

    Matrix3<float> Af_asym12(
        2.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 1.0f,
        0.0f, 0.0f, 2.0f
    );
    auto rf_asym12 = SolveSymmetricPositiveDefinite3x3(Af_asym12, bf);
    EXPECT_FALSE(rf_asym12.has_value());
    EXPECT_EQ(rf_asym12.error(), MathError::invalid_argument);

    Matrix3<float> Af_sing_d2(
        2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f
    );
    auto rf_sing_d2 = SolveSymmetricPositiveDefinite3x3(Af_sing_d2, bf);
    EXPECT_FALSE(rf_sing_d2.has_value());
    EXPECT_EQ(rf_sing_d2.error(), MathError::singular_matrix);

    Matrix3<float> Af_sing_d3(
        2.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    );
    auto rf_sing_d3 = SolveSymmetricPositiveDefinite3x3(Af_sing_d3, bf);
    EXPECT_FALSE(rf_sing_d3.has_value());
    EXPECT_EQ(rf_sing_d3.error(), MathError::singular_matrix);
}

TEST(SymmetricLinearSolver3Test, FloatNonFiniteAndOverflow) {
    Matrix3<float> A_ok = Matrix3<float>::Identity();
    Vector3<float, TestFrame> b_ok(1.0f, 1.0f, 1.0f);

    // Matrix with NaN
    Matrix3<float> A_nan = A_ok;
    A_nan(0, 0) = std::numeric_limits<float>::quiet_NaN();
    auto res1 = SolveSymmetricPositiveDefinite3x3(A_nan, b_ok);
    EXPECT_FALSE(res1.has_value());
    EXPECT_EQ(res1.error(), MathError::non_finite_input);

    // Vector with Inf
    Vector3<float, TestFrame> b_inf(1.0f, std::numeric_limits<float>::infinity(), 1.0f);
    auto res2 = SolveSymmetricPositiveDefinite3x3(A_ok, b_inf);
    EXPECT_FALSE(res2.has_value());
    EXPECT_EQ(res2.error(), MathError::non_finite_input);

    // Vector with NaN in x, y, z
    Vector3<float, TestFrame> b_nan_x(std::numeric_limits<float>::quiet_NaN(), 1.0f, 1.0f);
    auto res_bx = SolveSymmetricPositiveDefinite3x3(A_ok, b_nan_x);
    EXPECT_FALSE(res_bx.has_value());
    EXPECT_EQ(res_bx.error(), MathError::non_finite_input);

    Vector3<float, TestFrame> b_nan_z(1.0f, 1.0f, std::numeric_limits<float>::quiet_NaN());
    auto res_bz = SolveSymmetricPositiveDefinite3x3(A_ok, b_nan_z);
    EXPECT_FALSE(res_bz.has_value());
    EXPECT_EQ(res_bz.error(), MathError::non_finite_input);

    // Float overflow
    Matrix3<float> A_coupled(
        1.0f, -0.999f, 0.0f,
        -0.999f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    Vector3<float, TestFrame> b_overflow(1e38f, 0.0f, 0.0f);
    auto res_huge = SolveSymmetricPositiveDefinite3x3(A_coupled, b_overflow);
    EXPECT_FALSE(res_huge.has_value());
    EXPECT_EQ(res_huge.error(), MathError::ill_conditioned);
}



