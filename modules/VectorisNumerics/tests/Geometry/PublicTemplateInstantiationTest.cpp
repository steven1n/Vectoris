#include <gtest/gtest.h>
#include <cmath>
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

// AFA2-002: scalar faults must propagate through every composite family.
namespace {
namespace AFA2Throwing {
namespace g = vectoris::numerics::geometry;
struct F {};
struct G {};
struct Raised {};
enum class Fault { none, def, convert, copy, move, copy_assign, move_assign, add, sub, neg, mul, div, compare };
struct ThrowingScalar {
  static inline Fault fault = Fault::none;
  double v;
  static void raise(Fault f) { if (fault == f) throw Raised{}; }
  ThrowingScalar() : v(0) { raise(Fault::def); }
  ThrowingScalar(double x) : v(x) { raise(Fault::convert); }
  ThrowingScalar(const ThrowingScalar& x) : v(x.v) { raise(Fault::copy); }
  ThrowingScalar(ThrowingScalar&& x) noexcept(false) : v(x.v) { raise(Fault::move); }
  ~ThrowingScalar() = default;
  ThrowingScalar& operator=(const ThrowingScalar& x) { v=x.v; raise(Fault::copy_assign); return *this; }
  ThrowingScalar& operator=(ThrowingScalar&& x) noexcept(false) { v=x.v; raise(Fault::move_assign); return *this; }
  friend ThrowingScalar operator+(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::add); return a.v+b.v; }
  friend ThrowingScalar operator-(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::sub); return a.v-b.v; }
  friend ThrowingScalar operator-(const ThrowingScalar& a) { raise(Fault::neg); return -a.v; }
  friend ThrowingScalar operator*(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::mul); return a.v*b.v; }
  friend ThrowingScalar operator/(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::div); return a.v/b.v; }
  ThrowingScalar& operator+=(const ThrowingScalar& b) { *this=*this+b; return *this; }
  friend bool operator==(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::compare); return a.v==b.v; }
  friend bool operator<(const ThrowingScalar& a,const ThrowingScalar& b) { raise(Fault::compare); return a.v<b.v; }
};
using S=ThrowingScalar;
using V=g::Vector3<S,F>; using P=g::Point3<S,F>; using M=g::Matrix3<S>;
using Q=g::Quaternion<S,F,F>; using R=g::RotationMatrix3<S,F,F>; using T=g::Transform3<S,F,F>;
static_assert(g::ScalarArithmetic<S>);
static_assert(!std::is_nothrow_copy_constructible_v<S>);
struct FaultScope {
 explicit FaultScope(Fault f) { S::fault=f; }
 ~FaultScope() { S::fault=Fault::none; }
 FaultScope(const FaultScope&) = delete;
 FaultScope& operator=(const FaultScope&) = delete;
 FaultScope(FaultScope&&) = delete;
 FaultScope& operator=(FaultScope&&) = delete;
};
} // namespace AFA2Throwing
} // namespace

TEST(AFA2Noexcept, vector_default) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(V{}));
    static_cast<void>(V{}); // successful control also exercises result construction
    const FaultScope fault{Fault::def};
    EXPECT_THROW(static_cast<void>(V{}), Raised);
}

TEST(AFA2Noexcept, vector_component_convert) {
    using namespace AFA2Throwing;
    static_assert(!noexcept((V{1.,2.,3.})));
    static_cast<void>((V{1.,2.,3.})); // successful control also exercises result construction
    const FaultScope fault{Fault::convert};
    EXPECT_THROW(static_cast<void>((V{1.,2.,3.})), Raised);
}

TEST(AFA2Noexcept, vector_component_copy) {
    using namespace AFA2Throwing;
    static_assert(!noexcept((V{S{1},S{2},S{3}})));
    static_cast<void>((V{S{1},S{2},S{3}})); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>((V{S{1},S{2},S{3}})), Raised);
}

TEST(AFA2Noexcept, vector_copy) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(V(v)));
    static_cast<void>(V(v)); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(V(v)), Raised);
}

TEST(AFA2Noexcept, vector_move) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(V(std::move(v))));
    static_cast<void>(V(std::move(v))); // successful control also exercises result construction
    v=V{S{1},S{2},S{3}}; // fresh source for the throwing control
    const FaultScope fault{Fault::move};
    EXPECT_THROW(static_cast<void>(V(std::move(v))), Raised);
}

TEST(AFA2Noexcept, vector_copy_assign) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    V v2=v;
    static_assert(!noexcept(v2=v));
    static_cast<void>(v2=v); // successful control also exercises result construction
    const FaultScope fault{Fault::copy_assign};
    EXPECT_THROW(static_cast<void>(v2=v), Raised);
}

TEST(AFA2Noexcept, vector_move_assign) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    V v2=v;
    static_assert(!noexcept(v2=std::move(v)));
    static_cast<void>(v2=std::move(v)); // successful control also exercises result construction
    v=V{S{1},S{2},S{3}}; // fresh source for the throwing control
    const FaultScope fault{Fault::move_assign};
    EXPECT_THROW(static_cast<void>(v2=std::move(v)), Raised);
}

TEST(AFA2Noexcept, vector_add) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v+v));
    static_cast<void>(v+v); // successful control also exercises result construction
    const FaultScope fault{Fault::add};
    EXPECT_THROW(static_cast<void>(v+v), Raised);
}

TEST(AFA2Noexcept, vector_sub) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v-v));
    static_cast<void>(v-v); // successful control also exercises result construction
    const FaultScope fault{Fault::sub};
    EXPECT_THROW(static_cast<void>(v-v), Raised);
}

TEST(AFA2Noexcept, vector_neg) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(-v));
    static_cast<void>(-v); // successful control also exercises result construction
    const FaultScope fault{Fault::neg};
    EXPECT_THROW(static_cast<void>(-v), Raised);
}

TEST(AFA2Noexcept, vector_mul_right) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const S scalar{2};
    static_assert(!noexcept(v*scalar));
    static_cast<void>(v*scalar); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(v*scalar), Raised);
}

TEST(AFA2Noexcept, vector_mul_left) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const S scalar{2};
    static_assert(!noexcept(scalar*v));
    static_cast<void>(scalar*v); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(scalar*v), Raised);
}

TEST(AFA2Noexcept, vector_dot_mul) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v.dot(v)));
    static_cast<void>(v.dot(v)); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(v.dot(v)), Raised);
}

TEST(AFA2Noexcept, vector_dot_add) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v.dot(v)));
    static_cast<void>(v.dot(v)); // successful control also exercises result construction
    const FaultScope fault{Fault::add};
    EXPECT_THROW(static_cast<void>(v.dot(v)), Raised);
}

TEST(AFA2Noexcept, vector_equal) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v==v));
    static_cast<void>(v==v); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(v==v), Raised);
}

TEST(AFA2Noexcept, vector_unequal) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    static_assert(!noexcept(v!=v));
    static_cast<void>(v!=v); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(v!=v), Raised);
}

TEST(AFA2Noexcept, point_default) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(P{}));
    static_cast<void>(P{}); // successful control also exercises result construction
    const FaultScope fault{Fault::def};
    EXPECT_THROW(static_cast<void>(P{}), Raised);
}

TEST(AFA2Noexcept, point_component_copy) {
    using namespace AFA2Throwing;
    static_assert(!noexcept((P{S{1},S{2},S{3}})));
    static_cast<void>((P{S{1},S{2},S{3}})); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>((P{S{1},S{2},S{3}})), Raised);
}

TEST(AFA2Noexcept, point_add) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const P p{S{1},S{2},S{3}};
    static_assert(!noexcept(p+v));
    static_cast<void>(p+v); // successful control also exercises result construction
    const FaultScope fault{Fault::add};
    EXPECT_THROW(static_cast<void>(p+v), Raised);
}

TEST(AFA2Noexcept, point_sub) {
    using namespace AFA2Throwing;
    const P p{S{1},S{2},S{3}};
    static_assert(!noexcept(p-p));
    static_cast<void>(p-p); // successful control also exercises result construction
    const FaultScope fault{Fault::sub};
    EXPECT_THROW(static_cast<void>(p-p), Raised);
}

TEST(AFA2Noexcept, point_equal) {
    using namespace AFA2Throwing;
    const P p{S{1},S{2},S{3}};
    static_assert(!noexcept(p==p));
    static_cast<void>(p==p); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(p==p), Raised);
}

TEST(AFA2Noexcept, matrix_default) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(M{}));
    static_cast<void>(M{}); // successful control also exercises result construction
    const FaultScope fault{Fault::def};
    EXPECT_THROW(static_cast<void>(M{}), Raised);
}

TEST(AFA2Noexcept, matrix_identity) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(M::Identity()));
    static_cast<void>(M::Identity()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(M::Identity()), Raised);
}

TEST(AFA2Noexcept, matrix_add) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix+matrix));
    static_cast<void>(matrix+matrix); // successful control also exercises result construction
    const FaultScope fault{Fault::add};
    EXPECT_THROW(static_cast<void>(matrix+matrix), Raised);
}

TEST(AFA2Noexcept, matrix_sub) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix-matrix));
    static_cast<void>(matrix-matrix); // successful control also exercises result construction
    const FaultScope fault{Fault::sub};
    EXPECT_THROW(static_cast<void>(matrix-matrix), Raised);
}

TEST(AFA2Noexcept, matrix_mul_right) {
    using namespace AFA2Throwing;
    const S scalar{2};
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix*scalar));
    static_cast<void>(matrix*scalar); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(matrix*scalar), Raised);
}

TEST(AFA2Noexcept, matrix_mul_left) {
    using namespace AFA2Throwing;
    const S scalar{2};
    const M matrix=M::Identity();
    static_assert(!noexcept(scalar*matrix));
    static_cast<void>(scalar*matrix); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(scalar*matrix), Raised);
}

TEST(AFA2Noexcept, matrix_matrix) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix*matrix));
    static_cast<void>(matrix*matrix); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(matrix*matrix), Raised);
}

TEST(AFA2Noexcept, matrix_vector) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix*v));
    static_cast<void>(matrix*v); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(matrix*v), Raised);
}

TEST(AFA2Noexcept, matrix_transposed) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix.transposed()));
    static_cast<void>(matrix.transposed()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(matrix.transposed()), Raised);
}

TEST(AFA2Noexcept, matrix_det) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix.det()));
    static_cast<void>(matrix.det()); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(matrix.det()), Raised);
}

TEST(AFA2Noexcept, matrix_frobenius) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix.frobenius_norm_squared()));
    static_cast<void>(matrix.frobenius_norm_squared()); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(matrix.frobenius_norm_squared()), Raised);
}

TEST(AFA2Noexcept, matrix_equal) {
    using namespace AFA2Throwing;
    const M matrix=M::Identity();
    static_assert(!noexcept(matrix==matrix));
    static_cast<void>(matrix==matrix); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(matrix==matrix), Raised);
}

TEST(AFA2Noexcept, quaternion_identity) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(Q::Identity()));
    static_cast<void>(Q::Identity()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(Q::Identity()), Raised);
}

TEST(AFA2Noexcept, quaternion_canonical) {
    using namespace AFA2Throwing;
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(quaternion.Canonicalized()));
    static_cast<void>(quaternion.Canonicalized()); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(quaternion.Canonicalized()), Raised);
}

TEST(AFA2Noexcept, quaternion_conjugate) {
    using namespace AFA2Throwing;
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(quaternion.Conjugate()));
    static_cast<void>(quaternion.Conjugate()); // successful control also exercises result construction
    const FaultScope fault{Fault::neg};
    EXPECT_THROW(static_cast<void>(quaternion.Conjugate()), Raised);
}

TEST(AFA2Noexcept, quaternion_compose) {
    using namespace AFA2Throwing;
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(quaternion*quaternion));
    static_cast<void>(quaternion*quaternion); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(quaternion*quaternion), Raised);
}

TEST(AFA2Noexcept, quaternion_vector) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(quaternion*v));
    static_cast<void>(quaternion*v); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(quaternion*v), Raised);
}

TEST(AFA2Noexcept, quaternion_equal) {
    using namespace AFA2Throwing;
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(quaternion==quaternion));
    static_cast<void>(quaternion==quaternion); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(quaternion==quaternion), Raised);
}

TEST(AFA2Noexcept, rotation_identity) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(R::Identity()));
    static_cast<void>(R::Identity()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(R::Identity()), Raised);
}

TEST(AFA2Noexcept, rotation_vector) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const R rotation=R::Identity();
    static_assert(!noexcept(rotation*v));
    static_cast<void>(rotation*v); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(rotation*v), Raised);
}

TEST(AFA2Noexcept, rotation_compose) {
    using namespace AFA2Throwing;
    const R rotation=R::Identity();
    static_assert(!noexcept(rotation*rotation));
    static_cast<void>(rotation*rotation); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(rotation*rotation), Raised);
}

TEST(AFA2Noexcept, rotation_inverse) {
    using namespace AFA2Throwing;
    const R rotation=R::Identity();
    static_assert(!noexcept(rotation.Inverse()));
    static_cast<void>(rotation.Inverse()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(rotation.Inverse()), Raised);
}

TEST(AFA2Noexcept, rotation_equal) {
    using namespace AFA2Throwing;
    const R rotation=R::Identity();
    static_assert(!noexcept(rotation==rotation));
    static_cast<void>(rotation==rotation); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(rotation==rotation), Raised);
}

TEST(AFA2Noexcept, transform_create) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    static_assert(!noexcept(T::Create(quaternion,v)));
    static_cast<void>(T::Create(quaternion,v)); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(T::Create(quaternion,v)), Raised);
}

TEST(AFA2Noexcept, transform_identity) {
    using namespace AFA2Throwing;
    static_assert(!noexcept(T::Identity()));
    static_cast<void>(T::Identity()); // successful control also exercises result construction
    const FaultScope fault{Fault::copy};
    EXPECT_THROW(static_cast<void>(T::Identity()), Raised);
}

TEST(AFA2Noexcept, transform_vector) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    const T transform=T::Create(quaternion,v);
    static_assert(!noexcept(transform*v));
    static_cast<void>(transform*v); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(transform*v), Raised);
}

TEST(AFA2Noexcept, transform_point) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const P p{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    const T transform=T::Create(quaternion,v);
    static_assert(!noexcept(transform*p));
    static_cast<void>(transform*p); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(transform*p), Raised);
}

TEST(AFA2Noexcept, transform_compose) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    const T transform=T::Create(quaternion,v);
    static_assert(!noexcept(transform*transform));
    static_cast<void>(transform*transform); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(transform*transform), Raised);
}

TEST(AFA2Noexcept, transform_inverse) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    const T transform=T::Create(quaternion,v);
    static_assert(!noexcept(transform.Inverse()));
    static_cast<void>(transform.Inverse()); // successful control also exercises result construction
    const FaultScope fault{Fault::neg};
    EXPECT_THROW(static_cast<void>(transform.Inverse()), Raised);
}

TEST(AFA2Noexcept, transform_equal) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const Q quaternion=Q::Identity();
    const T transform=T::Create(quaternion,v);
    static_assert(!noexcept(transform==transform));
    static_cast<void>(transform==transform); // successful control also exercises result construction
    const FaultScope fault{Fault::compare};
    EXPECT_THROW(static_cast<void>(transform==transform), Raised);
}

TEST(AFA2Noexcept, unit_mul_right) {
    using namespace AFA2Throwing;
    const S scalar{2};
    const auto unit=g::UnitVector3<double,F>::TryCreate(g::Vector3<double,F>{1,0,0}).value();
    static_assert(!noexcept(unit*scalar));
    static_cast<void>(unit*scalar); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(unit*scalar), Raised);
}

TEST(AFA2Noexcept, unit_mul_left) {
    using namespace AFA2Throwing;
    const S scalar{2};
    const auto unit=g::UnitVector3<double,F>::TryCreate(g::Vector3<double,F>{1,0,0}).value();
    static_assert(!noexcept(scalar*unit));
    static_cast<void>(scalar*unit); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(scalar*unit), Raised);
}

TEST(AFA2Noexcept, unit_dot_vector) {
    using namespace AFA2Throwing;
    V v{S{1},S{2},S{3}};
    const auto unit=g::UnitVector3<double,F>::TryCreate(g::Vector3<double,F>{1,0,0}).value();
    static_assert(!noexcept(unit.dot(v)));
    static_cast<void>(unit.dot(v)); // successful control also exercises result construction
    const FaultScope fault{Fault::mul};
    EXPECT_THROW(static_cast<void>(unit.dot(v)), Raised);
}

namespace {
struct CopyOnlyScalar {
 static inline bool throw_copy=false;
 int v;
 CopyOnlyScalar(int x=0) noexcept : v(x) {}
 CopyOnlyScalar(const CopyOnlyScalar& x) : v(x.v) { if(throw_copy) throw 7; }
 CopyOnlyScalar(CopyOnlyScalar&& x) noexcept(false) : v(x.v) { if(throw_copy) throw 7; }
 ~CopyOnlyScalar() = default;
 CopyOnlyScalar& operator=(const CopyOnlyScalar&) = default;
 CopyOnlyScalar& operator=(CopyOnlyScalar&&) = default;
 friend CopyOnlyScalar operator+(const CopyOnlyScalar& a,const CopyOnlyScalar& b) noexcept { return CopyOnlyScalar{a.v+b.v}; }
 friend CopyOnlyScalar operator-(const CopyOnlyScalar& a,const CopyOnlyScalar& b) noexcept { return CopyOnlyScalar{a.v-b.v}; }
 friend CopyOnlyScalar operator-(const CopyOnlyScalar& a) noexcept { return CopyOnlyScalar{-a.v}; }
 friend CopyOnlyScalar operator*(const CopyOnlyScalar& a,const CopyOnlyScalar& b) noexcept { return CopyOnlyScalar{a.v*b.v}; }
 friend CopyOnlyScalar operator/(const CopyOnlyScalar& a,const CopyOnlyScalar& b) noexcept { return CopyOnlyScalar{a.v/b.v}; }
};
}
TEST(AFA2Noexcept, MatrixLeftProductResultCopy) {
    namespace g=vectoris::numerics::geometry;
    using S=CopyOnlyScalar;
    const auto matrix=g::Matrix3<S>::Identity();
    const S scalar{2};
    static_assert(noexcept(scalar * matrix.m[0]));
    static_assert(!noexcept(scalar * matrix));
    S::throw_copy=true;
    EXPECT_THROW(static_cast<void>(scalar * matrix), int);
    S::throw_copy=false;
}
TEST(AFA2Noexcept, CompositeBuiltinContract) {
    namespace g=vectoris::numerics::geometry;
    using P=g::Point3<double,TestFrameA>; using M=g::Matrix3<double>;
    using Q=g::Quaternion<double,TestFrameA,TestFrameA>;
    using R=g::RotationMatrix3<double,TestFrameA,TestFrameA>;
    using T=g::Transform3<double,TestFrameA,TestFrameA>;
    constexpr auto v=g::Vector3<double,TestFrameA>{1.,2.,3.};
    constexpr auto m=M::Identity(); constexpr auto q=Q::Identity();
    constexpr auto r=R::Identity(); constexpr auto t=T::Identity();
    static_assert(noexcept(P{}) && noexcept(P{1.,2.,3.}) && noexcept(M{}));
    static_assert(noexcept(m+m) && noexcept(m*m) && noexcept(m*v) && noexcept(2.*m));
    static_assert(noexcept(q*q) && noexcept(q*v) && noexcept(q.Conjugate()));
    static_assert(noexcept(r*r) && noexcept(r*v) && noexcept(r.Inverse()));
    static_assert(noexcept(t*t) && noexcept(t*v) && noexcept(t*P{}) && noexcept(t.Inverse()));
    EXPECT_DOUBLE_EQ((t*v).y,2.);
}

TEST(AFA2Noexcept, ScalarFixtureArithmeticControls) {
    using namespace AFA2Throwing;
    const S a{6}, b{2};
    EXPECT_DOUBLE_EQ((a/b).v,3.);
    const CopyOnlyScalar x{6}, y{2};
    EXPECT_EQ((x+y).v,8); EXPECT_EQ((x-y).v,4);
    EXPECT_EQ((-x).v,-6); EXPECT_EQ((x/y).v,3);
}
namespace {
struct ThrowingQuaternionAdapter {
    using R=vectoris::numerics::geometry::RotationMatrix3<double,TestFrameA,TestFrameA>;
    vectoris::numerics::core::Result<R> ToRotationMatrix() const {
        throw AFA2Throwing::Raised{};
    }
};
}
TEST(AFA2Noexcept, QuaternionAdapterExceptionPropagates) {
    using R=ThrowingQuaternionAdapter::R;
    const ThrowingQuaternionAdapter adapter;
    static_assert(!noexcept(R::FromQuaternion(adapter)));
    EXPECT_THROW(static_cast<void>(R::FromQuaternion(adapter)),AFA2Throwing::Raised);
}

// C10: arithmetic capability does not supply the floating-point tolerance policy.
namespace {
template<class M> concept HasDefaultMatrixComparison = requires(const M& a) { a.AlmostEqual(a); };
template<class M> concept HasPartialMatrixComparison = requires(const M& a, typename vectoris::numerics::geometry::GeometryTraits<M>::ScalarType t) { a.AlmostEqual(a,t); };
template<class M> concept HasExplicitMatrixComparison = requires(const M& a, typename vectoris::numerics::geometry::GeometryTraits<M>::ScalarType t) { a.AlmostEqual(a,t,t); };
template<class M> concept HasMatrixInverse = requires(const M& a, M& out) { a.TryInverse(out); };
template<class G, class S> concept HasDefaultGeometryComparison = requires(const G& a) { vectoris::numerics::geometry::AlmostEqual(a,a); };
template<class G, class S> concept HasExplicitGeometryComparison = requires(const G& a, S t) { vectoris::numerics::geometry::AlmostEqual(a,a,t,t); };
template<class Q, class S> concept HasRotationComparison = requires(const Q& q, S t) { vectoris::numerics::geometry::RotationEquivalent(q,q,t,t); };

template<class T>
void CheckMatrixTolerancePolicy() {
    using M = vectoris::numerics::geometry::Matrix3<T>;
    static_assert(HasDefaultMatrixComparison<M> && HasPartialMatrixComparison<M> && HasExplicitMatrixComparison<M>);
    static_assert(noexcept(M::Identity().AlmostEqual(M::Identity())));
    const T eps = std::numeric_limits<T>::epsilon();
    const M identity = M::Identity();
    // The original, unambiguous member-function address remains source compatible.
    const auto compare = &M::AlmostEqual;
    EXPECT_TRUE((identity.*compare)(identity,T{0},T{0}));
    M near = identity;
    near.m[0] += eps*T{50};
    M far = identity;
    far.m[0] += eps*T{200};
    EXPECT_TRUE(identity.AlmostEqual(near));
    EXPECT_FALSE(identity.AlmostEqual(far));
    EXPECT_TRUE(identity.AlmostEqual(near,T{0}));
    EXPECT_FALSE(identity.AlmostEqual(near,T{0},T{0}));
    EXPECT_TRUE(identity.AlmostEqual(near,eps*T{100},T{0}));
    EXPECT_TRUE(vectoris::numerics::geometry::AlmostEqual(identity,near));
    EXPECT_FALSE(vectoris::numerics::geometry::AlmostEqual(identity,far));
    EXPECT_TRUE(vectoris::numerics::geometry::AlmostEqual(identity,near,T{0}));
    EXPECT_FALSE(vectoris::numerics::geometry::AlmostEqual(identity,near,T{0},T{0}));
}

template<class T>
void CheckInverseThresholdPolicy() {
    using M = vectoris::numerics::geometry::Matrix3<T>;
    static_assert(HasMatrixInverse<M>);
    const T eps = std::numeric_limits<T>::epsilon();
    M matrix = M::Identity();
    M out = M::Zero();
    matrix.m[8] = eps;
    EXPECT_FALSE(matrix.TryInverse(out));
    EXPECT_EQ(out.m[8],T{0}); // failed operation retains the prior output
    matrix.m[8] = std::nextafter(eps,std::numeric_limits<T>::infinity());
    ASSERT_TRUE(matrix.TryInverse(out));
    EXPECT_NEAR(out.m[8]*matrix.m[8],T{1},eps*T{2});
}
} // namespace

TEST(Matrix3ScalarContract, CustomConstructionAndArithmetic) {
    using namespace AFA2Throwing;
    const M identity = M::Identity();
    const M sum = identity+identity;
    const M product = (sum-identity)*identity;
    EXPECT_DOUBLE_EQ(product.m[0].v,1.0);
    EXPECT_DOUBLE_EQ(product.m[4].v,1.0);
    EXPECT_DOUBLE_EQ(product.m[8].v,1.0);
    EXPECT_DOUBLE_EQ(product.m[1].v,0.0);
}

TEST(Matrix3ScalarContract, UnsupportedToleranceIsUnavailableAtPublicBoundary) {
    using namespace AFA2Throwing;
    static_assert(!vectoris::numerics::Concepts::Numeric<S>);
    static_assert(!HasDefaultMatrixComparison<M>);
    static_assert(!HasPartialMatrixComparison<M>);
    static_assert(!HasExplicitMatrixComparison<M>);
    static_assert(!HasMatrixInverse<M>);
    static_assert(!HasDefaultGeometryComparison<M,S> && !HasExplicitGeometryComparison<M,S>);
    static_assert(!HasDefaultGeometryComparison<V,S> && !HasExplicitGeometryComparison<V,S>);
    static_assert(!HasDefaultGeometryComparison<P,S> && !HasExplicitGeometryComparison<P,S>);
    static_assert(!HasDefaultGeometryComparison<Q,S> && !HasExplicitGeometryComparison<Q,S>);
    static_assert(!HasDefaultGeometryComparison<R,S> && !HasExplicitGeometryComparison<R,S>);
    static_assert(!HasDefaultGeometryComparison<T,S> && !HasExplicitGeometryComparison<T,S>);
    static_assert(!HasRotationComparison<Q,S>);
    static_assert(!HasDefaultMatrixComparison<g::Matrix3<int>>);
    EXPECT_DOUBLE_EQ(M::Identity().m[0].v,1.0);
}

TEST(Matrix3ScalarContract, FloatDefaultAndExplicitTolerance) { CheckMatrixTolerancePolicy<float>(); }
TEST(Matrix3ScalarContract, DoubleDefaultAndExplicitTolerance) { CheckMatrixTolerancePolicy<double>(); }
TEST(Matrix3ScalarContract, FloatInverseThresholdUnchanged) { CheckInverseThresholdPolicy<float>(); }
TEST(Matrix3ScalarContract, DoubleInverseThresholdUnchanged) { CheckInverseThresholdPolicy<double>(); }
