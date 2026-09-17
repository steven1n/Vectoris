#include <gtest/gtest.h>
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Numerics/Core/Precision.h"

using namespace vectoris::numerics;
using namespace vectoris::numerics::Traits;

TEST(NumericTraitsTest, BasicTraits) {
    EXPECT_GT(NumericTraits<Real>::max(), static_cast<Real>(0.0));
    EXPECT_LT(NumericTraits<Real>::lowest(), static_cast<Real>(0.0));

    EXPECT_TRUE(IsNaN(NumericTraits<Real>::quietNaN()));
    EXPECT_TRUE(IsInfinity(NumericTraits<Real>::infinity()));
    EXPECT_TRUE(IsInfinity(-NumericTraits<Real>::infinity()));
    EXPECT_TRUE(IsFinite(static_cast<Real>(1.0)));
    EXPECT_FALSE(IsFinite(NumericTraits<Real>::quietNaN()));
    EXPECT_FALSE(IsFinite(NumericTraits<Real>::infinity()));
    EXPECT_FALSE(IsFinite(-NumericTraits<Real>::infinity()));
    EXPECT_FALSE(IsInfinity(static_cast<Real>(1.0)));

    EXPECT_TRUE(IsZero(static_cast<Real>(0.0)));
    EXPECT_FALSE(IsZero(static_cast<Real>(1.0)));

    // Float specialization
    EXPECT_GT(NumericTraits<float>::max(), 0.0f);
    EXPECT_LT(NumericTraits<float>::lowest(), 0.0f);
    EXPECT_TRUE(IsNaN(NumericTraits<float>::quietNaN()));
    EXPECT_TRUE(IsInfinity(NumericTraits<float>::infinity()));
    EXPECT_TRUE(IsInfinity(-NumericTraits<float>::infinity()));
    EXPECT_TRUE(IsFinite(1.0f));
    EXPECT_FALSE(IsFinite(NumericTraits<float>::quietNaN()));
    EXPECT_FALSE(IsFinite(NumericTraits<float>::infinity()));
    EXPECT_FALSE(IsFinite(-NumericTraits<float>::infinity()));
    EXPECT_FALSE(IsInfinity(1.0f));
    EXPECT_TRUE(IsZero(0.0f));
    EXPECT_FALSE(IsZero(1.0f));
}

TEST(NumericTraitsTest, StrictAlmostEqual) {
    const Real a = 1.0;
    const Real b = 1.0 + NumericTraits<Real>::epsilon() * 0.5;

    const Real absTol = 1e-6;
    const Real relTol = 1e-5;

    EXPECT_TRUE(AlmostEqual(a, b, absTol, relTol));

    // Exact equality fast-path (a == b)
    EXPECT_TRUE(AlmostEqual(a, a, absTol, relTol));

    const Real largeA = 10000000.0;
    const Real largeB = 10000000.1;
    EXPECT_TRUE(AlmostEqual(largeA, largeB, absTol, relTol));

    const Real nearZeroA = 1e-8;
    const Real nearZeroB = 1e-9;
    EXPECT_TRUE(AlmostEqual(nearZeroA, nearZeroB, absTol, relTol));

    EXPECT_FALSE(AlmostEqual(static_cast<Real>(1.0), static_cast<Real>(1.1), absTol, relTol));
    EXPECT_FALSE(AlmostEqual(static_cast<Real>(1.0), static_cast<Real>(-1.0), absTol, relTol));

    // NaN on either side
    EXPECT_FALSE(AlmostEqual(NumericTraits<Real>::quietNaN(), 1.0, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(1.0, NumericTraits<Real>::quietNaN(), absTol, relTol));
    EXPECT_FALSE(AlmostEqual(NumericTraits<Real>::quietNaN(), NumericTraits<Real>::quietNaN(), absTol, relTol));
}

TEST(NumericTraitsTest, FloatStrictAlmostEqual) {
    const float a = 1.0f;
    const float b = 1.0f + NumericTraits<float>::epsilon() * 0.5f;
    EXPECT_TRUE(AlmostEqual(a, b, 1e-4f, 1e-4f));
    EXPECT_TRUE(AlmostEqual(a, a, 1e-4f, 1e-4f));
    EXPECT_FALSE(AlmostEqual(1.0f, 1.1f, 1e-4f, 1e-4f));
    EXPECT_FALSE(AlmostEqual(1.0f, -1.0f, 1e-4f, 1e-4f));
    EXPECT_FALSE(AlmostEqual(NumericTraits<float>::quietNaN(), 1.0f, 1e-4f, 1e-4f));
    EXPECT_FALSE(AlmostEqual(1.0f, NumericTraits<float>::quietNaN(), 1e-4f, 1e-4f));
    EXPECT_FALSE(AlmostEqual(NumericTraits<float>::quietNaN(), NumericTraits<float>::quietNaN(), 1e-4f, 1e-4f));
}

TEST(NumericTraitsTest, IEEE754InfinityAndSignedZeroSemanticsDouble) {
    const Real posInf = NumericTraits<Real>::infinity();
    const Real negInf = -NumericTraits<Real>::infinity();
    const Real nanVal = NumericTraits<Real>::quietNaN();
    const Real finiteVal = 1000.0;
    const Real zero = 0.0;
    const Real negZero = -0.0;
    const Real absTol = 1e-6;
    const Real relTol = 1e-5;

    // +Inf vs +Inf -> true
    EXPECT_TRUE(AlmostEqual(posInf, posInf, absTol, relTol));
    // -Inf vs -Inf -> true
    EXPECT_TRUE(AlmostEqual(negInf, negInf, absTol, relTol));

    // +Inf vs -Inf -> false
    EXPECT_FALSE(AlmostEqual(posInf, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, posInf, absTol, relTol));

    // +Inf vs finite -> false
    EXPECT_FALSE(AlmostEqual(posInf, finiteVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(finiteVal, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(posInf, zero, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(zero, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(posInf, 1e300, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(1e300, posInf, absTol, relTol));

    // -Inf vs finite -> false
    EXPECT_FALSE(AlmostEqual(negInf, finiteVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(finiteVal, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, zero, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(zero, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, -1e300, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(-1e300, negInf, absTol, relTol));

    // Infinity vs NaN -> false
    EXPECT_FALSE(AlmostEqual(posInf, nanVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(nanVal, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, nanVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(nanVal, negInf, absTol, relTol));

    // Signed zeros: +0.0 == -0.0 under IEEE-754 -> true
    EXPECT_TRUE(AlmostEqual(zero, negZero, absTol, relTol));
    EXPECT_TRUE(AlmostEqual(negZero, zero, absTol, relTol));
}

TEST(NumericTraitsTest, IEEE754InfinityAndSignedZeroSemanticsFloat) {
    const float posInf = NumericTraits<float>::infinity();
    const float negInf = -NumericTraits<float>::infinity();
    const float nanVal = NumericTraits<float>::quietNaN();
    const float finiteVal = 1000.0f;
    const float zero = 0.0f;
    const float negZero = -0.0f;
    const float absTol = 1e-4f;
    const float relTol = 1e-4f;

    // +Inf vs +Inf -> true
    EXPECT_TRUE(AlmostEqual(posInf, posInf, absTol, relTol));
    // -Inf vs -Inf -> true
    EXPECT_TRUE(AlmostEqual(negInf, negInf, absTol, relTol));

    // +Inf vs -Inf -> false
    EXPECT_FALSE(AlmostEqual(posInf, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, posInf, absTol, relTol));

    // +Inf vs finite -> false
    EXPECT_FALSE(AlmostEqual(posInf, finiteVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(finiteVal, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(posInf, zero, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(zero, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(posInf, 1e38f, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(1e38f, posInf, absTol, relTol));

    // -Inf vs finite -> false
    EXPECT_FALSE(AlmostEqual(negInf, finiteVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(finiteVal, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, zero, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(zero, negInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, -1e38f, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(-1e38f, negInf, absTol, relTol));

    // Infinity vs NaN -> false
    EXPECT_FALSE(AlmostEqual(posInf, nanVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(nanVal, posInf, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(negInf, nanVal, absTol, relTol));
    EXPECT_FALSE(AlmostEqual(nanVal, negInf, absTol, relTol));

    // Signed zeros: +0.0 == -0.0 under IEEE-754 -> true
    EXPECT_TRUE(AlmostEqual(zero, negZero, absTol, relTol));
    EXPECT_TRUE(AlmostEqual(negZero, zero, absTol, relTol));
}