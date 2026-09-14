#include <gtest/gtest.h>
#include <limits>
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"

struct BodyFrame {};
struct OtherFrame {};

using namespace AegisMath::Dynamics;
using namespace AegisMath::Units;

// 编译期静态拒绝：禁止跨坐标系力矩求解角加速度
template <typename I, typename T>
concept CanSolveWithMismatchedFrame = requires(I inertia, T tau) {
    { inertia.Solve(tau) };
};
static_assert(!CanSolveWithMismatchedFrame<InertiaTensor3<double, BodyFrame>, Torque3<OtherFrame>>,
    "Compile error expected: InertiaTensor3 must reject torque in mismatched coordinate frame.");

TEST(InertiaTensorTest, ValidSPDInertia) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> tensor(
        MI(10.0), MI(2.0),  MI(1.0),
        MI(2.0),  MI(12.0), MI(3.0),
        MI(1.0),  MI(3.0),  MI(15.0)
    );
    EXPECT_TRUE(tensor.IsValid());
}

TEST(InertiaTensorTest, IndefiniteWithPositiveDiagonalRejected) {
    // AML-MED-003 复现用例：对角元素为正，但为不定矩阵 (det = 1*(1-0) - 2*(2-0) = -3 < 0)
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> indefinite_tensor(
        MI(1.0), MI(2.0), MI(0.0),
        MI(2.0), MI(1.0), MI(0.0),
        MI(0.0), MI(0.0), MI(1.0)
    );
    EXPECT_FALSE(indefinite_tensor.IsValid());
}

TEST(InertiaTensorTest, NegativeDiagonalRejected) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> invalid_tensor(
        MI(-1.0), MI(0.0), MI(0.0),
        MI(0.0),  MI(20.0), MI(0.0),
        MI(0.0),  MI(0.0),  MI(30.0)
    );
    EXPECT_FALSE(invalid_tensor.IsValid());
}

TEST(InertiaTensorTest, AsymmetryRejected) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> asym_tensor(
        MI(10.0), MI(2.5), MI(0.0),
        MI(1.0),  MI(20.0), MI(0.0), // ixy != iyx
        MI(0.0),  MI(0.0),  MI(30.0)
    );
    EXPECT_FALSE(asym_tensor.IsValid());
}

TEST(InertiaTensorTest, SingularRejected) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> singular_tensor(
        MI(1.0), MI(1.0), MI(0.0),
        MI(1.0), MI(1.0), MI(0.0),
        MI(0.0), MI(0.0), MI(1.0)
    );
    EXPECT_FALSE(singular_tensor.IsValid());
}

TEST(InertiaTensorTest, NonFiniteRejected) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> nan_tensor(
        MI(std::numeric_limits<double>::quiet_NaN()), MI(0.0), MI(0.0),
        MI(0.0), MI(20.0), MI(0.0),
        MI(0.0), MI(0.0), MI(30.0)
    );
    EXPECT_FALSE(nan_tensor.IsValid());
}

TEST(InertiaTensorTest, TypedSolveSPD) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> I(
        MI(10.0), MI(2.0),  MI(1.0),
        MI(2.0),  MI(12.0), MI(3.0),
        MI(1.0),  MI(3.0),  MI(15.0)
    );

    Torque3<BodyFrame> tau(Torque(3.0), Torque::Zero(), Torque(23.0));

    auto res = I.Solve(tau);
    ASSERT_TRUE(res.has_value());
    auto alpha = res.value();

    const double ref_x = 0.22727272727272727;
    const double ref_y = -0.43939393939393934;
    const double ref_z = 1.606060606060606;

    EXPECT_NEAR(alpha.x.value(), ref_x, 1e-14);
    EXPECT_NEAR(alpha.y.value(), ref_y, 1e-14);
    EXPECT_NEAR(alpha.z.value(), ref_z, 1e-14);

    // 验证自由函数 SolveSPD 表现一致
    auto res_fn = SolveSPD(I, tau);
    ASSERT_TRUE(res_fn.has_value());
    EXPECT_DOUBLE_EQ(res_fn.value().x.value(), alpha.x.value());
    EXPECT_DOUBLE_EQ(res_fn.value().y.value(), alpha.y.value());
    EXPECT_DOUBLE_EQ(res_fn.value().z.value(), alpha.z.value());
}
