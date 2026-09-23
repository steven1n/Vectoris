#include <gtest/gtest.h>
#include <cstddef>
#include <array>
#include <cmath>
#include <limits>
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Numerics/Geometry/Transform3.h"

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
