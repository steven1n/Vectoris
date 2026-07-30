#include <gtest/gtest.h>
#include <cstddef>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Geometry/Point3.h"
#include "AegisMath/Geometry/Quaternion.h"
#include "AegisMath/Geometry/Transform3.h"

using namespace AegisMath::Geometry;

struct FrameA {};
struct FrameB {};
struct FrameC {};

// ============================================================================
// Phase 2.4: Quaternion Core Engine Tests
// ============================================================================

TEST(QuaternionTest, ABI_Contract) {
    using QuatD = Quaternion<double, FrameA, FrameB>;
    using QuatF = Quaternion<float, FrameA, FrameB>;

    static_assert(sizeof(QuatD) == sizeof(double) * 4, "Quaternion<double> ABI size mismatch");
    static_assert(sizeof(QuatF) == sizeof(float) * 4, "Quaternion<float> ABI size mismatch");

    static_assert(offsetof(QuatD, w) == 0, "ABI offset mismatch");
    static_assert(offsetof(QuatD, x) == sizeof(double), "ABI offset mismatch");
    static_assert(offsetof(QuatD, y) == sizeof(double) * 2, "ABI offset mismatch");
    static_assert(offsetof(QuatD, z) == sizeof(double) * 3, "ABI offset mismatch");
}

TEST(QuaternionTest, CascadingOrder_Q002_Fix) {
    auto result_AB = Quaternion<double, FrameA, FrameB>::TryCreate(0.70710678, 0.0, 0.0, 0.70710678);
    ASSERT_TRUE(result_AB.IsSuccess());
    auto q_AB = result_AB.Value();

    auto result_BC = Quaternion<double, FrameB, FrameC>::TryCreate(0.70710678, 0.0, 0.70710678, 0.0);
    ASSERT_TRUE(result_BC.IsSuccess());
    auto q_BC = result_BC.Value();

    // 级联顺序：A -> B 接着 B -> C (即 q_AB * q_BC)
    auto q_AC = q_AB * q_BC;

    Vector3<double, FrameA> vec_A(1.0, 0.0, 0.0);
    Vector3<double, FrameC> vec_C = q_AC * vec_A;

    EXPECT_NEAR(vec_C.x, 0.0, 1e-6);
    EXPECT_NEAR(vec_C.y, 1.0, 1e-6);
    EXPECT_NEAR(vec_C.z, 0.0, 1e-6);
}

// ============================================================================
// Phase 2.5: Transform3 Rigid Body Engine Tests
// ============================================================================

TEST(Transform3Test, ABI_Contract_T003_Fix) {
    using TransD = Transform3<double, FrameA, FrameB>;

    static_assert(offsetof(TransD, originOffset_) == sizeof(Quaternion<double, FrameA, FrameB>),
                  "Transform3 padding detected between rotation and translation");

    static_assert(sizeof(TransD) == sizeof(double) * 7,
                  "Transform3 overall size must be exactly 7 doubles");
}

TEST(Transform3Test, PointVsVectorMapping_T001_Fix) {
    // 跨坐标系不能直接调 Identity()，通过 TryCreate 显式生成无旋转四元数
    auto q_ident = Quaternion<double, FrameA, FrameB>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Vector3<double, FrameB> offset(10.0, 20.0, 30.0);

    auto T_AB = Transform3<double, FrameA, FrameB>::Create(q_ident, offset);

    Point3<double, FrameA> p_A(1.0, 1.0, 1.0);
    Point3<double, FrameB> p_B = T_AB * p_A;

    EXPECT_DOUBLE_EQ(p_B.x, 11.0);
    EXPECT_DOUBLE_EQ(p_B.y, 21.0);
    EXPECT_DOUBLE_EQ(p_B.z, 31.0);

    Vector3<double, FrameA> v_A(1.0, 1.0, 1.0);
    Vector3<double, FrameB> v_B = T_AB * v_A;

    EXPECT_DOUBLE_EQ(v_B.x, 1.0);
    EXPECT_DOUBLE_EQ(v_B.y, 1.0);
    EXPECT_DOUBLE_EQ(v_B.z, 1.0);
}

TEST(Transform3Test, CompositionAndInverse) {
    auto q1_res = Quaternion<double, FrameA, FrameB>::TryCreate(0.707106, 0.707106, 0.0, 0.0);
    auto q2_res = Quaternion<double, FrameB, FrameC>::TryCreate(0.707106, 0.0, 0.707106, 0.0);

    auto T_AB = Transform3<double, FrameA, FrameB>::Create(q1_res.Value(), Vector3<double, FrameB>(1, 0, 0));
    auto T_BC = Transform3<double, FrameB, FrameC>::Create(q2_res.Value(), Vector3<double, FrameC>(0, 1, 0));

    // 级联顺序：T_AB * T_BC 得到 T_AC
    auto T_AC = T_AB * T_BC;

    Point3<double, FrameC> p_C(5.0, 5.0, 5.0);
    
    auto T_CA = T_AC.Inverse();
    Point3<double, FrameA> p_A = T_CA * p_C;
    Point3<double, FrameC> p_C_restored = T_AC * p_A;

    EXPECT_NEAR(p_C.x, p_C_restored.x, 1e-5);
    EXPECT_NEAR(p_C.y, p_C_restored.y, 1e-5);
    EXPECT_NEAR(p_C.z, p_C_restored.z, 1e-5);
}