#include <gtest/gtest.h>
#include "AegisMath/Dynamics/InertiaTensor3.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"

struct BodyFrame {};

TEST(InertiaTensorTest, SymmetryAndPositiveDefinite) {
    using MI = AegisMath::Units::MomentOfInertia;

    // 合法惯量张量
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> valid_tensor(
        MI(10.0), MI(0.1), MI(0.2),
        MI(0.1),  MI(20.0), MI(0.3),
        MI(0.2),  MI(0.3),  MI(30.0)
    );
    EXPECT_TRUE(valid_tensor.IsValid());

    // 非法惯量张量（对角线负数）
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> invalid_tensor(
        MI(-1.0), MI(0.0), MI(0.0),
        MI(0.0),  MI(20.0), MI(0.0),
        MI(0.0),  MI(0.0),  MI(30.0)
    );
    EXPECT_FALSE(invalid_tensor.IsValid());
}