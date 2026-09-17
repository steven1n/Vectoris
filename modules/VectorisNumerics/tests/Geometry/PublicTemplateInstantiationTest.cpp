#include <gtest/gtest.h>
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Geometry/UnitVector3.h"
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/RotationMatrix3.h"
#include "Vectoris/Numerics/Geometry/Transform3.h"

struct TestFrameA {};
struct TestFrameB {};

TEST(GeometryPublicTemplateTest, Vector3DotProduct) {
    vectoris::numerics::Geometry::Vector3<double, TestFrameA> v1(1.0, 2.0, 3.0);
    vectoris::numerics::Geometry::Vector3<double, TestFrameA> v2(4.0, 5.0, 6.0);
    double d = v1.dot(v2);
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    EXPECT_DOUBLE_EQ(d, 32.0);
}

TEST(GeometryPublicTemplateTest, UnitVector3TryCreate) {
    vectoris::numerics::Geometry::Vector3<double, TestFrameA> v(3.0, 4.0, 0.0);

    // 1. Result-based factory
    auto uv_res = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(v);
    ASSERT_TRUE(uv_res.IsSuccess());
    auto uv = uv_res.Value();
    EXPECT_NEAR(uv.x, 0.6, 1e-12);
    EXPECT_NEAR(uv.y, 0.8, 1e-12);
    EXPECT_NEAR(uv.z, 0.0, 1e-12);
    EXPECT_TRUE(uv.IsValid());

    // 2. Output-parameter factory
    vectoris::numerics::Geometry::UnitVector3<double, TestFrameA> uv_out = uv;
    bool ok = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(v, uv_out);
    EXPECT_TRUE(ok);
    EXPECT_NEAR(uv_out.x, 0.6, 1e-12);
    EXPECT_NEAR(uv_out.y, 0.8, 1e-12);

    // 3. Zero vector rejection
    vectoris::numerics::Geometry::Vector3<double, TestFrameA> zero_v(0.0, 0.0, 0.0);
    auto zero_res = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(zero_v);
    EXPECT_FALSE(zero_res.IsSuccess());
    EXPECT_EQ(zero_res.error(), vectoris::numerics::Core::MathError::zero_norm);

    // Non-finite vector rejection (x, y, z)
    vectoris::numerics::Geometry::Vector3<double, TestFrameA> nan_v(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0
    );
    auto nan_res = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(nan_v);
    EXPECT_FALSE(nan_res.IsSuccess());
    EXPECT_EQ(nan_res.error(), vectoris::numerics::Core::MathError::non_finite_input);

    vectoris::numerics::Geometry::Vector3<double, TestFrameA> nan_vy(0.0, std::numeric_limits<double>::quiet_NaN(), 0.0);
    EXPECT_FALSE((vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(nan_vy).IsSuccess()));

    vectoris::numerics::Geometry::Vector3<double, TestFrameA> nan_vz(0.0, 0.0, std::numeric_limits<double>::quiet_NaN());
    EXPECT_FALSE((vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(nan_vz).IsSuccess()));

    bool fail_ok = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(zero_v, uv_out);
    EXPECT_FALSE(fail_ok);

    // 4. Dot product with UnitVector3 and Vector3
    double d_self = uv.dot(uv);
    EXPECT_NEAR(d_self, 1.0, 1e-12);

    double d_vec = uv.dot(v);
    // 0.6*3 + 0.8*4 = 1.8 + 3.2 = 5.0 (norm of v)
    EXPECT_NEAR(d_vec, 5.0, 1e-12);
}

TEST(GeometryPublicTemplateTest, Matrix3FrobeniusNormSquared) {
    auto ident = vectoris::numerics::Geometry::Matrix3<double>::Identity();
    // 1^2 + 1^2 + 1^2 = 3
    EXPECT_DOUBLE_EQ(ident.frobenius_norm_squared(), 3.0);

    vectoris::numerics::Geometry::Matrix3<double> m(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0
    );
    // 1+4+9+16+25+36+49+64+81 = 285
    EXPECT_DOUBLE_EQ(m.frobenius_norm_squared(), 285.0);
}

TEST(GeometryPublicTemplateTest, RotationMatrix3TryCreate) {
    auto ident = vectoris::numerics::Geometry::Matrix3<double>::Identity();
    auto rot_res = vectoris::numerics::Geometry::RotationMatrix3<double, TestFrameA, TestFrameB>::TryCreate(ident);
    ASSERT_TRUE(rot_res.IsSuccess());

    // Non-orthogonal matrix rejection
    vectoris::numerics::Geometry::Matrix3<double> bad_mat(
        1.0, 2.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto bad_res = vectoris::numerics::Geometry::RotationMatrix3<double, TestFrameA, TestFrameB>::TryCreate(bad_mat);
    EXPECT_FALSE(bad_res.IsSuccess());
    EXPECT_EQ(bad_res.error(), vectoris::numerics::Core::MathError::invalid_state);

    // Non-finite matrix rejection
    vectoris::numerics::Geometry::Matrix3<double> nan_mat(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    );
    auto nan_mat_res = vectoris::numerics::Geometry::RotationMatrix3<double, TestFrameA, TestFrameB>::TryCreate(nan_mat);
    EXPECT_FALSE(nan_mat_res.IsSuccess());
    EXPECT_EQ(nan_mat_res.error(), vectoris::numerics::Core::MathError::non_finite_input);
}

TEST(GeometryPublicTemplateTest, Transform3Identity) {
    auto t_id = vectoris::numerics::Geometry::Transform3<double, TestFrameA, TestFrameA>::Identity();

    vectoris::numerics::Geometry::Point3<double, TestFrameA> p(1.0, 2.0, 3.0);
    auto p_trans = t_id * p;
    EXPECT_DOUBLE_EQ(p_trans.x, 1.0);
    EXPECT_DOUBLE_EQ(p_trans.y, 2.0);
    EXPECT_DOUBLE_EQ(p_trans.z, 3.0);

    vectoris::numerics::Geometry::Vector3<double, TestFrameA> v(4.0, 5.0, 6.0);
    auto v_trans = t_id * v;
    EXPECT_DOUBLE_EQ(v_trans.x, 4.0);
    EXPECT_DOUBLE_EQ(v_trans.y, 5.0);
    EXPECT_DOUBLE_EQ(v_trans.z, 6.0);
}
