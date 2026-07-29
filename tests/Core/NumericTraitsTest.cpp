#include <gtest/gtest.h>
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Core/Precision.h"

using namespace AegisMath;
using namespace AegisMath::Traits;

TEST(NumericTraitsTest, BasicTraits) {
    EXPECT_GT(NumericTraits<Real>::max(), static_cast<Real>(0.0));
    EXPECT_LT(NumericTraits<Real>::lowest(), static_cast<Real>(0.0));

    EXPECT_TRUE(IsNaN(NumericTraits<Real>::quietNaN()));
    EXPECT_TRUE(IsInfinity(NumericTraits<Real>::infinity()));
    EXPECT_TRUE(IsFinite(static_cast<Real>(1.0)));
    EXPECT_FALSE(IsInfinity(static_cast<Real>(1.0)));
}

TEST(NumericTraitsTest, StrictAlmostEqual) {
    const Real a = 1.0;
    const Real b = 1.0 + NumericTraits<Real>::epsilon() * 0.5;

    const Real absTol = 1e-6;
    const Real relTol = 1e-5;

    EXPECT_TRUE(AlmostEqual(a, b, absTol, relTol));

    const Real largeA = 10000000.0;
    const Real largeB = 10000000.1;
    EXPECT_TRUE(AlmostEqual(largeA, largeB, absTol, relTol));

    const Real nearZeroA = 1e-8;
    const Real nearZeroB = 1e-9;
    EXPECT_TRUE(AlmostEqual(nearZeroA, nearZeroB, absTol, relTol));

    EXPECT_FALSE(AlmostEqual(static_cast<Real>(1.0), static_cast<Real>(1.1), absTol, relTol));
    EXPECT_FALSE(AlmostEqual(NumericTraits<Real>::quietNaN(), 1.0, absTol, relTol));
}