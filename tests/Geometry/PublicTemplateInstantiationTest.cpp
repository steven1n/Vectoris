#include <gtest/gtest.h>
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Geometry/Point3.h"
#include "AegisMath/Geometry/UnitVector3.h"
#include "AegisMath/Geometry/Matrix3.h"
#include "AegisMath/Geometry/RotationMatrix3.h"
#include "AegisMath/Geometry/Transform3.h"

struct TestFrameA {};
struct TestFrameB {};

TEST(GeometryPublicTemplateTest, Vector3DotProduct) {
    AegisMath::Geometry::Vector3<double, TestFrameA> v1(1.0, 2.0, 3.0);
    AegisMath::Geometry::Vector3<double, TestFrameA> v2(4.0, 5.0, 6.0);
    double d = v1.dot(v2);
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    EXPECT_DOUBLE_EQ(d, 32.0);
}

TEST(GeometryPublicTemplateTest, UnitVector3TryCreate) {
    AegisMath::Geometry::Vector3<double, TestFrameA> v(3.0, 4.0, 0.0);

    // 1. Result-based factory
    auto uv_res = AegisMath::Geometry::UnitVector3<double, TestFrameA>::TryCreate(v);
    ASSERT_TRUE(uv_res.IsSuccess());
    auto uv = uv_res.Value();
    EXPECT_NEAR(uv.x, 0.6, 1e-12);
    EXPECT_NEAR(uv.y, 0.8, 1e-12);
    EXPECT_NEAR(uv.z, 0.0, 1e-12);
    EXPECT_TRUE(uv.IsValid());

    // 2. Output-parameter factory
    AegisMath::Geometry::UnitVector3<double, TestFrameA> uv_out = uv;
    bool ok = AegisMath::Geometry::UnitVector3<double, TestFrameA>::TryCreate(v, uv_out);
    EXPECT_TRUE(ok);
    EXPECT_NEAR(uv_out.x, 0.6, 1e-12);
    EXPECT_NEAR(uv_out.y, 0.8, 1e-12);

    // 3. Zero vector rejection
    AegisMath::Geometry::Vector3<double, TestFrameA> zero_v(0.0, 0.0, 0.0);
    auto zero_res = AegisMath::Geometry::UnitVector3<double, TestFrameA>::TryCreate(zero_v);
    EXPECT_FALSE(zero_res.IsSuccess());

    // 4. Dot product with UnitVector3 and Vector3
    double d_self = uv.dot(uv);
    EXPECT_NEAR(d_self, 1.0, 1e-12);

    double d_vec = uv.dot(v);
    // 0.6*3 + 0.8*4 = 1.8 + 3.2 = 5.0 (norm of v)
    EXPECT_NEAR(d_vec, 5.0, 1e-12);
}

TEST(GeometryPublicTemplateTest, Matrix3FrobeniusNormSquared) {
    auto ident = AegisMath::Geometry::Matrix3<double>::Identity();
    // 1^2 + 1^2 + 1^2 = 3
    EXPECT_DOUBLE_EQ(ident.frobenius_norm_squared(), 3.0);

    AegisMath::Geometry::Matrix3<double> m(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0
    );
    // 1+4+9+16+25+36+49+64+81 = 285
    EXPECT_DOUBLE_EQ(m.frobenius_norm_squared(), 285.0);
}

TEST(GeometryPublicTemplateTest, RotationMatrix3TryCreate) {
    auto ident = AegisMath::Geometry::Matrix3<double>::Identity();
    auto rot_res = AegisMath::Geometry::RotationMatrix3<double, TestFrameA, TestFrameB>::TryCreate(ident);
    ASSERT_TRUE(rot_res.IsSuccess());

    // Non-orthogonal matrix rejection
    AegisMath::Geometry::Matrix3<double> bad_mat(
        1.0, 2.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto bad_res = AegisMath::Geometry::RotationMatrix3<double, TestFrameA, TestFrameB>::TryCreate(bad_mat);
    EXPECT_FALSE(bad_res.IsSuccess());
}

TEST(GeometryPublicTemplateTest, Transform3Identity) {
    auto t_id = AegisMath::Geometry::Transform3<double, TestFrameA, TestFrameA>::Identity();

    AegisMath::Geometry::Point3<double, TestFrameA> p(1.0, 2.0, 3.0);
    auto p_trans = t_id * p;
    EXPECT_DOUBLE_EQ(p_trans.x, 1.0);
    EXPECT_DOUBLE_EQ(p_trans.y, 2.0);
    EXPECT_DOUBLE_EQ(p_trans.z, 3.0);

    AegisMath::Geometry::Vector3<double, TestFrameA> v(4.0, 5.0, 6.0);
    auto v_trans = t_id * v;
    EXPECT_DOUBLE_EQ(v_trans.x, 4.0);
    EXPECT_DOUBLE_EQ(v_trans.y, 5.0);
    EXPECT_DOUBLE_EQ(v_trans.z, 6.0);
}
