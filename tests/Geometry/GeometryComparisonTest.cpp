#include <gtest/gtest.h>
#include <type_traits>
#include <cmath>
#include <numbers>
#include <limits>
#include "AegisMath/Geometry/AlmostEqual.h"
#include "AegisMath/Geometry/SymmetricLinearSolver3.h"
#include "AegisMath/Core/MathError.h"

using namespace AegisMath::Core;
using namespace AegisMath::Geometry;

namespace {
    struct FrameA {};
    struct FrameB {};
    struct FrameC {};
    struct FrameD {};
}

// ----------------------------------------------------------------------------
// 1. Vector3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, Vector3EqualityAndAlmostEqual) {
    Vector3<double, FrameA> v1(1.0, 2.0, 3.0);
    Vector3<double, FrameA> v2(1.0, 2.0, 3.0);
    Vector3<double, FrameA> v3(1.0, 2.0, 3.001);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);

    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);

    EXPECT_TRUE(AlmostEqual(v1, v2));
    EXPECT_TRUE(AlmostEqual(v1, v3, 1e-2, 1e-2));
    EXPECT_FALSE(AlmostEqual(v1, v3, 1e-4, 1e-4));

    // Vector - Vector
    Vector3<double, FrameA> diff = v1 - v2;
    EXPECT_DOUBLE_EQ(diff.x, 0.0);
    EXPECT_DOUBLE_EQ(diff.y, 0.0);
    EXPECT_DOUBLE_EQ(diff.z, 0.0);

    Vector3<double, FrameA> vx(2.0, 2.0, 3.0);
    Vector3<double, FrameA> vy(1.0, 3.0, 3.0);
    Vector3<double, FrameA> vz(1.0, 2.0, 4.0);
    EXPECT_FALSE(AlmostEqual(v1, vx, 1e-4, 1e-4));
    EXPECT_FALSE(AlmostEqual(v1, vy, 1e-4, 1e-4));
    EXPECT_FALSE(AlmostEqual(v1, vz, 1e-4, 1e-4));

    // Phase 3 Regression: operator== is exact component-wise value equality, NOT bitwise equality
    // +0.0 == -0.0 evaluates to true under C++ floating-point == semantics despite differing sign bits
    Vector3<double, FrameA> pz(+0.0, 1.0, 2.0);
    Vector3<double, FrameA> nz(-0.0, 1.0, 2.0);
    EXPECT_TRUE(pz == nz);
    EXPECT_FALSE(pz != nz);

    Vector3<double, FrameA> v_diff_y(1.0, 9.0, 3.0);
    EXPECT_FALSE(v1 == v_diff_y);
    Vector3<double, FrameA> v_diff_z(1.0, 2.0, 9.0);
    EXPECT_FALSE(v1 == v_diff_z);

    // NaN != NaN under standard IEEE-754 / C++ floating-point == semantics
    Vector3<double, FrameA> nan_vec(std::numeric_limits<double>::quiet_NaN(), 1.0, 2.0);
    EXPECT_FALSE(nan_vec == nan_vec);
    EXPECT_TRUE(nan_vec != nan_vec);
}

// ----------------------------------------------------------------------------
// 2. Point3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, Point3EqualityAndAlmostEqual) {
    Point3<double, FrameA> p1(1.0, 2.0, 3.0);
    Point3<double, FrameA> p2(1.0, 2.0, 3.0);
    Point3<double, FrameA> p3(1.0, 2.0, 3.001);

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 != p2);

    EXPECT_FALSE(p1 == p3);
    EXPECT_TRUE(p1 != p3);

    // Component-wise operator== variations
    Point3<double, FrameA> p_diff_x(9.0, 2.0, 3.0);
    EXPECT_FALSE(p1 == p_diff_x);
    Point3<double, FrameA> p_diff_y(1.0, 5.0, 3.0);
    EXPECT_FALSE(p1 == p_diff_y);
    Point3<double, FrameA> p_diff_z(1.0, 2.0, 5.0);
    EXPECT_FALSE(p1 == p_diff_z);

    EXPECT_TRUE(AlmostEqual(p1, p2));
    EXPECT_TRUE(AlmostEqual(p1, p3, 1e-2, 1e-2));
    EXPECT_FALSE(AlmostEqual(p1, p3, 1e-4, 1e-4));

    // Point + Vector -> Point
    Vector3<double, FrameA> v_shift(10.0, 20.0, 30.0);
    Point3<double, FrameA> p_sum = p1 + v_shift;
    EXPECT_DOUBLE_EQ(p_sum.x, 11.0);
    EXPECT_DOUBLE_EQ(p_sum.y, 22.0);
    EXPECT_DOUBLE_EQ(p_sum.z, 33.0);

    // Point - Point -> Vector
    Vector3<double, FrameA> p_diff = p3 - p1;
    EXPECT_DOUBLE_EQ(p_diff.x, 0.0);
    EXPECT_DOUBLE_EQ(p_diff.y, 0.0);
    EXPECT_NEAR(p_diff.z, 0.001, 1e-12);

    Point3<double, FrameA> px(2.0, 2.0, 3.0);
    Point3<double, FrameA> py(1.0, 3.0, 3.0);
    Point3<double, FrameA> pz_diff(1.0, 2.0, 4.0);
    EXPECT_FALSE(AlmostEqual(p1, px, 1e-4, 1e-4));
    EXPECT_FALSE(AlmostEqual(p1, py, 1e-4, 1e-4));
    EXPECT_FALSE(AlmostEqual(p1, pz_diff, 1e-4, 1e-4));
}

// ----------------------------------------------------------------------------
// 3. Matrix3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, Matrix3EqualityAndAlmostEqual) {
    Matrix3<double> m1(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0
    );
    Matrix3<double> m2 = m1;
    Matrix3<double> m3(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.001
    );

    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 != m2);

    EXPECT_FALSE(m1 == m3);
    EXPECT_TRUE(m1 != m3);

    EXPECT_TRUE(m1.AlmostEqual(m2));
    EXPECT_TRUE(AlmostEqual(m1, m2));

    EXPECT_TRUE(AlmostEqual(m1, m3, 1e-2, 1e-2));
    EXPECT_FALSE(AlmostEqual(m1, m3, 1e-4, 1e-4));

    // Element-wise operator== and AlmostEqual variations across all 9 entries
    for (size_t i = 0; i < 9; ++i) {
        Matrix3<double> m_diff = m1;
        m_diff.m[i] += 10.0;
        EXPECT_FALSE(m1 == m_diff);
        EXPECT_TRUE(m1 != m_diff);
        EXPECT_FALSE(AlmostEqual(m1, m_diff, 1e-4, 1e-4));
    }

    // Singular matrix inverse returning false
    Matrix3<double> m_sing = Matrix3<double>::Zero();
    Matrix3<double> m_inv;
    EXPECT_FALSE(m_sing.TryInverse(m_inv));

    // Rank-deficient matrix inverse returning false
    Matrix3<double> m_rank1(
        1.0, 1.0, 1.0,
        1.0, 1.0, 1.0,
        1.0, 1.0, 1.0
    );
    EXPECT_FALSE(m_rank1.TryInverse(m_inv));

    // Non-finite matrix entries in TryInverse
    Matrix3<double> m_nan(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    EXPECT_FALSE(m_nan.TryInverse(m_inv));

    Matrix3<double> m_inf(
        std::numeric_limits<double>::infinity(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    EXPECT_FALSE(m_inf.TryInverse(m_inv));

    // Near-singular matrix in TryInverse
    Matrix3<double> m_near_sing(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1e-18
    );
    EXPECT_FALSE(m_near_sing.TryInverse(m_inv));

    // Negative determinant matrix in TryInverse (covers d < 0 branch at line 157)
    Matrix3<double> m_neg_det(
        -1.0, 0.0, 0.0,
         0.0, 1.0, 0.0,
         0.0, 0.0, 1.0
    );
    EXPECT_TRUE(m_neg_det.TryInverse(m_inv));
}

// ----------------------------------------------------------------------------
// 4. UnitVector3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, UnitVector3EqualityAndAlmostEqual) {
    auto u1_res = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(1.0, 0.0, 0.0));
    ASSERT_TRUE(u1_res.IsSuccess());
    auto u1 = u1_res.Value();

    auto u2_res = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(2.0, 0.0, 0.0));
    ASSERT_TRUE(u2_res.IsSuccess());
    auto u2 = u2_res.Value();

    EXPECT_TRUE(u1 == u2);
    EXPECT_FALSE(u1 != u2);
    EXPECT_TRUE(AlmostEqual(u1, u2));

    // Non-finite input (x, y, z individually)
    auto uv_nan_x = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0));
    ASSERT_FALSE(uv_nan_x.IsSuccess());
    EXPECT_EQ(uv_nan_x.error(), MathError::non_finite_input);

    auto uv_nan_y = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, std::numeric_limits<double>::quiet_NaN(), 0.0));
    ASSERT_FALSE(uv_nan_y.IsSuccess());
    EXPECT_EQ(uv_nan_y.error(), MathError::non_finite_input);

    auto uv_nan_z = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 0.0, std::numeric_limits<double>::quiet_NaN()));
    ASSERT_FALSE(uv_nan_z.IsSuccess());
    EXPECT_EQ(uv_nan_z.error(), MathError::non_finite_input);

    // Zero-norm input
    auto uv_zero = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 0.0, 0.0));
    ASSERT_FALSE(uv_zero.IsSuccess());
    EXPECT_EQ(uv_zero.error(), MathError::zero_norm);

    // TryCreate with out param failure and success
    auto uv_out = u1;
    bool create_fail = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 0.0, 0.0), uv_out);
    EXPECT_FALSE(create_fail);

    bool create_ok = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 2.0, 0.0), uv_out);
    EXPECT_TRUE(create_ok);
    EXPECT_DOUBLE_EQ(uv_out.y, 1.0);

    auto u3 = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 1.0, 0.0)).Value();
    auto u4 = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 0.0, 1.0)).Value();
    EXPECT_FALSE(u1 == u3);
    EXPECT_TRUE(u1 != u3);
    EXPECT_FALSE(u1 == u4);
    auto u_z_neg = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 0.0, -1.0)).Value();
    EXPECT_FALSE(u4 == u_z_neg);
    EXPECT_FALSE(AlmostEqual(u4, u_z_neg));

    auto u_y_pos = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, 1.0, 0.0)).Value();
    auto u_y_neg = UnitVector3<double, FrameA>::TryCreate(Vector3<double, FrameA>(0.0, -1.0, 0.0)).Value();
    EXPECT_FALSE(u_y_pos == u_y_neg);
    EXPECT_FALSE(AlmostEqual(u1, u3));
    EXPECT_FALSE(AlmostEqual(u_y_pos, u_y_neg));
}

// ----------------------------------------------------------------------------
// 5. RotationMatrix3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, RotationMatrix3EqualityAndAlmostEqual) {
    auto r1 = RotationMatrix3<double, FrameA, FrameB>::Identity();
    auto r2 = RotationMatrix3<double, FrameA, FrameB>::Identity();

    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
    EXPECT_TRUE(AlmostEqual(r1, r2));

    // Invalid rotation matrix with det != 1
    Matrix3<double> m_scaled(
        2.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 2.0
    );
    auto r_inv = RotationMatrix3<double, FrameA, FrameB>::TryCreate(m_scaled);
    ASSERT_FALSE(r_inv.IsSuccess());
    EXPECT_EQ(r_inv.error(), MathError::invalid_state);

    // Mismatched rotation matrices
    auto q_rot90 = Quaternion<double, FrameA, FrameB>::TryCreate(0.7071067811865476, 0.7071067811865476, 0.0, 0.0).Value();
    auto r_diff = RotationMatrix3<double, FrameA, FrameB>::FromQuaternion(q_rot90);
    EXPECT_FALSE(r1 == r_diff);
    EXPECT_TRUE(r1 != r_diff);
    EXPECT_FALSE(AlmostEqual(r1, r_diff));
}

// ----------------------------------------------------------------------------
// 6. Transform3 Exact Equality & AlmostEqual
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, Transform3EqualityAndAlmostEqual) {
    auto t1 = Transform3<double, FrameA, FrameA>::Identity();
    auto t2 = Transform3<double, FrameA, FrameA>::Identity();

    EXPECT_TRUE(t1 == t2);
    EXPECT_FALSE(t1 != t2);
    EXPECT_TRUE(AlmostEqual(t1, t2));

    // Different rotation
    auto q_rot90_A = Quaternion<double, FrameA, FrameA>::TryCreate(0.7071067811865476, 0.7071067811865476, 0.0, 0.0).Value();
    auto t_diff_rot = Transform3<double, FrameA, FrameA>::Create(q_rot90_A, Vector3<double, FrameA>(0.0, 0.0, 0.0));
    EXPECT_FALSE(t1 == t_diff_rot);
    EXPECT_TRUE(t1 != t_diff_rot);
    EXPECT_FALSE(AlmostEqual(t1, t_diff_rot));

    // Different offset
    auto t_diff_offset = Transform3<double, FrameA, FrameA>::Create(
        Quaternion<double, FrameA, FrameA>::Identity(), Vector3<double, FrameA>(1.0, 2.0, 3.0)
    );
    EXPECT_FALSE(t1 == t_diff_offset);
    EXPECT_TRUE(t1 != t_diff_offset);
    EXPECT_FALSE(AlmostEqual(t1, t_diff_offset));
}

// ----------------------------------------------------------------------------
// 7. Quaternion Exact Equality, AlmostEqual & SO(3) Rotational Equivalence
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, QuaternionEqualityAndRotationalEquivalence) {
    auto q1 = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    auto q2 = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();

    EXPECT_TRUE(q1 == q2);
    EXPECT_FALSE(q1 != q2);

    // Non-finite input across all components
    auto q_nan_w = Quaternion<double, FrameA, FrameB>::TryCreate(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0);
    ASSERT_FALSE(q_nan_w.IsSuccess());
    EXPECT_EQ(q_nan_w.error(), MathError::non_finite_input);

    auto q_nan_x = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    ASSERT_FALSE(q_nan_x.IsSuccess());
    EXPECT_EQ(q_nan_x.error(), MathError::non_finite_input);

    auto q_nan_y = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, std::numeric_limits<double>::quiet_NaN(), 0.0);
    ASSERT_FALSE(q_nan_y.IsSuccess());
    EXPECT_EQ(q_nan_y.error(), MathError::non_finite_input);

    auto q_nan_z = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, std::numeric_limits<double>::quiet_NaN());
    ASSERT_FALSE(q_nan_z.IsSuccess());
    EXPECT_EQ(q_nan_z.error(), MathError::non_finite_input);

    // Zero-norm input
    auto q_zero = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, 0.0, 0.0, 0.0);
    ASSERT_FALSE(q_zero.IsSuccess());
    EXPECT_EQ(q_zero.error(), MathError::zero_norm);

    // Audit Canonicalization: Public TryCreate normalizes sign to w >= 0
    // TryCreate(-1, 0, 0, 0) collapses to w = +1.0 canonical representative
    auto q_neg_input = Quaternion<double, FrameA, FrameB>::TryCreate(-1.0, 0.0, 0.0, 0.0).Value();
    EXPECT_DOUBLE_EQ(q_neg_input.w, 1.0);
    EXPECT_TRUE(q1 == q_neg_input);

    // Phase 7: Nontrivial rotation case (90 degrees about Y axis, theta != 0)
    const double half_angle = std::numbers::pi / 4.0;
    auto q_rot = Quaternion<double, FrameA, FrameB>::TryCreate(
        std::cos(half_angle), 0.0, std::sin(half_angle), 0.0
    ).Value();

    // Canonicalized with negative w
    auto minus_q = q_rot;
    minus_q.w = -q_rot.w;
    minus_q.x = -q_rot.x;
    minus_q.y = -q_rot.y;
    minus_q.z = -q_rot.z;
    auto q_canon = minus_q.Canonicalized();
    EXPECT_GE(q_canon.w, 0.0);

    // Slerp non-finite t
    auto slerp_nan = q1.Slerp(q2, std::numeric_limits<double>::quiet_NaN());
    ASSERT_FALSE(slerp_nan.IsSuccess());
    EXPECT_EQ(slerp_nan.error(), MathError::non_finite_input);

    // Slerp identical quaternions (small angle linear path)
    auto slerp_same = q1.Slerp(q1, 0.5);
    ASSERT_TRUE(slerp_same.IsSuccess());
    EXPECT_DOUBLE_EQ(slerp_same.Value().w, q1.w);

    // Slerp spherical interpolation path (nontrivial angle ~ 60 degrees)
    auto q_spherical_target = Quaternion<double, FrameA, FrameB>::TryCreate(
        std::cos(std::numbers::pi / 6.0), std::sin(std::numbers::pi / 6.0), 0.0, 0.0
    ).Value();
    auto slerp_spherical = q1.Slerp(q_spherical_target, 0.5);
    ASSERT_TRUE(slerp_spherical.IsSuccess());
    EXPECT_NEAR(slerp_spherical.Value().w, std::cos(std::numbers::pi / 12.0), 1e-12);

    // Slerp negative cos_theta path (dot product < 0)
    auto q_pos = Quaternion<double, FrameA, FrameB>::TryCreate(0.5, 0.5, 0.5, 0.5).Value();
    auto q_neg = Quaternion<double, FrameA, FrameB>::TryCreate(0.5, -0.5, -0.5, -0.5).Value();
    auto slerp_neg_cos = q_pos.Slerp(q_neg, 0.5);
    ASSERT_TRUE(slerp_neg_cos.IsSuccess());

    // 1. Exact value equality fails:
    EXPECT_FALSE(q_rot == minus_q);
    EXPECT_TRUE(q_rot != minus_q);

    // Component-wise operator== and AlmostEqual variations
    auto q_base = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.8, 0.0, 0.0).Value();
    auto q_diff_w = Quaternion<double, FrameA, FrameB>::TryCreate(0.8, 0.6, 0.0, 0.0).Value();
    auto q_diff_y = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, 0.8, 0.0).Value();
    auto q_diff_z = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, 0.0, 0.8).Value();

    EXPECT_FALSE(q_base == q_diff_w);
    EXPECT_FALSE(q_base == q_diff_y);
    EXPECT_FALSE(q_base == q_diff_z);
    EXPECT_FALSE(AlmostEqual(q_base, q_diff_w));
    EXPECT_FALSE(AlmostEqual(q_base, q_diff_y));
    EXPECT_FALSE(AlmostEqual(q_base, q_diff_z));

    auto q_wy1 = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, 0.8, 0.0).Value();
    auto q_wy2 = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, -0.8, 0.0).Value();
    EXPECT_FALSE(q_wy1 == q_wy2);
    EXPECT_FALSE(AlmostEqual(q_wy1, q_wy2));

    auto q_wz1 = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, 0.0, 0.8).Value();
    auto q_wz2 = Quaternion<double, FrameA, FrameB>::TryCreate(0.6, 0.0, 0.0, -0.8).Value();
    EXPECT_FALSE(q_wz1 == q_wz2);
    EXPECT_FALSE(AlmostEqual(q_wz1, q_wz2));

    // 2. Tolerance-aware component-wise AlmostEqual fails:
    EXPECT_FALSE(AlmostEqual(q_rot, minus_q));
    EXPECT_FALSE(AlmostEqual(q1, q_rot));

    // 3. SO(3) Rotational Equivalence succeeds (verifying the double cover q ~ -q):
    EXPECT_TRUE(RotationEquivalent(q_rot, minus_q));
    EXPECT_TRUE(RotationEquivalent(q_base, q_base));

    // Double cover 180-degree pure vector rotation equivalence
    auto q_180_x = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, 1.0, 0.0, 0.0).Value();
    auto q_180_neg_x = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, -1.0, 0.0, 0.0).Value();
    EXPECT_TRUE(RotationEquivalent(q_180_x, q_180_neg_x));

    // RotationEquivalent failures
    EXPECT_FALSE(RotationEquivalent(q_base, q_180_x));
    EXPECT_FALSE(RotationEquivalent(q_180_x, q_diff_y));
    EXPECT_FALSE(RotationEquivalent(q_180_x, q_diff_z));
    EXPECT_FALSE(RotationEquivalent(q_wz1, q_wz2));
    auto q_180_y = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, 0.0, 1.0, 0.0).Value();
    EXPECT_FALSE(RotationEquivalent(q_180_x, q_180_y));

    // RotationEquivalent line 210 and 211 false branches
    // Line 210: a.w ~ -b.w (true), a.x ~ -b.x (true), a.y ~ -b.y (false)
    const double sqrt2_inv = 0.7071067811865475;
    auto q_xy_a = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, sqrt2_inv, sqrt2_inv, 0.0).Value();
    auto q_xy_b = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, -sqrt2_inv, sqrt2_inv, 0.0).Value();
    EXPECT_FALSE(RotationEquivalent(q_xy_a, q_xy_b));

    // Line 211: a.w ~ -b.w (true), a.x ~ -b.x (true), a.y ~ -b.y (true), a.z ~ -b.z (false)
    auto q_xz_a = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, sqrt2_inv, 0.0, sqrt2_inv).Value();
    auto q_xz_b = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, -sqrt2_inv, 0.0, sqrt2_inv).Value();
    EXPECT_FALSE(RotationEquivalent(q_xz_a, q_xz_b));

    // 4. Perturbed quaternion beyond tolerance fails:
    auto perturbed_q = q_rot;
    perturbed_q.y += 0.01;
    EXPECT_FALSE(RotationEquivalent(q_rot, perturbed_q, 1e-4, 1e-4));
    EXPECT_TRUE(RotationEquivalent(q_rot, perturbed_q, 0.05, 0.05));
}

// ----------------------------------------------------------------------------
// 8. Quaternion to RotationMatrix3 Agreement Property Test
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, QuaternionToRotationMatrixAgreement) {
    // 90-degree rotation about X-axis: q = [cos(pi/4), sin(pi/4), 0, 0]
    auto q = Quaternion<double, FrameA, FrameB>::TryCreate(0.7071067811865476, 0.7071067811865476, 0.0, 0.0).Value();
    auto R = q.ToRotationMatrix();

    // Verify factory method also works
    auto R_factory = RotationMatrix3<double, FrameA, FrameB>::FromQuaternion(q);
    EXPECT_TRUE(R == R_factory);

    // Test on multiple arbitrary vectors
    Vector3<double, FrameA> vectors[] = {
        Vector3<double, FrameA>(1.0, 0.0, 0.0),
        Vector3<double, FrameA>(0.0, 1.0, 0.0),
        Vector3<double, FrameA>(0.0, 0.0, 1.0),
        Vector3<double, FrameA>(2.5, -3.1, 7.8)
    };

    for (const auto& v : vectors) {
        Vector3<double, FrameB> v_from_q = q * v;
        Vector3<double, FrameB> v_from_R = R * v;
        EXPECT_TRUE(AlmostEqual(v_from_q, v_from_R, 1e-12, 1e-12));
    }
}

// ----------------------------------------------------------------------------
// 9. RotationMatrix3 Composition Property Test
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, RotationCompositionProperty) {
    // Phase 9: Verify composition result type statically matches RotationMatrix3<double, FrameA, FrameC>
    static_assert(std::same_as<
        decltype(
            std::declval<RotationMatrix3<double, FrameA, FrameB>>() *
            std::declval<RotationMatrix3<double, FrameB, FrameC>>()
        ),
        RotationMatrix3<double, FrameA, FrameC>
    >, "Composition result type must evaluate to RotationMatrix3<T, FrameA, FrameC>");

    // R_AB: 90 deg about Z
    auto q_AB = Quaternion<double, FrameA, FrameB>::TryCreate(0.7071067811865476, 0.0, 0.0, 0.7071067811865476).Value();
    auto R_AB = q_AB.ToRotationMatrix();

    // R_BC: 90 deg about Y
    auto q_BC = Quaternion<double, FrameB, FrameC>::TryCreate(0.7071067811865476, 0.0, 0.7071067811865476, 0.0).Value();
    auto R_BC = q_BC.ToRotationMatrix();

    // Pipeline composition: R_AC = R_AB * R_BC
    auto R_AC = R_AB * R_BC;

    Vector3<double, FrameA> v_A(1.0, 2.0, 3.0);
    Vector3<double, FrameC> v_C_composed = R_AC * v_A;
    Vector3<double, FrameC> v_C_stepwise = R_BC * (R_AB * v_A);

    EXPECT_TRUE(AlmostEqual(v_C_composed, v_C_stepwise, 1e-14, 1e-14));
}

// ----------------------------------------------------------------------------
// 10. Compile-Time Frame Safety Negative Assertions
// ----------------------------------------------------------------------------
namespace {
    template <typename T, typename U>
    concept CanAdd = requires(T t, U u) { t + u; };

    template <typename T, typename U>
    concept CanSubtract = requires(T t, U u) { t - u; };

    template <typename T, typename U>
    concept CanMultiply = requires(T t, U u) { t * u; };

    template <typename T, typename U>
    concept CanDot = requires(T t, U u) { t.dot(u); };

    // Point + Vector
    static_assert(CanAdd<Point3<double, FrameA>, Vector3<double, FrameA>>);
    static_assert(!CanAdd<Point3<double, FrameA>, Vector3<double, FrameB>>,
        "Cross-frame Point + Vector must be rejected at compile time");

    // Point - Point
    static_assert(CanSubtract<Point3<double, FrameA>, Point3<double, FrameA>>);
    static_assert(!CanSubtract<Point3<double, FrameA>, Point3<double, FrameB>>,
        "Cross-frame Point - Point must be rejected at compile time");

    // Vector + Vector
    static_assert(CanAdd<Vector3<double, FrameA>, Vector3<double, FrameA>>);
    static_assert(!CanAdd<Vector3<double, FrameA>, Vector3<double, FrameB>>,
        "Cross-frame Vector + Vector must be rejected at compile time");

    // Vector - Vector
    static_assert(CanSubtract<Vector3<double, FrameA>, Vector3<double, FrameA>>);
    static_assert(!CanSubtract<Vector3<double, FrameA>, Vector3<double, FrameB>>,
        "Cross-frame Vector - Vector must be rejected at compile time");

    // Vector dot Vector
    static_assert(CanDot<Vector3<double, FrameA>, Vector3<double, FrameA>>);
    static_assert(!CanDot<Vector3<double, FrameA>, Vector3<double, FrameB>>,
        "Cross-frame Vector dot Vector must be rejected at compile time");

    // RotationMatrix * Vector
    static_assert(CanMultiply<RotationMatrix3<double, FrameA, FrameB>, Vector3<double, FrameA>>);
    static_assert(!CanMultiply<RotationMatrix3<double, FrameA, FrameB>, Vector3<double, FrameB>>,
        "Mismatched RotationMatrix * Vector must be rejected at compile time");
    static_assert(!CanMultiply<RotationMatrix3<double, FrameA, FrameB>, Vector3<double, FrameC>>,
        "Cross-frame RotationMatrix * Vector must be rejected at compile time");

    // RotationMatrix * RotationMatrix
    static_assert(CanMultiply<RotationMatrix3<double, FrameA, FrameB>, RotationMatrix3<double, FrameB, FrameC>>);
    static_assert(!CanMultiply<RotationMatrix3<double, FrameA, FrameB>, RotationMatrix3<double, FrameC, FrameD>>,
        "Mismatched RotationMatrix cascade must be rejected at compile time");

    // Quaternion * Vector
    static_assert(CanMultiply<Quaternion<double, FrameA, FrameB>, Vector3<double, FrameA>>);
    static_assert(!CanMultiply<Quaternion<double, FrameA, FrameB>, Vector3<double, FrameB>>,
        "Mismatched Quaternion * Vector must be rejected at compile time");

    // Generic Matrix3 remains unframed
    template <typename M>
    concept HasFrameType = requires { typename M::FrameType; };
    static_assert(!HasFrameType<Matrix3<double>>,
        "Matrix3 must not require or carry FrameTag");
}

TEST(GeometryComparisonTest, CompileTimeRejectionGuardsVerified) {
    // Run-time assertion witnessing that concept tests passed compilation
    SUCCEED();
}

// ----------------------------------------------------------------------------
// 11. Generic Matrix3 SPD Solver Unframed Interoperability
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, GenericMatrix3UnframedInteroperability) {
    // Verify SolveSymmetricPositiveDefinite3x3 accepts generic unframed Matrix3<double>
    Matrix3<double> A = Matrix3<double>::Identity();
    Vector3<double, FrameA> b(1.0, 2.0, 3.0);

    auto res = SolveSymmetricPositiveDefinite3x3(A, b);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(AlmostEqual(res.value(), b));

    // Exercise defensive error paths on this instantiation
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(Matrix3<double>::Zero(), b).has_value());

    Matrix3<double> A_asym(
        2.0, 1.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 2.0
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_asym, b).has_value());

    Matrix3<double> A_indef1(
        -2.0, 0.0, 0.0,
         0.0, 2.0, 0.0,
         0.0, 0.0, 2.0
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_indef1, b).has_value());

    Matrix3<double> A_sing1(
        0.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 2.0
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_sing1, b).has_value());

    Matrix3<double> A_indef2(
        1.0, 2.0, 0.0,
        2.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_indef2, b).has_value());

    Matrix3<double> A_indef3(
        1.0, 0.0, 2.0,
        0.0, 1.0, 0.0,
        2.0, 0.0, 1.0
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_indef3, b).has_value());

    Matrix3<double> A_ill(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 5e-15
    );
    EXPECT_FALSE(SolveSymmetricPositiveDefinite3x3(A_ill, b).has_value());
}

// ----------------------------------------------------------------------------
// 12. Quaternion 180-Degree Edge Case (theta = pi, w = 0)
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, Quaternion180DegreeEdgeCase) {
    // 180-degree rotation about X: q = [0, 1, 0, 0] and -q = [0, -1, 0, 0]
    auto q1 = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, 1.0, 0.0, 0.0).Value();
    auto q2 = Quaternion<double, FrameA, FrameB>::TryCreate(0.0, -1.0, 0.0, 0.0).Value();

    // Under Option A, w == 0 is preserved without forced vector negation
    EXPECT_DOUBLE_EQ(q1.w, 0.0);
    EXPECT_DOUBLE_EQ(q2.w, 0.0);
    EXPECT_DOUBLE_EQ(q1.x, 1.0);
    EXPECT_DOUBLE_EQ(q2.x, -1.0);

    // Exact value equality fails between q and -q
    EXPECT_FALSE(q1 == q2);
    EXPECT_TRUE(q1 != q2);

    // Tolerance-aware component-wise AlmostEqual fails
    EXPECT_FALSE(AlmostEqual(q1, q2));

    // SO(3) Rotational Equivalence succeeds (both represent 180-deg flip about X)
    EXPECT_TRUE(RotationEquivalent(q1, q2));

    // Verify physical rotation equivalence on vectors
    Vector3<double, FrameA> v(1.0, 2.0, 3.0);
    Vector3<double, FrameB> v1 = q1 * v;
    Vector3<double, FrameB> v2 = q2 * v;
    EXPECT_TRUE(AlmostEqual(v1, v2, 1e-14, 1e-14));
}

// ----------------------------------------------------------------------------
// 13. Quaternion Near-Zero w Determinism
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, QuaternionNearZeroWDeterminism) {
    // 1. Given q and its true opposite -q with near-zero w (w = +/- 1e-16):
    // q = [+1e-16, 1, 0, 0], -q = [-1e-16, -1, 0, 0]
    auto q = Quaternion<double, FrameA, FrameB>::TryCreate(1e-16, 1.0, 0.0, 0.0).Value();
    auto minus_q = Quaternion<double, FrameA, FrameB>::TryCreate(-1e-16, -1.0, 0.0, 0.0).Value();

    // Canonicalization uses exact w < 0 to deterministically collapse sign:
    // minus_q is negated into [+1e-16, 1, 0, 0]
    EXPECT_GE(q.w, 0.0);
    EXPECT_GE(minus_q.w, 0.0);
    EXPECT_TRUE(AlmostEqual(q, minus_q, 1e-14, 1e-14));
    EXPECT_TRUE(RotationEquivalent(q, minus_q));

    // 2. Numerical sign noise on near-zero w where vector part is identical:
    // q_pos = [+1e-16, 1, 0, 0], q_noisy = [-1e-16, 1, 0, 0]
    auto q_pos = Quaternion<double, FrameA, FrameB>::TryCreate(1e-16, 1.0, 0.0, 0.0).Value();
    auto q_noisy = Quaternion<double, FrameA, FrameB>::TryCreate(-1e-16, 1.0, 0.0, 0.0).Value();

    // Canonicalization negates q_noisy into [+1e-16, -1, 0, 0]
    EXPECT_GE(q_pos.w, 0.0);
    EXPECT_GE(q_noisy.w, 0.0);

    // Component-wise AlmostEqual fails because x is +1 vs -1
    EXPECT_FALSE(AlmostEqual(q_pos, q_noisy));

    // SO(3) RotationEquivalent succeeds because both represent 180-deg flip about X axis
    EXPECT_TRUE(RotationEquivalent(q_pos, q_noisy));
}
