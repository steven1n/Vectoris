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

// Negative Test 3: 力向量不可赋值给力矩向量 (量纲隔离)
static_assert(!std::is_assignable_v<Torque3<TestBodyFrame>&, Force3<TestBodyFrame>>,
    "Compile error expected: Force3 must not be assignable to Torque3.");
static_assert(!std::is_constructible_v<Torque3<TestBodyFrame>, Force3<TestBodyFrame>>,
    "Compile error expected: Torque3 must not be constructible from Force3.");

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

// ==============================================================================
// 2. 运行期与正向物理代数测试 (Positive Algebraic & Frame Tests)
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

    // 3. force * length -> torque
    Force force_val(25.0);
    Meter arm_val(4.0);
    Torque torque_val = force_val * arm_val;
    EXPECT_DOUBLE_EQ(torque_val.value(), 100.0);

    // 4. rotational torque: I * alpha -> tau
    MomentOfInertia I(10.0);
    AngularAcceleration alpha(5.0);
    Torque tau = RotationalTorque(I, alpha);
    EXPECT_DOUBLE_EQ(tau.value(), 50.0);
}

TEST(DynamicsUnitsTest, CombinedFrameAndUnitSafety) {
    // 坐标系变换必须由四元数显式驱动
    auto q = AegisMath::Geometry::Quaternion<double, TestBodyFrame, TestRefFrame>::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    Velocity3<TestBodyFrame> v_body(Velocity(5.0), Velocity::Zero(), Velocity::Zero());

    Velocity3<TestRefFrame> v_ref = q * v_body;
    EXPECT_DOUBLE_EQ(v_ref.x.value(), 5.0);

    SUCCEED();
}
