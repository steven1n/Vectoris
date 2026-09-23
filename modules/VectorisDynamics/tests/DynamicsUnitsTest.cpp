#include <gtest/gtest.h>
#include <type_traits>
#include "Vectoris/Dynamics/QuantityVector3.h"
#include "Vectoris/Dynamics/RigidBodyParameters.h"
#include "Vectoris/Dynamics/Detail/StateTypes.h"
#include "Vectoris/Dynamics/Twist6.h"
#include "Vectoris/Dynamics/Wrench6.h"
#include "Vectoris/Dynamics/EulerIntegrator.h"
#include "Vectoris/Numerics/Units/DerivedUnits/Frequency.h"

struct TestRefFrame {};
struct TestBodyFrame {};
struct TestOtherFrame {};

using namespace vectoris::dynamics;
using namespace vectoris::numerics::Units;

template <typename A, typename B>
concept CanRotationalCross = requires(const A& a, const B& b) {
    RotationalCross(a, b);
};

template <typename A, typename B>
concept CanGenericCross = requires(const A& a, const B& b) {
    Cross(a, b);
};

// VRT-16 compile-time contract: ordinary vector algebra retains Angle, while
// only the named physical rotational operation normalizes by one radian.
using TestOmega = AngularVelocity3<TestBodyFrame>;
using TestAngularAcceleration = AngularAcceleration3<TestBodyFrame>;
using TestVelocity = Velocity3<TestBodyFrame>;
using TestPosition = Position3<TestBodyFrame>;
using GenericOmegaVelocityCross = decltype(Cross(std::declval<TestOmega>(), std::declval<TestVelocity>()));
using GenericOmegaPositionCross = decltype(Cross(std::declval<TestOmega>(), std::declval<TestPosition>()));
using GenericAlphaPositionCross = decltype(Cross(std::declval<TestAngularAcceleration>(), std::declval<TestPosition>()));
using PhysicalOmegaVelocityCross = decltype(RotationalCross(std::declval<TestOmega>(), std::declval<TestVelocity>()));

static_assert(std::is_same_v<Radian::DimensionType, AngleDimension>);
static_assert(!std::is_same_v<AngularVelocity::DimensionType, Frequency::DimensionType>);
static_assert(std::is_same_v<AngularVelocity::DimensionType, AngularVelocityDimension>);
static_assert(std::is_same_v<AngularAcceleration::DimensionType, AngularAccelerationDimension>);
static_assert(std::is_same_v<MomentOfInertia::DimensionType, MomentOfInertiaDimension>);
static_assert(std::is_same_v<Torque::DimensionType, TorqueDimension>);
static_assert(std::is_same_v<AngularMomentum::DimensionType, AngularMomentumDimension>);
static_assert(DimensionEqual<typename GenericOmegaPositionCross::UnitType::Dimension,
                             DimensionAdd_t<AngularVelocityDimension, LengthDimension>>);
static_assert(DimensionEqual<typename GenericOmegaVelocityCross::UnitType::Dimension,
                             Dimension<1, 0, -2, 0, 0, 0, 0, 1>>);
static_assert(!std::is_same_v<GenericOmegaVelocityCross, Acceleration3<TestBodyFrame>>);
static_assert(!std::is_constructible_v<Acceleration3<TestBodyFrame>, GenericOmegaVelocityCross>);
static_assert(DimensionEqual<typename GenericAlphaPositionCross::UnitType::Dimension,
                             DimensionAdd_t<AngularAccelerationDimension, LengthDimension>>);
static_assert(std::is_same_v<PhysicalOmegaVelocityCross, Acceleration3<TestBodyFrame>>);
static_assert(std::is_same_v<typename PhysicalOmegaVelocityCross::FrameType, TestBodyFrame>);

// VRT-16 negative API probes: a physical rotational cross cannot mix Frames,
// accept unrelated dimensions or untyped scalars. Position-based physical
// cross identities are intentionally not exposed until a production equation needs them.
static_assert(!CanGenericCross<TestOmega, Velocity3<TestOtherFrame>>);
static_assert(!CanRotationalCross<TestOmega, Velocity3<TestOtherFrame>>);
static_assert(!CanRotationalCross<TestVelocity, TestVelocity>);
static_assert(!CanRotationalCross<double, TestVelocity>);
static_assert(!CanRotationalCross<TestOmega, TestPosition>);
static_assert(!CanRotationalCross<TestAngularAcceleration, TestPosition>);

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

TEST(DynamicsUnitsTest, RotationalCrossPhysicalSemantics) {
    TestOmega omega(AngularVelocity::Zero(), AngularVelocity::Zero(), AngularVelocity(2.0));
    TestVelocity velocity(Velocity(3.0), Velocity::Zero(), Velocity::Zero());

    const auto generic = Cross(omega, velocity);
    const auto physical = RotationalCross(omega, velocity);
    EXPECT_NEAR(generic.y.value(), 6.0, 1.0e-14);
    EXPECT_NEAR(physical.x.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(physical.y.value(), 6.0, 1.0e-14);
    EXPECT_NEAR(physical.z.value(), 0.0, 1.0e-14);
}

TEST(DynamicsUnitsTest, RotationalCrossZeroParallelOrthogonalAndSigns) {
    const TestOmega omega(AngularVelocity(1.0), AngularVelocity(-2.0), AngularVelocity(3.0));
    const TestVelocity velocity(Velocity(-4.0), Velocity(5.0), Velocity(-6.0));
    const auto mixed = RotationalCross(omega, velocity);
    EXPECT_NEAR(mixed.x.value(), -3.0, 1.0e-14);
    EXPECT_NEAR(mixed.y.value(), -6.0, 1.0e-14);
    EXPECT_NEAR(mixed.z.value(), -3.0, 1.0e-14);

    const TestOmega zero_omega;
    const TestVelocity nonzero_velocity(Velocity(4.0), Velocity(-5.0), Velocity(6.0));
    const auto zero_rate = RotationalCross(zero_omega, nonzero_velocity);
    EXPECT_NEAR(zero_rate.x.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(zero_rate.y.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(zero_rate.z.value(), 0.0, 1.0e-14);

    const TestVelocity zero_velocity;
    const auto zero_vector = RotationalCross(omega, zero_velocity);
    EXPECT_NEAR(zero_vector.x.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(zero_vector.y.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(zero_vector.z.value(), 0.0, 1.0e-14);

    const TestOmega x_axis(AngularVelocity(2.0), AngularVelocity::Zero(), AngularVelocity::Zero());
    const TestOmega negative_z_axis(AngularVelocity::Zero(), AngularVelocity::Zero(), AngularVelocity(-2.0));
    const TestVelocity parallel(Velocity(3.0), Velocity::Zero(), Velocity::Zero());
    const TestVelocity anti_parallel(Velocity(-3.0), Velocity::Zero(), Velocity::Zero());
    const auto parallel_result = RotationalCross(x_axis, parallel);
    const auto antiparallel_result = RotationalCross(x_axis, anti_parallel);
    const auto opposite_sign_result = RotationalCross(negative_z_axis, parallel);
    EXPECT_NEAR(parallel_result.x.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(parallel_result.y.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(parallel_result.z.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(antiparallel_result.x.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(antiparallel_result.y.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(antiparallel_result.z.value(), 0.0, 1.0e-14);
    EXPECT_NEAR(opposite_sign_result.y.value(), -6.0, 1.0e-14);
}

template <typename T>
void ExpectRotationalCrossFiniteScaleProduct(T large, T small, T tolerance) {
    using OmegaT = AngularVelocity3<TestBodyFrame, T>;
    using VelocityT = Velocity3<TestBodyFrame, T>;
    using OmegaQ = Quantity<T, RadianPerSecondUnit>;
    using VelocityQ = Quantity<T, MeterPerSecondUnit>;
    const OmegaT omega(OmegaQ(static_cast<T>(0)), OmegaQ(static_cast<T>(0)), OmegaQ(large));
    const VelocityT velocity(VelocityQ(small), VelocityQ(static_cast<T>(0)), VelocityQ(static_cast<T>(0)));
    const auto acceleration = RotationalCross(omega, velocity);
    EXPECT_NEAR(acceleration.y.value(), static_cast<T>(1), tolerance);
}

TEST(DynamicsUnitsTest, RotationalCrossFiniteScaleCasesFloat) {
    ExpectRotationalCrossFiniteScaleProduct<float>(1.0e18F, 1.0e-18F, 2.0e-6F);
    ExpectRotationalCrossFiniteScaleProduct<float>(1.0e-18F, 1.0e18F, 2.0e-6F);
}

TEST(DynamicsUnitsTest, RotationalCrossFiniteScaleCasesDouble) {
    ExpectRotationalCrossFiniteScaleProduct<double>(1.0e150, 1.0e-150, 2.0e-14);
    ExpectRotationalCrossFiniteScaleProduct<double>(1.0e-150, 1.0e150, 2.0e-14);
}

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
