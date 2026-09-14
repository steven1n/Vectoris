#include <gtest/gtest.h>
#include <type_traits>
#include "AegisMath/Geometry/AlmostEqual.h"
#include "AegisMath/Geometry/SymmetricLinearSolver3.h"

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

    EXPECT_TRUE(AlmostEqual(p1, p2));
    EXPECT_TRUE(AlmostEqual(p1, p3, 1e-2, 1e-2));
    EXPECT_FALSE(AlmostEqual(p1, p3, 1e-4, 1e-4));
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
}

// ----------------------------------------------------------------------------
// 7. Quaternion Exact Equality, AlmostEqual & SO(3) Rotational Equivalence
// ----------------------------------------------------------------------------
TEST(GeometryComparisonTest, QuaternionEqualityAndRotationalEquivalence) {
    auto q1 = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    auto q2 = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    // -q represents the exact same physical rotation in SO(3)
    auto q_neg = Quaternion<double, FrameA, FrameB>::TryCreate(-1.0, 0.0, 0.0, 0.0).Value();

    EXPECT_TRUE(q1 == q2);
    EXPECT_FALSE(q1 != q2);

    // TryCreate automatically canonicalizes (w >= 0), so q_neg canonicalized has w = +1
    EXPECT_TRUE(RotationEquivalent(q1, q2));
    EXPECT_TRUE(RotationEquivalent(q1, q_neg));
    EXPECT_TRUE(AlmostEqual(q1, q2));

    // Non-canonical quaternion test: manually inverted rotation
    auto q_rot = Quaternion<double, FrameA, FrameB>::TryCreate(0.70710678, 0.70710678, 0.0, 0.0).Value();
    auto q_rot_clone = Quaternion<double, FrameA, FrameB>::TryCreate(0.70710678, 0.70710678, 0.0, 0.0).Value();
    EXPECT_TRUE(RotationEquivalent(q_rot, q_rot_clone));
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
}
