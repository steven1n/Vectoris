#include <gtest/gtest.h>
#include "AegisDynamics/RigidBodyParameters.h"
#include "AegisDynamics/Wrench6.h"
#include "AegisDynamics/RigidBodyState.h"
#include "AegisDynamics/QuantityVector3.h"

struct WorldFrame {};
struct BodyFrame {};
struct OtherFrame {};

using namespace AegisDynamics;
using namespace AegisMath::Units;

// 编译期静态拒绝：坐标系安全约束
template <typename State, typename Params, typename Wrench>
concept CanComputeDerivativeWithMismatchedFrames = requires(State s, Params p, Wrench w) {
    { RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(s, p, w) };
};
static_assert(!CanComputeDerivativeWithMismatchedFrames<
    KinematicState<double, WorldFrame, OtherFrame>,
    RigidBodyParameters<double, BodyFrame>,
    Wrench6<double, BodyFrame>>,
    "Compile error expected: State BodyFrame must match Parameters BodyFrame.");

// Test A: 对角惯量后向兼容性回归测试
TEST(RigidBodyStateTest, DiagonalInertiaRegression) {
    Kilogram m{1000.0};
    Position3<BodyFrame> com;
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(100.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(200.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(300.0)
    );
    RigidBodyParameters<double, BodyFrame> params(m, com, inertia);

    Force3<BodyFrame> force(Force(1000.0), Force::Zero(), Force::Zero());
    Torque3<BodyFrame> moment(Torque(10.0), Torque(20.0), Torque(30.0));
    Wrench6<double, BodyFrame> wrench(force, moment);

    Position3<WorldFrame> init_pos;
    auto init_att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> init_vel;
    AngularVelocity3<BodyFrame> init_rate;

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(init_pos, init_att, init_vel, init_rate);

    auto res = RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(state, params, wrench);
    ASSERT_TRUE(res.has_value());
    const auto& deriv = res.value();

    EXPECT_DOUBLE_EQ(deriv.linear.x.value(), 1.0); // 1000 N / 1000 kg = 1 m/s^2
    EXPECT_DOUBLE_EQ(deriv.angular.x.value(), 0.1); // 10 Nm / 100 = 0.1 rad/s^2
    EXPECT_DOUBLE_EQ(deriv.angular.y.value(), 0.1); // 20 Nm / 200 = 0.1 rad/s^2
    EXPECT_DOUBLE_EQ(deriv.angular.z.value(), 0.1); // 30 Nm / 300 = 0.1 rad/s^2
}

// Test B: 非对角正定惯量张量与独立基准解对比 (Remediating AML-HIGH-003)
TEST(RigidBodyStateTest, NonDiagonalInertiaCouplingMatchesReference) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI(2.0),  MI(1.0),
        MI(2.0),  MI(12.0), MI(3.0),
        MI(1.0),  MI(3.0),  MI(15.0)
    );

    Kilogram mass(100.0);
    Position3<BodyFrame> com;
    RigidBodyParameters<double, BodyFrame> params(mass, com, inertia);

    Position3<WorldFrame> pos;
    auto att = AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<BodyFrame> vel;
    AngularVelocity3<BodyFrame> omega(AngularVelocity(1.0), AngularVelocity(2.0), AngularVelocity(-1.0));

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(pos, att, vel, omega);

    Force3<BodyFrame> force;
    Torque3<BodyFrame> torque_ext(Torque(10.0), Torque(-5.0), Torque(20.0));
    Wrench6<double, BodyFrame> wrench(force, torque_ext);

    auto res = RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(state, params, wrench);
    ASSERT_TRUE(res.has_value());
    const auto& deriv = res.value();

    // 独立基准真值: I \ (tau_ext - [w, Iw])
    // tau_net = [3.0, 0.0, 23.0]
    // 旧对角近似产生: [0.3, 0.0, 1.5333] (y 分量误差达 100%!)
    const double ref_x = 0.22727272727272727;
    const double ref_y = -0.43939393939393934;
    const double ref_z = 1.606060606060606;

    EXPECT_NEAR(deriv.angular.x.value(), ref_x, 1e-14);
    EXPECT_NEAR(deriv.angular.y.value(), ref_y, 1e-14);
    EXPECT_NEAR(deriv.angular.z.value(), ref_z, 1e-14);
}

// Test C: 零角速度状态退化测试: omega = 0 ==> I * alpha = tau_ext
TEST(RigidBodyStateTest, ZeroAngularVelocityDegeneration) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI(2.0),  MI(1.0),
        MI(2.0),  MI(12.0), MI(3.0),
        MI(1.0),  MI(3.0),  MI(15.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(50.0), Position3<BodyFrame>{}, inertia);

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>{},
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        Velocity3<BodyFrame>{},
        AngularVelocity3<BodyFrame>{} // omega = 0
    );

    Torque3<BodyFrame> torque_ext(Torque(5.0), Torque(10.0), Torque(-4.0));
    Wrench6<double, BodyFrame> wrench(Force3<BodyFrame>{}, torque_ext);

    auto res = RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(state, params, wrench);
    ASSERT_TRUE(res.has_value());

    // 验证 I * alpha ≈ tau_ext (使用强类型 Multiply 运算，无 .value() 标量重解释)
    Torque3<BodyFrame> reconstructed_tau = inertia.Multiply(res.value().angular);
    static_assert(std::is_same_v<decltype(reconstructed_tau), Torque3<BodyFrame>>,
        "Inertia * AngularAcceleration must yield Torque3.");

    // reconstructed_tau 数值应与 tau_ext 严格一致
    EXPECT_NEAR(reconstructed_tau.x.value(), torque_ext.x.value(), 1e-13);
    EXPECT_NEAR(reconstructed_tau.y.value(), torque_ext.y.value(), 1e-13);
    EXPECT_NEAR(reconstructed_tau.z.value(), torque_ext.z.value(), 1e-13);
}

// Test D: 无外力矩非对称刚体转动测试 (Torque-free asymmetric body):
// tau_ext = 0, omega != 0 不沿主轴 ==> alpha != 0 (经典欧拉动力学非对角/陀螺效应)
TEST(RigidBodyStateTest, TorqueFreeAsymmetricBodyNonZeroAcceleration) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> inertia(
        MI(10.0), MI::Zero(), MI::Zero(),
        MI::Zero(), MI(20.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(30.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, inertia);

    // omega = [1.0, 1.0, 1.0] rad/s (不沿任何主轴)
    AngularVelocity3<BodyFrame> omega(AngularVelocity(1.0), AngularVelocity(1.0), AngularVelocity(1.0));
    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>{},
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        Velocity3<BodyFrame>{},
        omega
    );

    Wrench6<double, BodyFrame> zero_wrench;

    auto res = RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(state, params, zero_wrench);
    ASSERT_TRUE(res.has_value());
    const auto& alpha = res.value().angular;

    // L = [10, 20, 30]
    // w x L = [1*30 - 1*20, 1*10 - 1*30, 1*20 - 1*10] = [10, -20, 10]
    // tau_net = -[10, -20, 10] = [-10, 20, -10]
    // alpha = [-10/10, 20/20, -10/30] = [-1.0, 1.0, -0.3333333333333333]
    EXPECT_NEAR(alpha.x.value(), -1.0, 1e-14);
    EXPECT_NEAR(alpha.y.value(), 1.0, 1e-14);
    EXPECT_NEAR(alpha.z.value(), -1.0 / 3.0, 1e-14);
}

// Test E: 奇异/不定惯量张量错误处理 (无静默假死或降级)
TEST(RigidBodyStateTest, SingularInertiaReturnsError) {
    using MI = MomentOfInertia;
    InertiaTensor3<double, BodyFrame> singular_inertia(
        MI(1.0), MI(1.0), MI::Zero(),
        MI(1.0), MI(1.0), MI::Zero(),
        MI::Zero(), MI::Zero(), MI(1.0)
    );
    RigidBodyParameters<double, BodyFrame> params(Kilogram(10.0), Position3<BodyFrame>{}, singular_inertia);

    auto state = KinematicState<double, WorldFrame, BodyFrame>::Create(
        Position3<WorldFrame>{},
        AegisMath::Geometry::Quaternion<double, BodyFrame, WorldFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value(),
        Velocity3<BodyFrame>{},
        AngularVelocity3<BodyFrame>{}
    );

    Torque3<BodyFrame> tau(Torque(1.0), Torque(1.0), Torque(1.0));
    Wrench6<double, BodyFrame> wrench(Force3<BodyFrame>{}, tau);

    auto res = RigidBodyDynamicsKernel<double, WorldFrame, BodyFrame>::ComputeDerivative(state, params, wrench);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), AegisMath::Core::MathError::singular_matrix);
}
