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
}
