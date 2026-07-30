#include <gtest/gtest.h>
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Core/Constants.h"
#include "AegisMath/Units/Common.h"
#include "AegisMath/Units/BaseUnits/Length.h"
#include "AegisMath/Units/BaseUnits/Time.h"
#include "AegisMath/Units/BaseUnits/Length.h"  // 确保包含具体的单位定义文件

using namespace AegisMath;
using namespace AegisMath::Units;

TEST(UnitsSystemRevisionB2Test, CRTPABIAndZeroInit) {
    static_assert(sizeof(Meter) == sizeof(double));
    static_assert(alignof(Meter) == alignof(double));
    static_assert(std::is_trivially_copyable_v<Meter>);

    Meter zero_m = Meter::Zero();
    EXPECT_EQ(zero_m.value(), 0.0);
}

TEST(UnitsSystemRevisionB2Test, StrictTypeConceptAndCast) {
    // 验证 IsQuantity 的强类型过滤
    static_assert(IsQuantity<Meter>);
    
    struct FakeQuantity {}; // 简化为空结构体，消除 unused-local-typedef 警告
    static_assert(!IsQuantity<FakeQuantity>);

    Meter m{100.0};
    EXPECT_EQ(m.value(), 100.0);

    Meter distance{200.0};
    Second time_val{10.0};
    auto velocity = distance / time_val;
    EXPECT_TRUE(Traits::AlmostEqual(velocity.value(), static_cast<Scalar>(20.0), 1e-6, 1e-6));
}