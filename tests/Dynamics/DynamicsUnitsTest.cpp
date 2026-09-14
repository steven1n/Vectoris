#include <gtest/gtest.h>
#include <type_traits>
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Dynamics/RigidBodyParameters.h"
#include "AegisMath/Dynamics/Detail/StateTypes.h"
#include "AegisMath/Dynamics/Twist6.h"
#include "AegisMath/Dynamics/Wrench6.h"
#include "AegisMath/Dynamics/EulerIntegrator.h"

struct TestRefFrame {};
struct TestBodyFrame {};

using namespace AegisMath::Dynamics;
using namespace AegisMath::Units;

// ==============================================================================
// 1. 编译期静态拒绝测试 (Compile-Time Negative Tests via Concepts & Requires)
// ==============================================================================

// Negative Test 1: 质量与长度不可相加
template <typename M, typename L>
concept CanAddMassAndLength = requires(M m, L l) {
    { m + l };
};
static_assert(!CanAddMassAndLength<Kilogram, Meter>, 
    "Compile error expected: Mass and Length must not be addable.");

// Negative Test 2: 线速度与线加速度不可相加
template <typename V, typename A>
concept CanAddVelocityAndAcceleration = requires(V v, A a) {
    { v + a };
};
static_assert(!CanAddVelocityAndAcceleration<Velocity3<TestBodyFrame>, Acceleration3<TestBodyFrame>>,
    "Compile error expected: Velocity3 and Acceleration3 must not be addable.");

// Negative Test 3: 力向量与力标量不可赋值给力矩 (量纲隔离)
static_assert(!std::is_assignable_v<Torque3<TestBodyFrame>&, Force3<TestBodyFrame>>,
    "Compile error expected: Force3 must not be assignable to Torque3.");
static_assert(!std::is_constructible_v<Torque3<TestBodyFrame>, Force3<TestBodyFrame>>,
    "Compile error expected: Torque3 must not be constructible from Force3.");
static_assert(!std::is_assignable_v<Torque&, Force>,
    "Compile error expected: ordinary Force must not be assignable to Torque.");
static_assert(!std::is_constructible_v<Torque, Force>,
    "Compile error expected: Torque must not be constructible from Force.");

// Negative Test 4: 时间标量不可赋值给质量标量
static_assert(!std::is_assignable_v<Kilogram&, Second>,
    "Compile error expected: Second must not be assignable to Kilogram.");
static_assert(!std::is_constructible_v<Kilogram, Second>,
    "Compile error expected: Kilogram must not be constructible from Second.");

// Negative Test 5: 机体系速度不可赋值给参考系速度 (坐标系隔离)
static_assert(!std::is_assignable_v<Velocity3<TestRefFrame>&, Velocity3<TestBodyFrame>>,
    "Compile error expected: BodyFrame velocity must not be assignable to RefFrame velocity.");
static_assert(!std::is_constructible_v<Velocity3<TestRefFrame>, Velocity3<TestBodyFrame>>,
    "Compile error expected: RefFrame velocity must not be constructible from BodyFrame velocity.");

// Negative Test 6: 角速度不可与线速度混淆
static_assert(!std::is_assignable_v<Velocity3<TestBodyFrame>&, AngularVelocity3<TestBodyFrame>>,
    "Compile error expected: AngularVelocity3 must not be assignable to Velocity3.");

// Negative Test 7: 跨坐标系加减法被类型系统直接禁止
template <typename V1, typename V2>
concept CanAddVectors = requires(V1 a, V2 b) {
    { a + b };
};
static_assert(!CanAddVectors<Velocity3<TestRefFrame>, Velocity3<TestBodyFrame>>,
    "Compile error expected: BodyFrame velocity and RefFrame velocity must not be addable.");

// Negative Test 8: 力矩与能量隔离 (Torque [M L^2 T^-2 A^-1] != Energy [M L^2 T^-2 A^0])
struct TestEnergyUnit {
    using Dimension = EnergyDimension;
    using Ratio     = std::ratio<1>;
    static constexpr bool IsBaseUnit = false;
};
using Energy = Quantity<double, TestEnergyUnit>;
static_assert(!std::is_assignable_v<Energy&, Torque>,
    "Compile error expected: Torque must not be assignable to Energy.");
static_assert(!std::is_constructible_v<Energy, Torque>,
    "Compile error expected: Energy must not be constructible from Torque.");

// Negative Test 9: 转动惯量与普通 Mass * Length^2 隔离 (MomentOfInertia [M L^2 A^-2] != Mass*Length^2 [M L^2 A^0])
using MassLengthSquared = decltype(std::declval<Kilogram>() * std::declval<Meter>() * std::declval<Meter>());
static_assert(!std::is_assignable_v<MomentOfInertia&, MassLengthSquared>,
    "Compile error expected: Mass * Length^2 must not be assignable to MomentOfInertia.");
static_assert(!std::is_constructible_v<MomentOfInertia, MassLengthSquared>,
    "Compile error expected: MomentOfInertia must not be constructible from Mass * Length^2.");

// ==============================================================================
// 2. 编译期转动量纲恒等式测试 (Model B Compile-Time Identities)
// ==============================================================================

// Identity 1: MomentOfInertia * AngularAcceleration -> Torque
using ProdIA = decltype(std::declval<MomentOfInertia>() * std::declval<AngularAcceleration>());
static_assert(DimensionEqual<typename ProdIA::DimensionType, TorqueDimension>,
    "Identity 1 violation: MomentOfInertia * AngularAcceleration must yield Torque dimension.");
static_assert(std::is_constructible_v<Torque, ProdIA>,
    "Identity 1 violation: Torque must be constructible from MomentOfInertia * AngularAcceleration.");

// Identity 2: Torque * AngularVelocity -> Power
using ProdTauOmega = decltype(std::declval<Torque>() * std::declval<AngularVelocity>());
static_assert(DimensionEqual<typename ProdTauOmega::DimensionType, PowerDimension>,
    "Identity 2 violation: Torque * AngularVelocity must yield Power dimension.");
static_assert(std::is_constructible_v<Power, ProdTauOmega>,
    "Identity 2 violation: Power must be constructible from Torque * AngularVelocity.");

// Identity 3: Power / AngularVelocity -> Torque
using DivPOmega = decltype(std::declval<Power>() / std::declval<AngularVelocity>());
static_assert(DimensionEqual<typename DivPOmega::DimensionType, TorqueDimension>,
    "Identity 3 violation: Power / AngularVelocity must yield Torque dimension.");
static_assert(std::is_constructible_v<Torque, DivPOmega>,
    "Identity 3 violation: Torque must be constructible from Power / AngularVelocity.");

// Identity 4: Torque / AngularAcceleration -> MomentOfInertia
using DivTauAlpha = decltype(std::declval<Torque>() / std::declval<AngularAcceleration>());
static_assert(DimensionEqual<typename DivTauAlpha::DimensionType, MomentOfInertiaDimension>,
    "Identity 4 violation: Torque / AngularAcceleration must yield MomentOfInertia dimension.");
static_assert(std::is_constructible_v<MomentOfInertia, DivTauAlpha>,
    "Identity 4 violation: MomentOfInertia must be constructible from Torque / AngularAcceleration.");

// ==============================================================================
// 3. 运行期与正向物理代数测试 (Positive Algebraic & Frame Tests)
// ==============================================================================

TEST(DynamicsUnitsTest, DimensionalAlgebraAssertions) {
    // 1. velocity * time -> position displacement
    Velocity3<TestBodyFrame> vel(Velocity(10.0), Velocity(20.0), Velocity(-5.0));
    Second dt(2.0);
    auto disp = vel * dt;
    static_assert(std::is_same_v<decltype(disp)::UnitType::Dimension, LengthDimension>,
        "vel * dt must yield Length dimension.");
    EXPECT_DOUBLE_EQ(disp.x.value(), 20.0);
    EXPECT_DOUBLE_EQ(disp.y.value(), 40.0);
    EXPECT_DOUBLE_EQ(disp.z.value(), -10.0);

    // 2. mass * acceleration -> force
    Kilogram mass(5.0);
    Acceleration3<TestBodyFrame> accel(Acceleration(2.0), Acceleration(-3.0), Acceleration(4.0));
    auto f = mass * accel;
    static_assert(std::is_same_v<decltype(f)::UnitType::Dimension, ForceDimension>,
        "mass * accel must yield Force dimension.");
    EXPECT_DOUBLE_EQ(f.x.value(), 10.0);
    EXPECT_DOUBLE_EQ(f.y.value(), -15.0);
    EXPECT_DOUBLE_EQ(f.z.value(), 20.0);

    // 3. force * length -> energy (distinct from torque)
    Force force_val(25.0);
    Meter arm_val(4.0);
    auto work_val = force_val * arm_val;
    static_assert(DimensionEqual<decltype(work_val)::DimensionType, EnergyDimension>,
        "force * arm must yield Energy dimension.");
    EXPECT_DOUBLE_EQ(work_val.value(), 100.0);

    // 4. rotational torque: I * alpha -> tau
    MomentOfInertia I(10.0);
    AngularAcceleration alpha(5.0);
    Torque tau = I * alpha;
    EXPECT_DOUBLE_EQ(tau.value(), 50.0);
    Torque tau_fn = RotationalTorque(I, alpha);
    EXPECT_DOUBLE_EQ(tau_fn.value(), 50.0);

    // 5. rotational power: tau * omega -> P
    AngularVelocity omega(3.0);
    Power P = tau * omega;
    EXPECT_DOUBLE_EQ(P.value(), 150.0);
    Power P_fn = RotationalPower(tau, omega);
    EXPECT_DOUBLE_EQ(P_fn.value(), 150.0);

    // 6. inversions: P / omega -> tau, tau / alpha -> I
    Torque tau_rec = P / omega;
    EXPECT_DOUBLE_EQ(tau_rec.value(), 50.0);
    MomentOfInertia I_rec = tau / alpha;
    EXPECT_DOUBLE_EQ(I_rec.value(), 10.0);
}

TEST(DynamicsUnitsTest, CombinedFrameAndUnitSafety) {
    // 坐标系变换必须由四元数显式驱动
    auto q = AegisMath::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<TestBodyFrame> v_body(Velocity(5.0), Velocity::Zero(), Velocity::Zero());

    Velocity3<TestRefFrame> v_ref = q * v_body;
    EXPECT_DOUBLE_EQ(v_ref.x.value(), 5.0);

    SUCCEED();
}
