#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include "Vectoris/Numerics/Core/NumericTraits.h"

namespace {
using vectoris::numerics::Traits::AlmostEqual;
template <typename T> class AlmostEqualExtremeTest : public ::testing::Test {};
using ComparisonScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(AlmostEqualExtremeTest, ComparisonScalars);

TYPED_TEST(AlmostEqualExtremeTest, OrdinaryAbsoluteAndRelativeCriteria) {
    using T = TypeParam;
    EXPECT_TRUE(AlmostEqual(T{1},T{1},T{0},T{0}));
    EXPECT_FALSE(AlmostEqual(T{1},T{2},T{0},T{0}));
    EXPECT_TRUE(AlmostEqual(T{1},T{1.25},T{0.25},T{0}));
    EXPECT_FALSE(AlmostEqual(T{1},T{1.25},T{0.125},T{0}));
    EXPECT_TRUE(AlmostEqual(T{8},T{6},T{0},T{0.25}));
    EXPECT_FALSE(AlmostEqual(T{8},T{6},T{0},T{0.125}));
    EXPECT_TRUE(AlmostEqual(T{-1},T{1},T{2},T{0}));
    EXPECT_FALSE(AlmostEqual(T{-1},T{1},T{1},T{0}));
    EXPECT_TRUE(AlmostEqual(T{-1},T{0},T{0},T{1}));
    EXPECT_FALSE(AlmostEqual(T{-1},T{0},T{0},T{0.5}));
    // Approximate equality is not transitive.
    EXPECT_TRUE(AlmostEqual(T{0},T{1},T{1},T{0}));
    EXPECT_TRUE(AlmostEqual(T{1},T{2},T{1},T{0}));
    EXPECT_FALSE(AlmostEqual(T{0},T{2},T{1},T{0}));
    static_assert(noexcept(AlmostEqual(T{1},T{2},T{0},T{0})));
}

TYPED_TEST(AlmostEqualExtremeTest, OppositeMaximumUsesExactRatioTwo) {
    using T = TypeParam;
    const T m=std::numeric_limits<T>::max();
    for(T r:{T{0},T{0.5},T{1},T{1.5}}) {
        SCOPED_TRACE(r);
        EXPECT_FALSE(AlmostEqual(m,-m,T{0},r));
        EXPECT_FALSE(AlmostEqual(-m,m,T{0},r));
    }
    EXPECT_TRUE(AlmostEqual(m,-m,T{0},T{2}));
    EXPECT_TRUE(AlmostEqual(-m,m,T{0},T{2}));
    EXPECT_TRUE(AlmostEqual(m,-m,T{0},m));
    EXPECT_FALSE(AlmostEqual(m,-m,m,T{0}));
    // A normalized tiny operand must not vanish at the exact rel=1 boundary.
    const T tiny=std::numeric_limits<T>::denorm_min();
    EXPECT_FALSE(AlmostEqual(m,-tiny,T{0},T{1}));
    EXPECT_FALSE(AlmostEqual(-tiny,m,T{0},T{1}));
    EXPECT_TRUE(AlmostEqual(m,-tiny,T{0},std::nextafter(T{1},T{2})));
}

TYPED_TEST(AlmostEqualExtremeTest, MaximumNeighborsHalfAndZero) {
    using T=TypeParam;
    const T m=std::numeric_limits<T>::max(), prev=std::nextafter(m,T{0});
    EXPECT_FALSE(AlmostEqual(m,prev,T{0},T{0}));
    EXPECT_TRUE(AlmostEqual(m,prev,T{0},std::numeric_limits<T>::epsilon()));
    EXPECT_TRUE(AlmostEqual(m,prev,m-prev,T{0}));
    EXPECT_TRUE(AlmostEqual(m,m/T{2},T{0},T{0.5}));
    EXPECT_FALSE(AlmostEqual(m,m/T{2},T{0},std::nextafter(T{0.5},T{0})));
    EXPECT_TRUE(AlmostEqual(m,m/T{2},m/T{2},T{0}));
    EXPECT_FALSE(AlmostEqual(m,T{0},T{0},T{0.5}));
    EXPECT_TRUE(AlmostEqual(m,T{0},T{0},T{1}));
    EXPECT_TRUE(AlmostEqual(m,T{0},m,T{0}));
}

TYPED_TEST(AlmostEqualExtremeTest, SubnormalAndMinimumNormalAbsoluteTolerance) {
    using T=TypeParam;
    static_assert(std::numeric_limits<T>::is_iec559);
    const T d=std::numeric_limits<T>::denorm_min(), n=std::numeric_limits<T>::min();
    const T sub=std::nextafter(n,T{0});
    for(T x:{d,T{2}*d,sub,n}) {
        SCOPED_TRACE(x);
        EXPECT_FALSE(AlmostEqual(T{0},x,T{0},T{0}));
        EXPECT_TRUE(AlmostEqual(T{0},x,x,T{0}));
        EXPECT_FALSE(AlmostEqual(T{0},x,std::nextafter(x,T{0}),T{0}));
        EXPECT_FALSE(AlmostEqual(x,-x,T{0},T{1.5}));
        EXPECT_TRUE(AlmostEqual(x,-x,T{0},T{2}));
        EXPECT_TRUE(AlmostEqual(x,-x,T{2}*x,T{0}));
    }
    EXPECT_TRUE(AlmostEqual(sub,n,d,T{0}));
    EXPECT_FALSE(AlmostEqual(sub,n,T{0},T{0}));
    EXPECT_TRUE(AlmostEqual(d,T{2}*d,d,T{0}));
    EXPECT_TRUE(AlmostEqual(d,-T{2}*d,T{3}*d,T{0}));
    EXPECT_FALSE(AlmostEqual(d,-T{2}*d,T{2}*d,T{0}));
    EXPECT_FALSE(AlmostEqual(T{3}*d,-T{2}*d,T{0},T{1.5}));
}

TYPED_TEST(AlmostEqualExtremeTest, InvalidTolerancesAlwaysFail) {
    using T=TypeParam;
    const T inf=std::numeric_limits<T>::infinity(), nan=std::numeric_limits<T>::quiet_NaN();
    const std::array<T,5> invalid{T{-1},-std::numeric_limits<T>::denorm_min(),nan,inf,-inf};
    for(T bad:invalid) {
        for(T v:{T{0},T{1},inf,-inf,nan}) {
            EXPECT_FALSE(AlmostEqual(v,v,bad,T{0}));
            EXPECT_FALSE(AlmostEqual(v,v,T{0},bad));
            EXPECT_FALSE(AlmostEqual(v,T{2},bad,T{1}));
            EXPECT_FALSE(AlmostEqual(T{2},v,T{1},bad));
        }
    }
    EXPECT_TRUE(AlmostEqual(T{0},-T{0},-T{0},-T{0}));
}

TYPED_TEST(AlmostEqualExtremeTest, SpecialValuesReflexivityAndSymmetry) {
    using T=TypeParam;
    const T inf=std::numeric_limits<T>::infinity(), nan=std::numeric_limits<T>::quiet_NaN();
    const T m=std::numeric_limits<T>::max(), tiny=std::numeric_limits<T>::denorm_min();
    const std::array<T,12> values{-m,T{-1},-tiny,-T{0},T{0},tiny,T{1},m,inf,-inf,nan,std::numeric_limits<T>::min()};
    for(T a:values) {
        EXPECT_EQ(AlmostEqual(a,a,T{0},T{0}),!std::isnan(a));
        for(T b:values) {
            for(T rel:{T{0},T{0.5},T{1},T{1.5},T{2}}) {
                EXPECT_EQ(AlmostEqual(a,b,tiny,rel),AlmostEqual(b,a,tiny,rel));
                if(std::isnan(a)||std::isnan(b)) EXPECT_FALSE(AlmostEqual(a,b,tiny,rel));
            }
        }
    }
    for(T a:{T{0},-T{0}}) for(T b:{T{0},-T{0}}) EXPECT_TRUE(AlmostEqual(a,b,T{0},T{0}));
    EXPECT_TRUE(AlmostEqual(inf,inf,T{0},T{0}));
    EXPECT_TRUE(AlmostEqual(-inf,-inf,T{0},T{0}));
    EXPECT_FALSE(AlmostEqual(inf,-inf,m,m));
    EXPECT_FALSE(AlmostEqual(m,inf,m,m));
    EXPECT_FALSE(AlmostEqual(-inf,-m,m,m));
}

TYPED_TEST(AlmostEqualExtremeTest, DeterministicDyadicPropertiesAgainstIntegerOracle) {
    using T=TypeParam;
    constexpr int first=std::numeric_limits<T>::min_exponent-std::numeric_limits<T>::digits;
    constexpr int last=std::numeric_limits<T>::max_exponent-4;
    // Each operand/tolerance is an exact integer multiple of a power of two.
    // The oracle is integer arithmetic, independent of floating-point comparison.
    for(int exponent=first;exponent<=last;exponent+=73) {
        const T scale=std::scalbn(T{1},exponent);
        for(int ma:{-8,-3,-1,0,1,3,8}) for(int mb:{-8,-3,-1,0,1,3,8}) {
            for(int abs_units:{0,1,4}) for(int rel_eighths:{0,4,8,12,16}) {
                const bool expected=8*std::abs(ma-mb)<=std::max(8*abs_units,rel_eighths*std::max(std::abs(ma),std::abs(mb)));
                const T a=static_cast<T>(ma)*scale, b=static_cast<T>(mb)*scale;
                const T atol=static_cast<T>(abs_units)*scale, rtol=static_cast<T>(rel_eighths)/T{8};
                const bool actual=AlmostEqual(a,b,atol,rtol);
                EXPECT_EQ(actual,expected) << exponent << ':' << ma << ',' << mb << ':' << abs_units << ',' << rel_eighths;
                EXPECT_EQ(actual,AlmostEqual(b,a,atol,rtol));
                EXPECT_EQ(actual,AlmostEqual(static_cast<T>(ma),static_cast<T>(mb),static_cast<T>(abs_units),rtol));
                EXPECT_TRUE(AlmostEqual(a,a,T{0},T{0}));
            }
        }
    }
}

TYPED_TEST(AlmostEqualExtremeTest, SelectedLongDoubleOracleWhenWider) {
    using T=TypeParam;
    if constexpr(std::numeric_limits<long double>::max_exponent>std::numeric_limits<T>::max_exponent &&
                 std::numeric_limits<long double>::digits>std::numeric_limits<T>::digits) {
        const T m=std::numeric_limits<T>::max(), n=std::numeric_limits<T>::min(), d=std::numeric_limits<T>::denorm_min();
        const std::array<std::array<T,2>,7> pairs{{{m,-m},{m,std::nextafter(m,T{0})},{m,m/T{2}},
            {T{1},T{1}+std::numeric_limits<T>::epsilon()},{n,-n},{d,-T{2}*d},{T{3}*d,-T{2}*d}}};
        for(const auto& pair:pairs) for(T atol:{T{0},d,n}) {
            for(T rtol:{T{0},std::numeric_limits<T>::epsilon()/T{2},T{0.5},T{1},T{1.5},T{2}}) {
                const long double a=pair[0],b=pair[1];
                const bool expected=std::abs(a-b)<=std::max(static_cast<long double>(atol),static_cast<long double>(rtol)*std::max(std::abs(a),std::abs(b)));
                EXPECT_EQ(AlmostEqual(pair[0],pair[1],atol,rtol),expected);
            }
        }
    }
}
} // namespace
