#include <gtest/gtest.h>
#include "AegisMath/Dynamics/InertiaTensor3.h"

struct BodyFrame {};

TEST(InertiaTensorTest, SymmetryAndPositiveDefinite) {
    // 合法惯量张量
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> valid_tensor(
        10.0, 0.1, 0.2,
        0.1,  20.0, 0.3,
        0.2,  0.3,  30.0
    );
    EXPECT_TRUE(valid_tensor.IsValid());

    // 非法惯量张量（对角线负数）
    AegisMath::Dynamics::InertiaTensor3<double, BodyFrame> invalid_tensor(
        -1.0, 0.0, 0.0,
        0.0,  20.0, 0.0,
        0.0,  0.0,  30.0
    );
    EXPECT_FALSE(invalid_tensor.IsValid());
}