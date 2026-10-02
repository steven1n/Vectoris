#include <gtest/gtest.h>
#include <cstddef>
#include <array>
#include <cmath>
#include <limits>
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Numerics/Geometry/Transform3.h"
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"

using namespace vectoris::numerics::Geometry;

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

    // Cover TryCreate validation branches for FrameA->FrameB and FrameB->FrameC instantiations
    EXPECT_FALSE((Quaternion<double, FrameA, FrameB>::TryCreate(0.0, 0.0, 0.0, 0.0).IsSuccess()));
    EXPECT_FALSE((Quaternion<double, FrameA, FrameB>::TryCreate(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0).IsSuccess()));
    auto q_neg1 = Quaternion<double, FrameA, FrameB>::TryCreate(-1.0, 0.0, 0.0, 0.0);
    EXPECT_TRUE(q_neg1.IsSuccess());

    EXPECT_FALSE((Quaternion<double, FrameB, FrameC>::TryCreate(0.0, 0.0, 0.0, 0.0).IsSuccess()));
    EXPECT_FALSE((Quaternion<double, FrameB, FrameC>::TryCreate(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0).IsSuccess()));
    auto q_neg2 = Quaternion<double, FrameB, FrameC>::TryCreate(-1.0, 0.0, 0.0, 0.0);
    EXPECT_TRUE(q_neg2.IsSuccess());
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
namespace {
template <typename T>
class QuaternionScaleTest : public ::testing::Test {};
using QuaternionScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(QuaternionScaleTest, QuaternionScalars);

TYPED_TEST(QuaternionScaleTest, NormalizesAcrossFiniteExponentRange) {
    using T = TypeParam;
    using Q = Quaternion<T, FrameA, FrameB>;
    const std::array<T, 6> scales{
        std::numeric_limits<T>::denorm_min(), std::numeric_limits<T>::min(),
        T{1e-5F}, T{1}, std::numeric_limits<T>::max() / T{4},
        std::numeric_limits<T>::max()};
    const T tolerance = std::numeric_limits<T>::epsilon() * T{32};
    for (const T scale : scales) {
        SCOPED_TRACE(scale);
        const auto result = Q::TryCreate(scale, -scale, scale, -scale);
        ASSERT_TRUE(result.IsSuccess());
        const auto& q = result.Value();
        // Analytic normalized value, independent of the implementation's norm.
        EXPECT_NEAR(q.w, T{0.5}, tolerance);
        EXPECT_NEAR(q.x, T{-0.5}, tolerance);
        EXPECT_NEAR(q.y, T{0.5}, tolerance);
        EXPECT_NEAR(q.z, T{-0.5}, tolerance);
        EXPECT_NEAR(std::hypot(std::hypot(q.w, q.x), std::hypot(q.y, q.z)),
                    T{1}, tolerance);
        const auto rotation = q.ToRotationMatrix();
        ASSERT_TRUE(rotation.IsSuccess());
        const auto mapped = rotation.Value() * Vector3<T, FrameA>(T{1}, T{0}, T{0});
        EXPECT_NEAR(mapped.x, T{0}, tolerance);
        EXPECT_NEAR(mapped.y, T{-1}, tolerance);
        EXPECT_NEAR(mapped.z, T{0}, tolerance);
        EXPECT_NEAR(rotation.Value().ToMatrix().det(), T{1}, tolerance);
        const auto single_axis = Q::TryCreate(-scale, T{0}, T{0}, T{0});
        ASSERT_TRUE(single_axis.IsSuccess());
        EXPECT_NEAR(single_axis.Value().w, T{1}, tolerance);
        ASSERT_TRUE(single_axis.Value().ToRotationMatrix().IsSuccess());
    }
}

TYPED_TEST(QuaternionScaleTest, RejectsInvalidInputAndModifiedState) {
    using T = TypeParam;
    using Q = Quaternion<T, FrameA, FrameA>;
    using Error = vectoris::numerics::Core::MathError;
    const auto zero = Q::TryCreate(T{0}, -T{0}, T{0}, -T{0});
    ASSERT_FALSE(zero.IsSuccess());
    EXPECT_EQ(zero.error(), Error::zero_norm);
    for (const T bad : {std::numeric_limits<T>::infinity(),
                        -std::numeric_limits<T>::infinity(),
                        std::numeric_limits<T>::quiet_NaN()}) {
        for (std::size_t component = 0; component < 4; ++component) {
            std::array<T, 4> input{T{1}, T{0}, T{0}, T{0}};
            input[component] = bad;
            const auto result = Q::TryCreate(input[0], input[1], input[2], input[3]);
            ASSERT_FALSE(result.IsSuccess());
            EXPECT_EQ(result.error(), Error::non_finite_input);
            auto modified = Q::Identity();
            std::array<T*, 4> members{&modified.w, &modified.x, &modified.y, &modified.z};
            *members[component] = bad;
            const auto rotation = modified.ToRotationMatrix();
            ASSERT_FALSE(rotation.IsSuccess());
            EXPECT_EQ(rotation.error(), Error::non_finite_input);
        }
    }
    for (const T bad : {T{0}, T{0.5}, std::numeric_limits<T>::max()}) {
        for (std::size_t component = 0; component < 4; ++component) {
            auto modified = Q::Identity();
            modified.w = T{0};
            std::array<T*, 4> members{&modified.w, &modified.x, &modified.y, &modified.z};
            *members[component] = bad;
            const auto rotation = RotationMatrix3<T, FrameA, FrameA>::FromQuaternion(modified);
            ASSERT_FALSE(rotation.IsSuccess());
            EXPECT_EQ(rotation.error(), Error::invalid_state);
        }
    }
}

TEST(QuaternionTest, AccumulatedDriftIsCheckedAndCanBeRenormalized) {
    using Q = Quaternion<double, FrameA, FrameA>;
    const auto step = Q::TryCreate(std::cos(0.01), 0.0, 0.0, std::sin(0.01));
    ASSERT_TRUE(step.IsSuccess());
    auto accumulated = Q::Identity();
    constexpr int max_iterations = 100000;
    for (int i = 0; i < max_iterations; ++i) {
        accumulated = accumulated * step.Value();
    }
    const auto rotation = accumulated.ToRotationMatrix();
    if (!rotation.IsSuccess()) {
        EXPECT_EQ(rotation.error(), vectoris::numerics::Core::MathError::invalid_state);
    } else {
        EXPECT_NEAR(rotation.Value().ToMatrix().det(), 1.0, 1e-13);
    }
    const auto normalized = Q::TryCreate(accumulated.w, accumulated.x, accumulated.y, accumulated.z);
    ASSERT_TRUE(normalized.IsSuccess());
    const auto recovered = normalized.Value().ToRotationMatrix();
    ASSERT_TRUE(recovered.IsSuccess());
    const auto mapped = recovered.Value() * Vector3<double, FrameA>(1.0, 0.0, 0.0);
    EXPECT_NEAR(mapped.x, std::cos(2000.0), 1e-10);
    EXPECT_NEAR(mapped.y, std::sin(2000.0), 1e-10);
    EXPECT_NEAR(mapped.z, 0.0, 1e-14);
}
} // namespace

// AFA-001: analytic rotations exercise representable answers at exponent limits.
namespace {
template <typename T>
void CheckAFA001Axes() {
    using Q = Quaternion<T, FrameA, FrameB>;
    using V = Vector3<T, FrameA>;
    const T hi = std::numeric_limits<T>::max();
    const T tiny = std::numeric_limits<T>::denorm_min();
    const std::array<T, 7> scales{hi, hi / T{2}, hi * T{0.75}, T{1},
                                 std::numeric_limits<T>::min(), tiny, -hi};
    for (const T s : scales) {
        const V v{s, -s, tiny};
        const auto rx = Q::TryCreate(T{0}, T{1}, T{0}, T{0}).value() * v;
        const auto ry = Q::TryCreate(T{0}, T{0}, T{1}, T{0}).value() * v;
        const auto rz = Q::TryCreate(T{0}, T{0}, T{0}, T{1}).value() * v;
        // Exact analytic sign/permutation operations, not computed-value heuristics.
        EXPECT_EQ(rx.x, s); EXPECT_EQ(rx.y, s); EXPECT_EQ(rx.z, -tiny);
        EXPECT_EQ(ry.x, -s); EXPECT_EQ(ry.y, -s); EXPECT_EQ(ry.z, -tiny);
        EXPECT_EQ(rz.x, -s); EXPECT_EQ(rz.y, s); EXPECT_EQ(rz.z, tiny);
        const auto cyclic = Q::TryCreate(T{1}, T{1}, T{1}, T{1}).value() * v;
        EXPECT_EQ(cyclic.x, tiny); EXPECT_EQ(cyclic.y, s); EXPECT_EQ(cyclic.z, -s);
    }
    const auto q = Q::TryCreate(T{0}, T{1}, T{0}, T{0}).value();
    const auto zero = q * V{-T{0}, T{0}, -T{0}};
    EXPECT_TRUE(std::signbit(zero.x)); EXPECT_FALSE(std::signbit(zero.y));
    EXPECT_TRUE(std::signbit(zero.z));
    const auto original = q * V{T{0}, hi, T{0}};
    EXPECT_EQ(original.y, -hi);
    const auto inverse_sign = q * V{T{0}, -hi, T{0}};
    EXPECT_EQ(inverse_sign.y, hi);
}

template <typename T>
void CheckAFA001General() {
    using Q = Quaternion<T, FrameA, FrameB>;
    const auto q = Q::TryCreate(T{1}, T{2}, T{3}, T{4}).value();
    // Independent Rodrigues oracle from the original axis/angle, in long double.
    const long double axis_norm = std::sqrt(29.L);
    const std::array<long double, 3> axis{2.L / axis_norm, 3.L / axis_norm, 4.L / axis_norm};
    const long double angle = 2.L * std::atan2(axis_norm, 1.L);
    const long double sine = std::sin(angle), cosine = std::cos(angle);
    const std::array<long double, 3> direction{0.125L, -0.25L, 0.375L};
    const long double dot = axis[0]*direction[0] + axis[1]*direction[1] + axis[2]*direction[2];
    const std::array<long double, 3> cross{axis[1]*direction[2]-axis[2]*direction[1],
        axis[2]*direction[0]-axis[0]*direction[2], axis[0]*direction[1]-axis[1]*direction[0]};
    for (const T scale : {T{1}, std::numeric_limits<T>::max(), std::numeric_limits<T>::min()}) {
        const auto v = Vector3<T, FrameA>{scale*T{0.125}, -scale*T{0.25}, scale*T{0.375}};
        const auto actual = q * v;
        const std::array<T, 3> parts{actual.x, actual.y, actual.z};
        for (std::size_t i=0; i<3; ++i) {
            const long double expected = direction[i]*cosine + cross[i]*sine + axis[i]*dot*(1.L-cosine);
            EXPECT_TRUE(std::isfinite(parts[i]));
            const long double error = std::abs(
                static_cast<long double>(parts[i])/static_cast<long double>(scale) - expected);
            EXPECT_LE(error, 16.L*static_cast<long double>(std::numeric_limits<T>::epsilon()));
        }
    }
}
}
TEST(AFA001Rotation, FloatAnalyticExtremes) { CheckAFA001Axes<float>(); }
TEST(AFA001Rotation, DoubleAnalyticExtremes) { CheckAFA001Axes<double>(); }
TEST(AFA001Rotation, FloatIndependentRodrigues) { CheckAFA001General<float>(); }
TEST(AFA001Rotation, DoubleIndependentRodrigues) { CheckAFA001General<double>(); }
TEST(AFA001Rotation, ConstexprAndMixedPrecision) {
    constexpr auto identity = Quaternion<double, FrameA, FrameA>::Identity();
    constexpr auto out = identity * Vector3<float, FrameA>{1.F, 2.F, 3.F};
    static_assert(out.x == 1. && out.y == 2. && out.z == 3.);
    static_assert(std::same_as<std::remove_cv_t<decltype(out)>, Vector3<double, FrameA>>);
    EXPECT_DOUBLE_EQ(out.z, 3.);
    const auto tiny = std::numeric_limits<double>::denorm_min();
    const auto mixed = identity * Vector3<double, FrameA>{std::numeric_limits<double>::max(), tiny, -tiny};
    EXPECT_EQ(mixed.y, tiny); EXPECT_EQ(mixed.z, -tiny);
}

namespace {
template <typename T>
void CheckAFA001RoundedBoundary() {
    using Q = Quaternion<T, FrameA, FrameA>;
    using V = Vector3<T, FrameA>;
    auto q = Q::Identity();
    V v;
    // Frozen binary inputs, independently evaluated with 120-digit Decimal
    // arithmetic on the exact stored values. Both x outputs round to max():
    // float: exact/max=0.99999999999999990890...
    // double: exact/max=0.99999999999999997885...
    if constexpr (std::same_as<T, float>) {
        q.w = 0x1.47f146p-3F; q.x = q.w; q.y = q.w; q.z = 0x1.ebe9e8p-1F;
        v = V{-0x1.cb7cb6p+127F, -0x1.069068p+126F, 0x1.6f96f8p+126F};
    } else {
        q.w = 0x1.3c03650e00e02p-3; q.x = q.w;
        q.y = 0x1.3c03650e00e02p-2; q.z = 0x1.da05179501504p-1;
        v = V{-0x1.cf3cf3cf3cf3cp+1023, -0x1.8618618618617p+1021, 0x1.8618618618617p+1022};
    }
    ASSERT_TRUE(q.ToRotationMatrix().has_value());
    const T maximum = std::numeric_limits<T>::max();
    V unrepresentable{-maximum, -maximum, maximum};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto positive = q * v, negative = q * (-v);
        const std::array<T, 3> p{positive.x, positive.y, positive.z};
        const std::array<T, 3> n{negative.x, negative.y, negative.z};
        EXPECT_EQ(p[axis], maximum); EXPECT_EQ(n[axis], -maximum);
        // A true >max result is not silently saturated to a finite sentinel.
        const auto overflow = q * unrepresentable;
        const auto negative_overflow = q * (-unrepresentable);
        const std::array<T, 3> o{overflow.x, overflow.y, overflow.z};
        const std::array<T, 3> no{negative_overflow.x, negative_overflow.y, negative_overflow.z};
        EXPECT_EQ(o[axis], std::numeric_limits<T>::infinity());
        EXPECT_EQ(no[axis], -std::numeric_limits<T>::infinity());
        const T old_x = q.x; q.x = q.z; q.z = q.y; q.y = old_x;
        v = V{v.z, v.x, v.y};
        unrepresentable = V{unrepresentable.z, unrepresentable.x, unrepresentable.y};
    }
}
}
TEST(AFA001Rotation, FloatRoundedOverflowBoundary) { CheckAFA001RoundedBoundary<float>(); }
TEST(AFA001Rotation, DoubleRoundedOverflowBoundary) { CheckAFA001RoundedBoundary<double>(); }
TEST(AFA001Rotation, ConstexprRoundedOverflowBoundary) {
    constexpr auto actual = [] {
        auto q = Quaternion<double, FrameA, FrameA>::Identity();
        q.w = 0x1.3c03650e00e02p-3; q.x = q.w;
        q.y = 0x1.3c03650e00e02p-2; q.z = 0x1.da05179501504p-1;
        return q * Vector3<double, FrameA>{-0x1.cf3cf3cf3cf3cp+1023,
            -0x1.8618618618617p+1021, 0x1.8618618618617p+1022};
    }();
    static_assert(actual.x == std::numeric_limits<double>::max());
    EXPECT_EQ(actual.x, std::numeric_limits<double>::max());
}

namespace {
template<class T> void C11AuditRotation() {
    const auto q=Quaternion<T,FrameA,FrameA>::TryCreate(T{0},T{1},T{1},T{1}).Value();
    const auto rotation=q.ToRotationMatrix();ASSERT_TRUE(rotation.IsSuccess());
    const T max=std::numeric_limits<T>::max();
    const Vector3<T,FrameA> v = [] {
        const T m=std::numeric_limits<T>::max();
        if constexpr (std::same_as<T,float>) return Vector3<T,FrameA>{m,m,m};
        else return Vector3<T,FrameA>{m,-m,-m};
    }();
    const auto result=rotation.Value()*v;
    if constexpr (std::same_as<T,float>) {
        EXPECT_FLOAT_EQ(result.z,0x1.fffffcp+127f);
    } else {
        EXPECT_DOUBLE_EQ(result.y,0x1.5555555555556p+1022);
    }
    const auto negative=rotation.Value()*(-v);
    const std::array<T,3> actual{result.x,result.y,result.z},neg{negative.x,negative.y,negative.z};
    for(size_t i=0;i<3;++i){
        EXPECT_EQ(neg[i],-actual[i]);
        if constexpr (std::same_as<T,double>) { if(i==0){EXPECT_EQ(actual[i],-std::numeric_limits<T>::infinity());continue;} }
        EXPECT_TRUE(std::isfinite(actual[i]));EXPECT_LE(std::abs(actual[i]),max);
    }
}
template<class T> void C11RotationAxes() {
    const T max=std::numeric_limits<T>::max(),tiny=std::numeric_limits<T>::denorm_min();
    for(size_t axis=0;axis<3;++axis){
        const std::array<T,3> unit{axis==0?T{1}:T{0},axis==1?T{1}:T{0},axis==2?T{1}:T{0}};
        const auto q=Quaternion<T,FrameA,FrameA>::TryCreate(T{0},unit[0],unit[1],unit[2]).Value();
        const auto r=q.ToRotationMatrix().Value();
        for(T value:{max,-max,max/T{2},tiny,-tiny,T{0}}){
            const auto a=r*Vector3<T,FrameA>{value,value,value};
            const std::array<T,3> components{a.x,a.y,a.z};
            for(size_t i=0;i<3;++i) EXPECT_EQ(components[i],i==axis?value:-value);
        }
    }
    const auto identity=RotationMatrix3<T,FrameA,FrameA>::Identity();
    const auto mixed=identity*Vector3<T,FrameA>{max,tiny,-tiny};
    EXPECT_EQ(mixed.x,max);EXPECT_EQ(mixed.y,tiny);EXPECT_EQ(mixed.z,-tiny);
}
template<class T> void C11OperationalRotation() {
    // Exact analytic rotation for input quaternion (1,2,3,4), norm squared 30.
    // The oracle never calls a production rotation or matrix arithmetic path.
    const auto q=Quaternion<T,FrameA,FrameA>::TryCreate(T{1},T{2},T{3},T{4}).Value();
    const auto r=q.ToRotationMatrix().Value();
    const auto transform=Transform3<T,FrameA,FrameA>::Create(q,Vector3<T,FrameA>{});
    constexpr long double coefficients[3][3]{{-20.L/30,4.L/30,22.L/30},
        {20.L/30,-10.L/30,20.L/30},{10.L/30,28.L/30,4.L/30}};
    for(T scale : {static_cast<T>(1e-9),static_cast<T>(1e-6),T{1},T{1e3},T{7e6},T{1e9},static_cast<T>(1e12)}){
        const Vector3<T,FrameA> v{scale,scale*T{-2},scale*T{3}};
        const auto matrix=r*v,quat=q*v,trans=transform*v;
        const std::array<T,3> m{matrix.x,matrix.y,matrix.z},a{quat.x,quat.y,quat.z},b{trans.x,trans.y,trans.z};
        const long double input[3]{static_cast<long double>(v.x),static_cast<long double>(v.y),static_cast<long double>(v.z)};
        for(size_t i=0;i<3;++i){
            const long double expected=coefficients[i][0]*input[0]+coefficients[i][1]*input[1]+coefficients[i][2]*input[2];
            const long double tolerance=64.L*std::numeric_limits<T>::epsilon()*std::abs(expected);
            EXPECT_LE(std::abs(static_cast<long double>(m[i])-expected),tolerance);
            EXPECT_LE(std::abs(static_cast<long double>(a[i])-expected),tolerance);
            EXPECT_LE(std::abs(static_cast<long double>(b[i])-expected),tolerance);
        }
        const long double input_norm=std::hypot(input[0],input[1],input[2]);
        const long double output_norm=std::hypot(static_cast<long double>(m[0]),static_cast<long double>(m[1]),static_cast<long double>(m[2]));
        EXPECT_LE(std::abs(output_norm/input_norm-1.L),64.L*std::numeric_limits<T>::epsilon());
    }
}
template<class T> void C11CancellationRotation() {
    // Exactly stored signed permutation/cancellation rotation coefficients.
    const auto q=Quaternion<T,FrameA,FrameA>::TryCreate(T{0},T{1},T{1},T{1}).Value();
    const auto r=q.ToRotationMatrix().Value();
    const T max=std::numeric_limits<T>::max();
    for(T scale:{max/T{2},static_cast<T>(1e12),T{1},std::numeric_limits<T>::min()}) {
        for(const auto& v:std::array<Vector3<T,FrameA>,3>{
            Vector3<T,FrameA>{scale,-scale,scale/T{2}},
            Vector3<T,FrameA>{scale,scale/T{4},-scale/T{2}},
            Vector3<T,FrameA>{-scale/T{4},scale,-scale/T{2}}}) {
            const auto a=r*v;
            const auto& m=r.ToMatrix();
            const std::array<T,3> actual{a.x,a.y,a.z};
            for(size_t i=0;i<3;++i){
                const long double reference=(static_cast<long double>(m(i,0))*static_cast<long double>(v.x)+
                    static_cast<long double>(m(i,1))*static_cast<long double>(v.y))+
                    static_cast<long double>(m(i,2))*static_cast<long double>(v.z);
                const long double tolerance=8.L*std::numeric_limits<T>::epsilon()*static_cast<long double>(scale)+
                    4.L*std::numeric_limits<T>::denorm_min();
                ASSERT_TRUE(std::isfinite(actual[i]));
                EXPECT_LE(std::abs(static_cast<long double>(actual[i])-reference),tolerance);
            }
        }
    }
}
template<class T> void C11NonFiniteRotation() {
    const auto r=RotationMatrix3<T,FrameA,FrameA>::Identity();
    const auto nan=r*Vector3<T,FrameA>{std::numeric_limits<T>::quiet_NaN(),T{1},T{2}};
    EXPECT_TRUE(std::isnan(nan.x));
    const auto inf=r*Vector3<T,FrameA>{std::numeric_limits<T>::infinity(),T{1},T{2}};
    EXPECT_TRUE(std::isinf(inf.x));
    const auto overflow=Quaternion<T,FrameA,FrameA>::TryCreate(T{1},T{0},T{0},T{1}).Value().ToRotationMatrix().Value()*
        Vector3<T,FrameA>{std::numeric_limits<T>::max(),-std::numeric_limits<T>::max(),T{0}};
    EXPECT_TRUE(std::isfinite(overflow.x));
}
}
TEST(C11Rotation, FloatOriginalAuditReproducer){C11AuditRotation<float>();}
TEST(C11Rotation, DoubleOriginalAuditReproducer){C11AuditRotation<double>();}
TEST(C11Rotation, FloatAxesAndMixedExponents){C11RotationAxes<float>();}
TEST(C11Rotation, DoubleAxesAndMixedExponents){C11RotationAxes<double>();}
TEST(C11Rotation, FloatOperationalRange){C11OperationalRotation<float>();}
TEST(C11Rotation, DoubleOperationalRange){C11OperationalRotation<double>();}
TEST(C11Rotation, FloatCancellation){C11CancellationRotation<float>();}
TEST(C11Rotation, DoubleCancellation){C11CancellationRotation<double>();}
TEST(C11Rotation, FloatNonFinitePropagation){C11NonFiniteRotation<float>();}
TEST(C11Rotation, DoubleNonFinitePropagation){C11NonFiniteRotation<double>();}
TEST(C11Rotation, ConstexprIdentityBoundary){
    constexpr auto a=RotationMatrix3<double,FrameA,FrameA>::Identity()*Vector3<double,FrameA>{std::numeric_limits<double>::max(),1.,-1.};
    static_assert(a.x==std::numeric_limits<double>::max() && a.y==1. && a.z==-1.);
    EXPECT_EQ(a.x,std::numeric_limits<double>::max());
}

namespace {
template<class T> void C11QuantityRotation() {
    using Quantity=vectoris::numerics::units::Quantity<T,vectoris::numerics::units::MeterUnit>;
    const auto q=Quaternion<T,FrameA,FrameB>::TryCreate(T{0},T{1},T{1},T{1}).Value();
    const auto r=q.ToRotationMatrix().Value();
    const T max=std::numeric_limits<T>::max();
    const auto v=[] {
        const T m=std::numeric_limits<T>::max();
        if constexpr(std::same_as<T,float>)return Vector3<Quantity,FrameA>{Quantity{m},Quantity{m},Quantity{m}};
        else return Vector3<Quantity,FrameA>{Quantity{m},Quantity{-m},Quantity{-m}};
    }();
    const auto out=r*v;
    static_assert(std::same_as<std::remove_cvref_t<decltype(out)>,Vector3<Quantity,FrameB>>);
    if constexpr(std::same_as<T,float>) EXPECT_FLOAT_EQ(out.z.value(),0x1.fffffcp+127f);
    else EXPECT_DOUBLE_EQ(out.y.value(),0x1.5555555555556p+1022);
    EXPECT_TRUE(std::isfinite(out.y.value()));EXPECT_TRUE(std::isfinite(out.z.value()));
    const auto mixed=r*Vector3<Quantity,FrameA>{Quantity{max/T{2}},Quantity{T{1}},Quantity{-max/T{2}}};
    EXPECT_TRUE(std::isfinite(mixed.x.value()));EXPECT_TRUE(std::isfinite(mixed.y.value()));EXPECT_TRUE(std::isfinite(mixed.z.value()));
}
}
TEST(C11Rotation, FloatQuantityKeepsUnitsAndFrame){C11QuantityRotation<float>();}
TEST(C11Rotation, DoubleQuantityKeepsUnitsAndFrame){C11QuantityRotation<double>();}

// C12: instantiate both safe-kernel selectors under each compiler's strict warnings.
TEST(C12RotationControlFlow, PlainFloatingBranches) {
    const auto rf=RotationMatrix3<float,FrameA,FrameA>::Identity();
    const auto rd=RotationMatrix3<double,FrameA,FrameA>::Identity();
    const auto f=rf*Vector3<float,FrameA>{1.f,-2.f,3.f};
    const auto d=rd*Vector3<double,FrameA>{1.,-2.,3.};
    EXPECT_FLOAT_EQ(f.x,1.f); EXPECT_FLOAT_EQ(f.y,-2.f); EXPECT_FLOAT_EQ(f.z,3.f);
    EXPECT_DOUBLE_EQ(d.x,1.); EXPECT_DOUBLE_EQ(d.y,-2.); EXPECT_DOUBLE_EQ(d.z,3.);
}
TEST(C12RotationControlFlow, QuantityBranchPreservesUnitsAndFrame) {
    using Quantity=vectoris::numerics::units::Quantity<double,vectoris::numerics::units::MeterUnit>;
    const auto r=Quaternion<double,FrameA,FrameB>::TryCreate(0.,1.,1.,1.).Value().ToRotationMatrix().Value();
    const auto v=Vector3<Quantity,FrameA>{Quantity{1.},Quantity{1.},Quantity{1.}};
    const auto out=r*v;
    static_assert(std::same_as<std::remove_cvref_t<decltype(out)>,Vector3<Quantity,FrameB>>);
    EXPECT_NEAR(out.x.value(),1.,1e-14);
    EXPECT_NEAR(out.y.value(),1.,1e-14);
    EXPECT_NEAR(out.z.value(),1.,1e-14);
}
