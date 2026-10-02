#include <gtest/gtest.h>
#include <concepts>
#include <limits>
#include <string>
#include <type_traits>

// Core
#include "Vectoris/Numerics/Core/BasicTypes.h"
#include "Vectoris/Numerics/Core/Compiler.h"
#include "Vectoris/Numerics/Core/Concepts.h"
#include "Vectoris/Numerics/Core/Constants.h"
#include "Vectoris/Numerics/Core/Math.h"
#include "Vectoris/Numerics/Core/MathError.h"
#include "Vectoris/Numerics/Core/MathFunctions.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Numerics/Core/Precision.h"
#include "Vectoris/Numerics/Core/Result.h"

// Units Base
#include "Vectoris/Numerics/Units/BaseUnits/Amount.h"
#include "Vectoris/Numerics/Units/BaseUnits/Angle.h"
#include "Vectoris/Numerics/Units/BaseUnits/Current.h"
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"
#include "Vectoris/Numerics/Units/BaseUnits/Luminosity.h"
#include "Vectoris/Numerics/Units/BaseUnits/Mass.h"
#include "Vectoris/Numerics/Units/BaseUnits/Temperature.h"
#include "Vectoris/Numerics/Units/BaseUnits/Time.h"

// Units Derived
#include "Vectoris/Numerics/Units/DerivedUnits/Acceleration.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularAcceleration.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularMomentum.h"
#include "Vectoris/Numerics/Units/DerivedUnits/AngularVelocity.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Force.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Frequency.h"
#include "Vectoris/Numerics/Units/DerivedUnits/MomentOfInertia.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Power.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Torque.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Velocity.h"

// Units General & Detail
#include "Vectoris/Numerics/Units/Common.h"
#include "Vectoris/Numerics/Units/Conversion.h"
#include "Vectoris/Numerics/Units/Core.h"
#include "Vectoris/Numerics/Units/Dimension.h"
#include "Vectoris/Numerics/Units/Literals.h"
#include "Vectoris/Numerics/Units/Quantity.h"
#include "Vectoris/Numerics/Units/QuantityABI.h"
#include "Vectoris/Numerics/Units/UnitCast.h"
#include "Vectoris/Numerics/Units/UnitConcepts.h"
#include "Vectoris/Numerics/Units/UnitTraits.h"
#include "Vectoris/Numerics/Units/Detail/ABI.h"
#include "Vectoris/Numerics/Units/Detail/Ratio.h"

// Geometry
#include "Vectoris/Numerics/Geometry/AlmostEqual.h"
#include "Vectoris/Numerics/Geometry/Concepts.h"
#include "Vectoris/Numerics/Geometry/CoordinateConvention.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Numerics/Geometry/RotationMatrix3.h"
#include "Vectoris/Numerics/Geometry/SymmetricLinearSolver3.h"
#include "Vectoris/Numerics/Geometry/Traits.h"
#include "Vectoris/Numerics/Geometry/Transform3.h"
#include "Vectoris/Numerics/Geometry/UnitVector3.h"
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/Detail/ABI.h"
#include "Vectoris/Numerics/Geometry/Detail/RotationInvariant.h"

using namespace vectoris::numerics;

// Declared representative frames
struct WorldFrame final {};
struct BodyFrame final {};
struct SensorFrame final {};

template <typename T>
concept HasIdentityFactory = requires {
    { T::Identity() } -> std::same_as<T>;
};

template <typename T, typename F1, typename F2>
concept HasExplicitIdentityFactory = requires {
    { T::template Identity<F1, F2>() } -> std::same_as<T>;
};

using LowerQuaternionSameFloat =
    vectoris::numerics::geometry::Quaternion<float, WorldFrame, WorldFrame>;
using LowerQuaternionCrossFloat =
    vectoris::numerics::geometry::Quaternion<float, WorldFrame, BodyFrame>;
using UpperQuaternionSameDouble =
    vectoris::numerics::Geometry::Quaternion<double, BodyFrame, BodyFrame>;
using UpperQuaternionCrossDouble =
    vectoris::numerics::Geometry::Quaternion<double, BodyFrame, SensorFrame>;

using LowerTransformSameFloat =
    vectoris::numerics::geometry::Transform3<float, WorldFrame, WorldFrame>;
using LowerTransformCrossFloat =
    vectoris::numerics::geometry::Transform3<float, WorldFrame, BodyFrame>;
using UpperTransformSameDouble =
    vectoris::numerics::Geometry::Transform3<double, BodyFrame, BodyFrame>;
using UpperTransformCrossDouble =
    vectoris::numerics::Geometry::Transform3<double, BodyFrame, SensorFrame>;

static_assert(HasIdentityFactory<LowerQuaternionSameFloat>);
static_assert(!HasIdentityFactory<LowerQuaternionCrossFloat>);
static_assert(HasExplicitIdentityFactory<LowerQuaternionSameFloat, WorldFrame, WorldFrame>);
static_assert(HasExplicitIdentityFactory<LowerQuaternionSameFloat, BodyFrame, BodyFrame>);
static_assert(!HasExplicitIdentityFactory<LowerQuaternionCrossFloat, WorldFrame, WorldFrame>);
static_assert(HasIdentityFactory<UpperQuaternionSameDouble>);
static_assert(!HasIdentityFactory<UpperQuaternionCrossDouble>);
static_assert(HasExplicitIdentityFactory<UpperQuaternionSameDouble, BodyFrame, BodyFrame>);
static_assert(HasExplicitIdentityFactory<UpperQuaternionSameDouble, WorldFrame, WorldFrame>);
static_assert(!HasExplicitIdentityFactory<UpperQuaternionCrossDouble, BodyFrame, BodyFrame>);

static_assert(HasIdentityFactory<LowerTransformSameFloat>);
static_assert(!HasIdentityFactory<LowerTransformCrossFloat>);
static_assert(HasExplicitIdentityFactory<LowerTransformSameFloat, WorldFrame, WorldFrame>);
static_assert(HasExplicitIdentityFactory<LowerTransformSameFloat, BodyFrame, BodyFrame>);
static_assert(!HasExplicitIdentityFactory<LowerTransformCrossFloat, WorldFrame, WorldFrame>);
static_assert(HasIdentityFactory<UpperTransformSameDouble>);
static_assert(!HasIdentityFactory<UpperTransformCrossDouble>);
static_assert(HasExplicitIdentityFactory<UpperTransformSameDouble, BodyFrame, BodyFrame>);
static_assert(HasExplicitIdentityFactory<UpperTransformSameDouble, WorldFrame, WorldFrame>);
static_assert(!HasExplicitIdentityFactory<UpperTransformCrossDouble, BodyFrame, BodyFrame>);

static_assert(Geometry::FrameTag<WorldFrame>);
static_assert(Geometry::FrameTag<BodyFrame>);
static_assert(Geometry::FrameTag<SensorFrame>);

// =============================================================================
// Core API Surface Tests
// =============================================================================

TEST(PublicApiSurfaceTest, CoreResultNamedFactories) {
    // float value
    auto res_f = Core::Result<float, Core::MathError>::success(3.14f);
    EXPECT_TRUE(res_f.IsSuccess());
    EXPECT_FLOAT_EQ(res_f.Value(), 3.14f);

    // double value
    auto res_d = Core::Result<double, Core::MathError>::success(2.718281828459045);
    EXPECT_TRUE(res_d.IsSuccess());
    EXPECT_DOUBLE_EQ(res_d.Value(), 2.718281828459045);

    // failure
    auto err_f = Core::Result<float, Core::MathError>::failure(Core::MathError::invalid_argument);
    EXPECT_FALSE(err_f.IsSuccess());
    EXPECT_EQ(err_f.error(), Core::MathError::invalid_argument);
}

TEST(PublicApiSurfaceTest, CoreAlmostEqualFloatAndDouble) {
    // float AlmostEqual
    float f1 = 1.0f;
    float f2 = 1.0f + 1e-6f;
    EXPECT_TRUE(Traits::AlmostEqual(f1, f2, 1e-5f, 1e-5f));
    EXPECT_FALSE(Traits::AlmostEqual(f1, 2.0f, 1e-5f, 1e-5f));

    // double AlmostEqual
    double d1 = 1.0;
    double d2 = 1.0 + 1e-14;
    EXPECT_TRUE(Traits::AlmostEqual(d1, d2, 1e-12, 1e-12));
    EXPECT_FALSE(Traits::AlmostEqual(d1, 2.0, 1e-12, 1e-12));
}

// =============================================================================
// Units API Surface Tests
// =============================================================================

TEST(PublicApiSurfaceTest, UnitsBaseQuantitiesFloatAndDouble) {
    using namespace Units;

    // Amount (Mole)
    Quantity<float, MoleUnit> mole_f(2.0f);
    Quantity<double, MoleUnit> mole_d(2.0);
    EXPECT_FLOAT_EQ(mole_f.value(), 2.0f);
    EXPECT_DOUBLE_EQ(mole_d.value(), 2.0);

    // Angle (Radian)
    Quantity<float, RadianUnit> rad_f(1.57f);
    Quantity<double, RadianUnit> rad_d(1.5707963267948966);
    EXPECT_FLOAT_EQ(rad_f.value(), 1.57f);
    EXPECT_DOUBLE_EQ(rad_d.value(), 1.5707963267948966);

    // Current (Ampere)
    Quantity<float, AmpereUnit> amp_f(5.0f);
    Quantity<double, AmpereUnit> amp_d(5.0);
    EXPECT_FLOAT_EQ(amp_f.value(), 5.0f);
    EXPECT_DOUBLE_EQ(amp_d.value(), 5.0);

    // Length (Meter)
    Quantity<float, MeterUnit> m_f(10.0f);
    Quantity<double, MeterUnit> m_d(10.0);
    EXPECT_FLOAT_EQ(m_f.value(), 10.0f);
    EXPECT_DOUBLE_EQ(m_d.value(), 10.0);

    // Luminosity (Candela)
    Quantity<float, CandelaUnit> cd_f(100.0f);
    Quantity<double, CandelaUnit> cd_d(100.0);
    EXPECT_FLOAT_EQ(cd_f.value(), 100.0f);
    EXPECT_DOUBLE_EQ(cd_d.value(), 100.0);

    // Mass (Kilogram)
    Quantity<float, KilogramUnit> kg_f(75.0f);
    Quantity<double, KilogramUnit> kg_d(75.0);
    EXPECT_FLOAT_EQ(kg_f.value(), 75.0f);
    EXPECT_DOUBLE_EQ(kg_d.value(), 75.0);

    // Temperature (Kelvin)
    Quantity<float, KelvinUnit> k_f(300.0f);
    Quantity<double, KelvinUnit> k_d(300.0);
    EXPECT_FLOAT_EQ(k_f.value(), 300.0f);
    EXPECT_DOUBLE_EQ(k_d.value(), 300.0);

    // Time (Second)
    Quantity<float, SecondUnit> s_f(60.0f);
    Quantity<double, SecondUnit> s_d(60.0);
    EXPECT_FLOAT_EQ(s_f.value(), 60.0f);
    EXPECT_DOUBLE_EQ(s_d.value(), 60.0);
}

TEST(PublicApiSurfaceTest, UnitsDerivedAndUnitCast) {
    using namespace Units;

    // Derived units
    Force F(100.0);
    Velocity v(25.0);
    Acceleration a(9.81);
    Power P(2500.0);
    Torque tau(50.0);
    Frequency freq(60.0);
    AngularAcceleration alpha(1.5);
    AngularMomentum L(10.0);
    AngularVelocity omega(2.0);
    MomentOfInertia I(5.0);

    EXPECT_DOUBLE_EQ(F.value(), 100.0);
    EXPECT_DOUBLE_EQ(v.value(), 25.0);
    EXPECT_DOUBLE_EQ(a.value(), 9.81);
    EXPECT_DOUBLE_EQ(P.value(), 2500.0);
    EXPECT_DOUBLE_EQ(tau.value(), 50.0);
    EXPECT_DOUBLE_EQ(freq.value(), 60.0);
    EXPECT_DOUBLE_EQ(alpha.value(), 1.5);
    EXPECT_DOUBLE_EQ(L.value(), 10.0);
    EXPECT_DOUBLE_EQ(omega.value(), 2.0);
    EXPECT_DOUBLE_EQ(I.value(), 5.0);

    // Unit cast: Gram <-> Kilogram
    Kilogram kg(2.0);
    Gram g = unit_cast<GramUnit>(kg);
    EXPECT_DOUBLE_EQ(g.value(), 2000.0);
    Kilogram kg2 = unit_cast<KilogramUnit>(g);
    EXPECT_DOUBLE_EQ(kg2.value(), 2.0);

    // Float unit cast
    Quantity<float, KilogramUnit> kg_f(0.5f);
    Quantity<float, GramUnit> g_f = unit_cast<GramUnit>(kg_f);
    EXPECT_FLOAT_EQ(g_f.value(), 500.0f);
}

// =============================================================================
// Geometry API Surface Tests
// =============================================================================

TEST(PublicApiSurfaceTest, GeometryVector3FloatAndDouble) {
    using namespace Geometry;

    Vector3<float, WorldFrame> vf(1.0f, 2.0f, 3.0f);
    Vector3<float, WorldFrame> vf2(4.0f, 5.0f, 6.0f);
    auto vf_sum = vf + vf2;
    EXPECT_FLOAT_EQ(vf_sum.x, 5.0f);
    EXPECT_FLOAT_EQ(vf_sum.y, 7.0f);
    EXPECT_FLOAT_EQ(vf_sum.z, 9.0f);
    EXPECT_FLOAT_EQ(vf.dot(vf2), 32.0f);

    Vector3<double, SensorFrame> vd(1.0, 2.0, 3.0);
    Vector3<double, SensorFrame> vd2(4.0, 5.0, 6.0);
    auto vd_diff = vd2 - vd;
    EXPECT_DOUBLE_EQ(vd_diff.x, 3.0);
    EXPECT_DOUBLE_EQ(vd_diff.y, 3.0);
    EXPECT_DOUBLE_EQ(vd_diff.z, 3.0);
    EXPECT_DOUBLE_EQ(vd.dot(vd2), 32.0);
}

TEST(PublicApiSurfaceTest, GeometryIdentityFactoriesRespectClassFrames) {
    const auto quaternion_f = LowerQuaternionSameFloat::Identity();
    EXPECT_FLOAT_EQ(quaternion_f.w, 1.0f);
    EXPECT_FLOAT_EQ(quaternion_f.x, 0.0f);
    EXPECT_FLOAT_EQ(quaternion_f.y, 0.0f);
    EXPECT_FLOAT_EQ(quaternion_f.z, 0.0f);

    const auto quaternion_d = UpperQuaternionSameDouble::Identity<BodyFrame, BodyFrame>();
    EXPECT_DOUBLE_EQ(quaternion_d.w, 1.0);
    EXPECT_DOUBLE_EQ(quaternion_d.x, 0.0);
    EXPECT_DOUBLE_EQ(quaternion_d.y, 0.0);
    EXPECT_DOUBLE_EQ(quaternion_d.z, 0.0);

    const auto transform_f = LowerTransformSameFloat::Identity<WorldFrame, WorldFrame>();
    EXPECT_FLOAT_EQ(transform_f.rotation_.w, 1.0f);
    EXPECT_FLOAT_EQ(transform_f.originOffset_.x, 0.0f);

    const auto transform_d = UpperTransformSameDouble::Identity();
    EXPECT_DOUBLE_EQ(transform_d.rotation_.w, 1.0);
    EXPECT_DOUBLE_EQ(transform_d.originOffset_.x, 0.0);

    // RotationMatrix3::Identity is a coordinate-identity matrix mapping and
    // intentionally remains available for cross-frame types with aligned axes.
    using CrossFrameMatrix =
        vectoris::numerics::geometry::RotationMatrix3<float, WorldFrame, BodyFrame>;
    const auto matrix = CrossFrameMatrix::Identity();
    EXPECT_FLOAT_EQ(matrix.ToMatrix()(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(matrix.ToMatrix()(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(matrix.ToMatrix()(2, 2), 1.0f);
}

TEST(PublicApiSurfaceTest, GeometryUnitVector3AccessorsFloatAndDouble) {
    using namespace Geometry;

    // float UnitVector3
    Vector3<float, WorldFrame> vf(3.0f, 4.0f, 0.0f);
    auto uv_res_f = UnitVector3<float, WorldFrame>::TryCreate(vf);
    ASSERT_TRUE(uv_res_f.IsSuccess());
    auto uv_f = uv_res_f.Value();
    EXPECT_FLOAT_EQ(uv_f.x(), 0.6f);
    EXPECT_FLOAT_EQ(uv_f.y(), 0.8f);
    EXPECT_FLOAT_EQ(uv_f.z(), 0.0f);
    EXPECT_FLOAT_EQ(uv_f.getX(), 0.6f);
    EXPECT_FLOAT_EQ(uv_f.getY(), 0.8f);
    EXPECT_FLOAT_EQ(uv_f.getZ(), 0.0f);
    EXPECT_TRUE(uv_f.IsValid());

    // double UnitVector3
    Vector3<double, BodyFrame> vd(0.0, 0.0, 5.0);
    auto uv_res_d = UnitVector3<double, BodyFrame>::TryCreate(vd);
    ASSERT_TRUE(uv_res_d.IsSuccess());
    auto uv_d = uv_res_d.Value();
    EXPECT_DOUBLE_EQ(uv_d.x(), 0.0);
    EXPECT_DOUBLE_EQ(uv_d.y(), 0.0);
    EXPECT_DOUBLE_EQ(uv_d.z(), 1.0);
    EXPECT_DOUBLE_EQ(uv_d.getX(), 0.0);
    EXPECT_DOUBLE_EQ(uv_d.getY(), 0.0);
    EXPECT_DOUBLE_EQ(uv_d.getZ(), 1.0);
    EXPECT_TRUE(uv_d.IsValid());

    auto v_back = uv_d.ToVector();
    EXPECT_DOUBLE_EQ(v_back.z, 1.0);
}

TEST(PublicApiSurfaceTest, GeometryMatrix3AndVectorMultiplication) {
    using namespace Geometry;

    // float Matrix3 * Vector3
    auto mf = Matrix3<float>::Identity();
    Vector3<float, WorldFrame> vf(1.0f, 2.0f, 3.0f);
    auto res_vf = mf * vf;
    EXPECT_FLOAT_EQ(res_vf.x, 1.0f);
    EXPECT_FLOAT_EQ(res_vf.y, 2.0f);
    EXPECT_FLOAT_EQ(res_vf.z, 3.0f);

    // double Matrix3 * Vector3
    Matrix3<double> md(
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0
    );
    Vector3<double, BodyFrame> vd(1.0, 2.0, 3.0);
    auto res_vd = md * vd;
    EXPECT_DOUBLE_EQ(res_vd.x, 14.0);
    EXPECT_DOUBLE_EQ(res_vd.y, 32.0);
    EXPECT_DOUBLE_EQ(res_vd.z, 50.0);

    const Matrix3<int> integer_matrix{2,0,0,0,1,0,0,0,1};
    const auto integer_determinant = integer_matrix.det();
    ASSERT_TRUE(integer_determinant.IsSuccess());
    EXPECT_EQ(integer_determinant.Value(), 2);
    // Scalar-left and scalar-right
    auto m_scaled = 2.0 * md;
    auto m_scaled2 = md * 2.0;
    EXPECT_DOUBLE_EQ(m_scaled(0, 0), 2.0);
    EXPECT_DOUBLE_EQ(m_scaled2(0, 0), 2.0);
}

TEST(PublicApiSurfaceTest, GeometryMatrix3RepresentationContract) {
    using Canonical = vectoris::numerics::geometry::Matrix3<double>;
    using Compatibility = vectoris::numerics::Geometry::Matrix3<double>;
    static_assert(std::is_same_v<Canonical, Compatibility>);
    static_assert(std::is_standard_layout_v<Canonical>);
    static_assert(std::is_trivially_copyable_v<Canonical>);
    static_assert(sizeof(Canonical) == sizeof(double) * 9U);

    Canonical matrix{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    matrix.m[0] = 10.0;
    matrix(2, 1) = 80.0;
    EXPECT_DOUBLE_EQ(matrix(0, 0), 10.0);
    EXPECT_DOUBLE_EQ(matrix.m[7], 80.0);
    const auto& read_only = matrix;
    EXPECT_DOUBLE_EQ(read_only(0, 2), 3.0);
}

TEST(PublicApiSurfaceTest, GeometryQuaternionCheckedConversion) {
    using namespace Geometry;

    // float Quaternion
    auto qf_res = Quaternion<float, WorldFrame, BodyFrame>::TryCreate(1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(qf_res.IsSuccess());
    auto qf = qf_res.Value();
    auto rot_f_res = qf.ToRotationMatrix();
    ASSERT_TRUE(rot_f_res.IsSuccess());
    auto rot_f = rot_f_res.Value();
    EXPECT_FLOAT_EQ(rot_f.ToMatrix()(0, 0), 1.0f);

    // double Quaternion
    auto qd_res = Quaternion<double, WorldFrame, SensorFrame>::TryCreate(1.0, 0.0, 0.0, 0.0);
    ASSERT_TRUE(qd_res.IsSuccess());
    auto qd = qd_res.Value();
    auto rot_d_res = qd.ToRotationMatrix();
    ASSERT_TRUE(rot_d_res.IsSuccess());
    auto rot_d = rot_d_res.Value();
    EXPECT_DOUBLE_EQ(rot_d.ToMatrix()(0, 0), 1.0);
}

TEST(PublicApiSurfaceTest, GeometryTransform3FloatAndDouble) {
    using namespace Geometry;

    // float Transform3
    auto qf_res = Quaternion<float, WorldFrame, BodyFrame>::TryCreate(1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(qf_res.IsSuccess());
    Vector3<float, BodyFrame> offset_f(1.0f, 2.0f, 3.0f);
    auto tf = Transform3<float, WorldFrame, BodyFrame>::Create(qf_res.Value(), offset_f);
    Point3<float, WorldFrame> pf(0.0f, 0.0f, 0.0f);
    auto pf_transformed = tf * pf;
    EXPECT_FLOAT_EQ(pf_transformed.x, 1.0f);
    EXPECT_FLOAT_EQ(pf_transformed.y, 2.0f);
    EXPECT_FLOAT_EQ(pf_transformed.z, 3.0f);
    EXPECT_TRUE(AlmostEqual(tf, tf));

    // double Transform3
    auto qd_res = Quaternion<double, WorldFrame, SensorFrame>::TryCreate(1.0, 0.0, 0.0, 0.0);
    ASSERT_TRUE(qd_res.IsSuccess());
    Vector3<double, SensorFrame> offset_d(10.0, 20.0, 30.0);
    auto td = Transform3<double, WorldFrame, SensorFrame>::Create(qd_res.Value(), offset_d);
    Point3<double, WorldFrame> pd(5.0, 5.0, 5.0);
    auto pd_transformed = td * pd;
    EXPECT_DOUBLE_EQ(pd_transformed.x, 15.0);
    EXPECT_DOUBLE_EQ(pd_transformed.y, 25.0);
    EXPECT_DOUBLE_EQ(pd_transformed.z, 35.0);
    EXPECT_TRUE(AlmostEqual(td, td));
}

TEST(PublicApiSurfaceTest, GeometrySymmetricLinearSolverFloatAndDouble) {
    using namespace Geometry;

    // float LDLT solver
    Matrix3<float> Af(
        4.0f, 1.0f, 0.0f,
        1.0f, 4.0f, 1.0f,
        0.0f, 1.0f, 4.0f
    );
    Vector3<float, WorldFrame> bf{5.0f, 6.0f, 5.0f};
    auto sol_f = SolveSymmetricPositiveDefinite3x3(Af, bf);
    ASSERT_TRUE(sol_f.IsSuccess());
    EXPECT_NEAR(sol_f.Value().x, 1.0f, 1e-4f);
    EXPECT_NEAR(sol_f.Value().y, 1.0f, 1e-4f);
    EXPECT_NEAR(sol_f.Value().z, 1.0f, 1e-4f);

    // double LDLT solver
    Matrix3<double> Ad(
        4.0, 1.0, 0.0,
        1.0, 4.0, 1.0,
        0.0, 1.0, 4.0
    );
    Vector3<double, BodyFrame> bd{5.0, 6.0, 5.0};
    auto sol_d = SolveSymmetricPositiveDefinite3x3(Ad, bd);
    ASSERT_TRUE(sol_d.IsSuccess());
    EXPECT_NEAR(sol_d.Value().x, 1.0, 1e-12);
    EXPECT_NEAR(sol_d.Value().y, 1.0, 1e-12);
    EXPECT_NEAR(sol_d.Value().z, 1.0, 1e-12);
}

// =============================================================================
// Compile-Time Negative Constraint Verification (Migrated Requires Checks)
// =============================================================================

template <typename T, typename U>
concept SupportsAddition = requires(T a, U b) { a + b; };

template <typename T, typename U>
concept SupportsMultiplication = requires(T a, U b) { a * b; };

TEST(PublicApiSurfaceTest, CompileTimeNegativeRejectionConstraints) {
    using namespace Geometry;
    using namespace Units;

    // 1. Cross-Frame Vector3 addition rejection
    static_assert(!SupportsAddition<Vector3<double, WorldFrame>, Vector3<double, BodyFrame>>,
                  "Cross-frame Vector3 addition must be rejected at compile time");

    // 2. Unsupported Matrix scalar multiplication rejection
    static_assert(!SupportsMultiplication<Matrix3<double>, std::string>,
                  "Matrix3 multiplication by non-scalar must be rejected by concept");

    // 3. Incompatible quantity dimensional addition rejection
    static_assert(!SupportsAddition<Meter, Second>,
                  "Incompatible dimensional addition must be rejected at compile time");

    EXPECT_TRUE(true);
}



// VRT-12: fully-qualified canonical calls, with compatibility entity identity.
namespace {
template <typename T>
void VerifyQualifiedNamespaceApi() {
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(
        vectoris::numerics::core::sqrt(T{4}), T{2}, T{0}, T{0}));
    EXPECT_TRUE(vectoris::numerics::Core::AlmostEqual(
        vectoris::numerics::Core::sqrt(T{4}), T{2}, T{0}, T{0}));
    static_assert(&vectoris::numerics::core::AlmostEqual<T> ==
                  &vectoris::numerics::Traits::AlmostEqual<T>);
    using R = vectoris::numerics::core::Result<T, vectoris::numerics::core::MathError>;
    static_assert(std::is_same_v<R, vectoris::numerics::Core::Result<T, vectoris::numerics::Core::MathError>>);
    const auto result = vectoris::numerics::core::Result<T, int>::success(T{3});
    const auto legacy = vectoris::numerics::Core::Result<T, int>::success(T{3});
    ASSERT_TRUE(result); ASSERT_TRUE(legacy);
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(result.value(), legacy.value(), T{0}, T{0}));

    const vectoris::numerics::units::Quantity<T, vectoris::numerics::units::KilogramUnit> mass{T{2}};
    const auto grams = vectoris::numerics::units::unit_cast<vectoris::numerics::units::GramUnit>(mass);
    const auto legacyGrams = vectoris::numerics::Units::unit_cast<vectoris::numerics::Units::GramUnit>(mass);
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(grams.value(), T{2000}, T{0}, T{0}));
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(grams.value(), legacyGrams.value(), T{0}, T{0}));
    const vectoris::numerics::units::Quantity<T, vectoris::numerics::units::MeterPerSecondUnit> speed{T{3}};
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(speed.value(), T{3}, T{0}, T{0}));
    static_assert(std::is_same_v<vectoris::numerics::units::Meter, vectoris::numerics::Units::Meter>);

    using V = vectoris::numerics::geometry::Vector3<T, WorldFrame>;
    static_assert(std::is_same_v<V, vectoris::numerics::Geometry::Vector3<T, WorldFrame>>);
    const vectoris::numerics::geometry::Vector3<T, WorldFrame> v{T{1}, T{2}, T{3}};
    const auto matrix = vectoris::numerics::geometry::Matrix3<T>::Identity();
    EXPECT_TRUE(vectoris::numerics::geometry::AlmostEqual(matrix * v, v, T{0}, T{0}));
    const auto unit = vectoris::numerics::geometry::UnitVector3<T, WorldFrame>::TryCreate(v);
    ASSERT_TRUE(unit); EXPECT_TRUE(unit.value().IsValid());
    const auto quaternion = vectoris::numerics::geometry::Quaternion<T, WorldFrame, WorldFrame>::TryCreate(T{1}, T{0}, T{0}, T{0});
    ASSERT_TRUE(quaternion);
    const auto transform = vectoris::numerics::geometry::Transform3<T, WorldFrame, WorldFrame>::Create(quaternion.value(), v);
    const auto point = transform * vectoris::numerics::geometry::Point3<T, WorldFrame>{};
    EXPECT_TRUE(vectoris::numerics::core::AlmostEqual(point.x, T{1}, T{0}, T{0}));
    const auto solution = vectoris::numerics::geometry::SolveSymmetricPositiveDefinite3x3(matrix, v);
    const auto legacySolution = vectoris::numerics::Geometry::SolveSymmetricPositiveDefinite3x3(matrix, v);
    ASSERT_TRUE(solution); ASSERT_TRUE(legacySolution);
    EXPECT_TRUE(vectoris::numerics::geometry::AlmostEqual(solution.value(), v, T{0}, T{0}));
    EXPECT_TRUE(vectoris::numerics::Geometry::AlmostEqual(solution.value(), legacySolution.value(), T{0}, T{0}));
}
} // namespace

TEST(PublicApiSurfaceTest, NamespaceQualifiedFloat) {
    VerifyQualifiedNamespaceApi<float>();
}

TEST(PublicApiSurfaceTest, NamespaceQualifiedDouble) {
    VerifyQualifiedNamespaceApi<double>();
}

namespace {
struct ApiThrowingMovePayload final {
    int id;
    explicit ApiThrowingMovePayload(int value) noexcept : id(value) {}
    ApiThrowingMovePayload(const ApiThrowingMovePayload& other) noexcept(false) : id(other.id) {}
    ApiThrowingMovePayload(ApiThrowingMovePayload&& other) noexcept(false) : id(other.id) {}
    ApiThrowingMovePayload& operator=(const ApiThrowingMovePayload&) = default;
    ApiThrowingMovePayload& operator=(ApiThrowingMovePayload&&) = default;
    ~ApiThrowingMovePayload() = default;
};
} // namespace

TEST(PublicApiSurfaceTest, CoreResultTwoStatePayloadPolicy) {
    using SafeResult = vectoris::numerics::core::Result<int, long>;
    using UnsafeValue = vectoris::numerics::core::Result<ApiThrowingMovePayload, int>;
    using UnsafeError = vectoris::numerics::core::Result<int, ApiThrowingMovePayload>;
    static_assert(std::is_copy_assignable_v<SafeResult> && std::is_nothrow_move_assignable_v<SafeResult>);
    static_assert(std::is_copy_constructible_v<UnsafeValue> && std::is_move_constructible_v<UnsafeValue>);
    static_assert(std::is_copy_constructible_v<UnsafeError> && std::is_move_constructible_v<UnsafeError>);
    static_assert(!std::is_copy_assignable_v<UnsafeValue> && !std::is_move_assignable_v<UnsafeValue>);
    static_assert(!std::is_copy_assignable_v<UnsafeError> && !std::is_move_assignable_v<UnsafeError>);
    auto result = SafeResult::failure(17);
    const auto source = SafeResult::success(42);
    result = source;
    ASSERT_TRUE(result.IsSuccess());
    ASSERT_NE(result.value_if(), nullptr);
    EXPECT_EQ(*result.value_if(), 42);
    EXPECT_EQ(result.error_if(), nullptr);
    auto value = UnsafeValue::success(42);
    auto error = UnsafeError::failure(17);
    EXPECT_EQ(value.Value().id, 42);
    EXPECT_EQ(error.error().id, 17);
}

namespace {
template<class T> void VerifyCanonicalSqrtBinaryApi() {
    constexpr T compile_time = vectoris::numerics::core::sqrt(T{2});
    constexpr T compatibility = vectoris::numerics::core::Math::sqrt(T{2});
    constexpr T negative = vectoris::numerics::core::sqrt(T{-1});
    static_assert(vectoris::numerics::Traits::IsNaN(negative));
    volatile T input = T{2};
    const T reference = std::sqrt(input);
    const T ulp = std::nextafter(reference, std::numeric_limits<T>::infinity()) - reference;
    EXPECT_LE(std::abs(compile_time - reference), ulp);
    EXPECT_LE(std::abs(compatibility - reference), ulp);
    EXPECT_LE(std::abs(vectoris::numerics::core::sqrt(input) - reference), ulp);
    EXPECT_LE(std::abs(vectoris::numerics::core::Math::sqrt(input) - reference), ulp);
    EXPECT_TRUE(std::signbit(vectoris::numerics::core::sqrt(-T{0})));
}
} // namespace

TEST(PublicApiSurfaceTest, CoreSqrtFloatRuntimeAndConstexpr) {
    VerifyCanonicalSqrtBinaryApi<float>();
}
TEST(PublicApiSurfaceTest, CoreSqrtDoubleRuntimeAndConstexpr) {
    VerifyCanonicalSqrtBinaryApi<double>();
}
TEST(PublicApiSurfaceTest, CoreSqrtLongDoubleRuntime) {
    static_assert(vectoris::numerics::Concepts::SupportedSqrtScalar<long double>);
    volatile long double input = 2.0L;
    const long double reference = std::sqrt(input);
    const long double ulp = std::nextafter(reference, std::numeric_limits<long double>::infinity()) - reference;
    EXPECT_LE(std::abs(vectoris::numerics::core::sqrt(input) - reference), ulp);
    EXPECT_LE(std::abs(vectoris::numerics::core::Math::sqrt(input) - reference), ulp);
    EXPECT_TRUE(std::isnan(vectoris::numerics::core::sqrt(-1.0L)));
}
