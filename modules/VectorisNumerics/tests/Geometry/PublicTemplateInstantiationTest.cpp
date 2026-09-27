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
    EXPECT_NEAR(uv.x(), 0.6, 1e-12);
    EXPECT_NEAR(uv.y(), 0.8, 1e-12);
    EXPECT_NEAR(uv.z(), 0.0, 1e-12);
    EXPECT_TRUE(uv.IsValid());

    // 2. Output-parameter factory
    vectoris::numerics::Geometry::UnitVector3<double, TestFrameA> uv_out = uv;
    bool ok = vectoris::numerics::Geometry::UnitVector3<double, TestFrameA>::TryCreate(v, uv_out);
    EXPECT_TRUE(ok);
    EXPECT_NEAR(uv_out.x(), 0.6, 1e-12);
    EXPECT_NEAR(uv_out.y(), 0.8, 1e-12);

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

// AFA-002: operations accepted by ScalarArithmetic may throw; their callers can catch.
namespace {
enum class AFA002Fault { None, Default, Copy, Move, Add, Subtract, Negate, Multiply, Equal };
struct AFA002Exception {};
struct AFA002Scalar {
    inline static AFA002Fault fault = AFA002Fault::None;
    int value{};
    static void check(AFA002Fault operation) { if (fault == operation) { throw AFA002Exception{}; } }
    AFA002Scalar() { check(AFA002Fault::Default); }
    explicit AFA002Scalar(int v) noexcept : value(v) {}
    AFA002Scalar(const AFA002Scalar& rhs) : value(rhs.value) { check(AFA002Fault::Copy); }
    AFA002Scalar(AFA002Scalar&& rhs) noexcept(false) : value(rhs.value) { check(AFA002Fault::Move); }
    ~AFA002Scalar() = default;
    AFA002Scalar& operator=(const AFA002Scalar&) = default;
    AFA002Scalar& operator=(AFA002Scalar&&) = default;
    friend AFA002Scalar operator+(const AFA002Scalar& a, const AFA002Scalar& b) {
        check(AFA002Fault::Add); return AFA002Scalar{a.value+b.value};
    }
    friend AFA002Scalar operator-(const AFA002Scalar& a, const AFA002Scalar& b) {
        check(AFA002Fault::Subtract); return AFA002Scalar{a.value-b.value};
    }
    friend AFA002Scalar operator-(const AFA002Scalar& a) {
        check(AFA002Fault::Negate); return AFA002Scalar{-a.value};
    }
    friend AFA002Scalar operator*(const AFA002Scalar& a, const AFA002Scalar& b) {
        check(AFA002Fault::Multiply); return AFA002Scalar{a.value*b.value};
    }
    friend AFA002Scalar operator/(const AFA002Scalar& a, const AFA002Scalar& b) {
        return AFA002Scalar{a.value/b.value};
    }
    friend bool operator==(const AFA002Scalar& a, const AFA002Scalar& b) {
        check(AFA002Fault::Equal); return a.value==b.value;
    }
};
using AFA002Vector = vectoris::numerics::geometry::Vector3<AFA002Scalar, TestFrameA>;
static_assert(vectoris::numerics::geometry::ScalarArithmetic<AFA002Scalar>);
static_assert(!std::is_nothrow_default_constructible_v<AFA002Vector>);
static_assert(!std::is_nothrow_copy_constructible_v<AFA002Vector>);
static_assert(!std::is_nothrow_move_constructible_v<AFA002Vector>);
static_assert(!noexcept(std::declval<const AFA002Vector&>() + std::declval<const AFA002Vector&>()));
static_assert(!noexcept(-std::declval<const AFA002Vector&>()));
static_assert(!noexcept(std::declval<const AFA002Vector&>() * std::declval<const AFA002Scalar&>()));
static_assert(!noexcept(std::declval<const AFA002Vector&>().dot(std::declval<const AFA002Vector&>())));
}
TEST(AFA002Noexcept, BuiltinScalarContract) {
    using V = vectoris::numerics::geometry::Vector3<double, TestFrameA>;
    using F = vectoris::numerics::geometry::Vector3<float, TestFrameA>;
    constexpr V v{1.,2.,3.}; constexpr F f{1.F,2.F,3.F};
    static_assert(noexcept(V{}) && noexcept(V{1.,2.,3.}) && noexcept(F{}));
    static_assert(noexcept(v+v) && noexcept(v-f) && noexcept(-v));
    static_assert(noexcept(v*2.) && noexcept(2.*v) && noexcept(v.dot(f)));
    static_assert(noexcept(v==v) && noexcept(v!=v));
    static_assert(std::is_nothrow_copy_constructible_v<V> && std::is_nothrow_move_constructible_v<F>);
    EXPECT_DOUBLE_EQ(v.dot(v), 14.);
}
TEST(AFA002Noexcept, ThrowingArithmeticIsCatchable) {
    const AFA002Vector a{AFA002Scalar{1}, AFA002Scalar{2}, AFA002Scalar{3}};
    const AFA002Scalar scalar{2};
    AFA002Scalar::fault=AFA002Fault::Add;
    EXPECT_THROW((void)(a+a), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::Subtract;
    EXPECT_THROW((void)(a-a), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::Negate;
    EXPECT_THROW((void)(-a), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::Multiply;
    EXPECT_THROW((void)(a*scalar), AFA002Exception);
    EXPECT_THROW((void)(scalar*a), AFA002Exception);
    EXPECT_THROW((void)(a.dot(a)), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::Equal;
    EXPECT_THROW((void)(a==a), AFA002Exception);
    EXPECT_THROW((void)(a!=a), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::None;
    EXPECT_EQ((a+a).x.value,2);
    EXPECT_EQ((scalar/scalar).value,1);
}
TEST(AFA002Noexcept, ThrowingConstructionIsCatchable) {
    AFA002Scalar::fault=AFA002Fault::Default;
    EXPECT_THROW((void)AFA002Vector{}, AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::None;
    AFA002Vector a{AFA002Scalar{1},AFA002Scalar{2},AFA002Scalar{3}};
    AFA002Scalar::fault=AFA002Fault::Copy;
    EXPECT_THROW((void)AFA002Vector(a), AFA002Exception);
    EXPECT_THROW((void)(a+a), AFA002Exception); // copying operation results into components
    AFA002Scalar::fault=AFA002Fault::Move;
    EXPECT_THROW((void)AFA002Vector(std::move(a)), AFA002Exception);
    AFA002Scalar::fault=AFA002Fault::None;
}

TEST(AFA002Noexcept, ReferenceScalarConstructionRemainsCompatible) {
    double x=1., y=2., z=3.;
    using V = vectoris::numerics::geometry::Vector3<double&, TestFrameA>;
    static_assert(vectoris::numerics::geometry::ScalarArithmetic<double&>);
    static_assert(noexcept(V{x,y,z}));
    V references{x,y,z};
    EXPECT_EQ(&references.x, &x);
    EXPECT_EQ(&references.y, &y);
    EXPECT_EQ(&references.z, &z);
}
