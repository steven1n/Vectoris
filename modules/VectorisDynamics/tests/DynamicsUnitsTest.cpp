#include <gtest/gtest.h>
#include <type_traits>
#include "Vectoris/Dynamics/QuantityVector3.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/EulerIntegrator.h"

struct TestRefFrame {};
struct TestBodyFrame {};

using namespace vectoris::dynamics;
using namespace vectoris::numerics::Units;

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

// Negative Test 10: 普通向量叉乘 Cross(omega, L) 生成 EnergyDimension [M L^2 T^-2 A^0]，而非力矩 [M L^2 T^-2 A^-1]
using RawCrossType = decltype(Cross(std::declval<AngularVelocity3<TestBodyFrame>>(), std::declval<AngularMomentum3<TestBodyFrame>>()));
static_assert(DimensionEqual<typename RawCrossType::UnitType::Dimension, EnergyDimension>,
    "Ordinary cross(omega, L) must yield EnergyDimension [M L^2 T^-2 A^0]");
static_assert(!DimensionEqual<typename RawCrossType::UnitType::Dimension, TorqueDimension>,
    "Ordinary cross(omega, L) must NOT yield TorqueDimension [M L^2 T^-2 A^-1]");
static_assert(!std::is_constructible_v<Torque3<TestBodyFrame>, RawCrossType>,
    "Compile error expected: Torque3 must NOT be constructible from ordinary cross(omega, L).");
static_assert(!std::is_assignable_v<Torque3<TestBodyFrame>&, RawCrossType>,
    "Compile error expected: Torque3 must NOT be assignable from ordinary cross(omega, L).");

template <typename T3, typename RC>
concept CanSubtractRawCrossFromTorque = requires(T3 t, RC r) {
    { t - r };
};
static_assert(!CanSubtractRawCrossFromTorque<Torque3<TestBodyFrame>, RawCrossType>,
    "Compile error expected: Torque3 cannot subtract ordinary cross(omega, L) without Radian normalization.");

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

// Identity 5: MomentOfInertia * AngularVelocity -> AngularMomentum
using ProdIOmega = decltype(std::declval<MomentOfInertia>() * std::declval<AngularVelocity>());
static_assert(DimensionEqual<typename ProdIOmega::DimensionType, AngularMomentumDimension>,
    "Identity 5 violation: MomentOfInertia * AngularVelocity must yield AngularMomentum dimension.");
static_assert(std::is_constructible_v<AngularMomentum, ProdIOmega>,
    "Identity 5 violation: AngularMomentum must be constructible from MomentOfInertia * AngularVelocity.");

// Identity 6: RotationalCross(AngularVelocity3, AngularMomentum3) -> Torque3
using RotCrossResult = decltype(RotationalCross(std::declval<AngularVelocity3<TestBodyFrame>>(), std::declval<AngularMomentum3<TestBodyFrame>>()));
static_assert(std::is_same_v<RotCrossResult, Torque3<TestBodyFrame>>,
    "Identity 6 violation: RotationalCross must yield Torque3.");

// Identity 7: Torque3 - RotationalCross(omega, I*omega) -> Torque3
using NetTorqueResult = decltype(std::declval<Torque3<TestBodyFrame>>() - std::declval<RotCrossResult>());
static_assert(std::is_same_v<NetTorqueResult, Torque3<TestBodyFrame>>,
    "Identity 7 violation: Net torque subtraction must yield Torque3.");

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

    // 7. angular momentum: I * omega -> L
    AngularMomentum L = I * omega;
    EXPECT_DOUBLE_EQ(L.value(), 30.0);

    // 8. rotational Lie bracket: RotationalCross(omega, L) -> Torque
    AngularVelocity3<TestBodyFrame> w_vec(AngularVelocity(1.0), AngularVelocity::Zero(), AngularVelocity::Zero());
    AngularMomentum3<TestBodyFrame> L_vec(AngularMomentum::Zero(), AngularMomentum(4.0), AngularMomentum::Zero());
    Torque3<TestBodyFrame> tau_gyro = RotationalCross(w_vec, L_vec);
    EXPECT_DOUBLE_EQ(tau_gyro.x.value(), 0.0);
    EXPECT_DOUBLE_EQ(tau_gyro.y.value(), 0.0);
    EXPECT_DOUBLE_EQ(tau_gyro.z.value(), 4.0);

    // 9. net torque subtraction: Torque - RotationalCross -> Torque
    Torque3<TestBodyFrame> tau_ext(Torque::Zero(), Torque::Zero(), Torque(10.0));
    Torque3<TestBodyFrame> tau_net = tau_ext - tau_gyro;
    EXPECT_DOUBLE_EQ(tau_net.z.value(), 6.0);
}

TEST(DynamicsUnitsTest, CombinedFrameAndUnitSafety) {
    // 坐标系变换必须由四元数显式驱动
    auto q = vectoris::numerics::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<TestBodyFrame> v_body(Velocity(5.0), Velocity::Zero(), Velocity::Zero());

    Velocity3<TestRefFrame> v_ref = q * v_body;
    EXPECT_DOUBLE_EQ(v_ref.x.value(), 5.0);

    EXPECT_FALSE((vectoris::numerics::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(0.0, 0.0, 0.0, 0.0).IsSuccess()));
    EXPECT_FALSE((vectoris::numerics::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0).IsSuccess()));
    auto q_neg = vectoris::numerics::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(-1.0, 0.0, 0.0, 0.0);
    EXPECT_TRUE(q_neg.IsSuccess());

    SUCCEED();
}
