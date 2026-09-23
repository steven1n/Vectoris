#include <gtest/gtest.h>
#include <limits>
#include <type_traits>
#include "Vectoris/Numerics/Geometry/AlmostEqual.h"
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"
#include "Vectoris/Numerics/Units/BaseUnits/Time.h"

namespace {
namespace G=vectoris::numerics::Geometry;
namespace U=vectoris::numerics::Units;
struct ComparisonFrame {};
struct OtherComparisonFrame {};
template<typename T> class AlmostEqualWrapperTest : public ::testing::Test {};
using WrapperScalars=::testing::Types<float,double>;
TYPED_TEST_SUITE(AlmostEqualWrapperTest,WrapperScalars);
template<class A,class B>
concept GeometryComparable=requires(const A& a,const B& b){G::AlmostEqual(a,b);};
template<class A,class B>
concept ScalarComparable=requires(const A& a,const B& b){vectoris::numerics::Traits::AlmostEqual(a,b,0.0,0.0);};
static_assert(!GeometryComparable<G::Vector3<double,ComparisonFrame>,G::Vector3<double,OtherComparisonFrame>>);
static_assert(!GeometryComparable<G::Point3<double,ComparisonFrame>,G::Point3<double,OtherComparisonFrame>>);
static_assert(!ScalarComparable<U::Quantity<double,U::MeterUnit>,U::Quantity<double,U::SecondUnit>>);
static_assert(!ScalarComparable<U::Quantity<double,U::MeterUnit>,U::Quantity<double,U::MeterUnit>>);

TYPED_TEST(AlmostEqualWrapperTest, ExtremeStoredComponentsDelegateToScalarComparison) {
    using T=TypeParam;
    using V=G::Vector3<T,ComparisonFrame>; using P=G::Point3<T,ComparisonFrame>;
    using M=G::Matrix3<T>; using Q=G::Quaternion<T,ComparisonFrame,ComparisonFrame>;
    const T m=std::numeric_limits<T>::max();
    const V a{m,T{0},T{0}},b{-m,T{0},T{0}};
    EXPECT_FALSE(G::AlmostEqual(a,b,T{0},T{1.5})); EXPECT_TRUE(G::AlmostEqual(a,b,T{0},T{2}));
    EXPECT_FALSE(G::AlmostEqual(P{m,T{0},T{0}},P{-m,T{0},T{0}},T{0},T{1.5}));
    M ma=M::Zero(),mb=M::Zero(); ma.m[8]=m;mb.m[8]=-m;
    EXPECT_FALSE(ma.AlmostEqual(mb,T{0},T{1.5})); EXPECT_FALSE(G::AlmostEqual(ma,mb,T{0},T{1.5}));
    EXPECT_TRUE(G::AlmostEqual(ma,mb,T{0},T{2}));
    // Comparison semantics apply to public stored components; no rotation validity claim.
    Q qa=Q::Identity(),qb=Q::Identity();qa.w=m;qb.w=-m;
    EXPECT_FALSE(G::AlmostEqual(qa,qb,T{0},T{1.5})); EXPECT_TRUE(G::AlmostEqual(qa,qb,T{0},T{2}));
    using Transform=G::Transform3<T,ComparisonFrame,ComparisonFrame>;
    const auto ta=Transform::Create(Q::Identity(),a),tb=Transform::Create(Q::Identity(),b);
    EXPECT_FALSE(G::AlmostEqual(ta,tb,T{0},T{1.5})); EXPECT_TRUE(G::AlmostEqual(ta,tb,T{0},T{2}));
}

TYPED_TEST(AlmostEqualWrapperTest, InvariantTypesAndInvalidTolerances) {
    using T=TypeParam;
    using V=G::Vector3<T,ComparisonFrame>;using UV=G::UnitVector3<T,ComparisonFrame>;
    using Q=G::Quaternion<T,ComparisonFrame,ComparisonFrame>;
    using R=G::RotationMatrix3<T,ComparisonFrame,ComparisonFrame>;
    const auto up=UV::TryCreate(V{T{1},T{0},T{0}}),un=UV::TryCreate(V{T{-1},T{0},T{0}});
    ASSERT_TRUE(up);ASSERT_TRUE(un);
    EXPECT_FALSE(G::AlmostEqual(up.value(),un.value(),T{0},T{1.5}));
    EXPECT_TRUE(G::AlmostEqual(up.value(),un.value(),T{0},T{2}));
    const auto turn=Q::TryCreate(T{0},T{0},T{0},T{1});ASSERT_TRUE(turn);
    const auto rotation=turn.value().ToRotationMatrix();ASSERT_TRUE(rotation);
    EXPECT_FALSE(G::AlmostEqual(R::Identity(),rotation.value(),T{0},T{1.5}));
    EXPECT_TRUE(G::AlmostEqual(R::Identity(),rotation.value(),T{0},T{2}));
    const T nan=std::numeric_limits<T>::quiet_NaN();
    EXPECT_FALSE(G::AlmostEqual(V{},V{},nan,T{0}));
    EXPECT_FALSE(G::AlmostEqual(G::Point3<T,ComparisonFrame>{},G::Point3<T,ComparisonFrame>{},T{0},T{-1}));
    EXPECT_FALSE(G::AlmostEqual(G::Matrix3<T>::Identity(),G::Matrix3<T>::Identity(),nan,T{0}));
    EXPECT_FALSE(G::AlmostEqual(Q::Identity(),Q::Identity(),T{0},nan));
    EXPECT_FALSE(G::AlmostEqual(up.value(),up.value(),nan,T{0}));
    EXPECT_FALSE(G::AlmostEqual(R::Identity(),R::Identity(),T{0},nan));
    using Transform=G::Transform3<T,ComparisonFrame,ComparisonFrame>;
    EXPECT_FALSE(G::AlmostEqual(Transform::Identity(),Transform::Identity(),nan,T{0}));
    const auto negativeTurn=Q::TryCreate(T{0},T{0},T{0},T{-1});ASSERT_TRUE(negativeTurn);
    EXPECT_TRUE(G::RotationEquivalent(turn.value(),negativeTurn.value(),T{0},T{0}));
    EXPECT_FALSE(G::RotationEquivalent(turn.value(),negativeTurn.value(),nan,T{0}));
}
} // namespace
