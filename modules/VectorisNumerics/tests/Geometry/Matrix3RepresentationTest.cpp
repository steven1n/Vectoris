#include <cstddef>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "Vectoris/Numerics/Geometry/Matrix3.h"

namespace {
namespace geometry = vectoris::numerics::geometry;

using FloatMatrix = geometry::Matrix3<float>;
using DoubleMatrix = geometry::Matrix3<double>;

static_assert(std::is_standard_layout_v<FloatMatrix>);
static_assert(std::is_trivially_copyable_v<FloatMatrix>);
static_assert(std::is_standard_layout_v<DoubleMatrix>);
static_assert(std::is_trivially_copyable_v<DoubleMatrix>);
static_assert(sizeof(FloatMatrix) == sizeof(float) * 9U);
static_assert(sizeof(DoubleMatrix) == sizeof(double) * 9U);
static_assert(offsetof(FloatMatrix, m) == 0U);
static_assert(offsetof(DoubleMatrix, m) == 0U);
static_assert(geometry::Matrix3ABIContract<float>::value);
static_assert(geometry::Matrix3ABIContract<double>::value);
static_assert(std::is_same_v<FloatMatrix, vectoris::numerics::Geometry::Matrix3<float>>);
static_assert(std::is_same_v<DoubleMatrix, vectoris::numerics::Geometry::Matrix3<double>>);
static_assert(std::is_same_v<decltype(std::declval<FloatMatrix&>()(0, 0)), float&>);
static_assert(std::is_same_v<decltype(std::declval<const FloatMatrix&>()(0, 0)), const float&>);
static_assert(std::is_same_v<decltype(std::declval<DoubleMatrix&>()(0, 0)), double&>);
static_assert(std::is_same_v<decltype(std::declval<const DoubleMatrix&>()(0, 0)), const double&>);

template <typename T>
void VerifyRepresentationAndAccess() {
    using Matrix = geometry::Matrix3<T>;
    Matrix matrix{T{1}, T{2}, T{3}, T{4}, T{5}, T{6}, T{7}, T{8}, T{9}};

    EXPECT_EQ(matrix(0, 0), T{1});
    EXPECT_EQ(matrix(0, 2), T{3});
    EXPECT_EQ(matrix(2, 0), T{7});
    EXPECT_EQ(matrix(2, 2), T{9});
    EXPECT_EQ(&matrix.m[0] + 1, &matrix.m[1]);
    EXPECT_EQ(&matrix.m[0] + 8, &matrix.m[8]);

    matrix(1, 2) = T{42};
    EXPECT_EQ(matrix.m[5], T{42});
    matrix.m[7] = T{-3};
    EXPECT_EQ(matrix(2, 1), T{-3});

    const Matrix& read_only = matrix;
    EXPECT_EQ(read_only(1, 2), T{42});
    EXPECT_EQ(read_only(2, 1), T{-3});
    EXPECT_EQ(sizeof(Matrix), sizeof(T) * 9U);
    EXPECT_GE(alignof(Matrix), alignof(T));
}
} // namespace

TEST(Matrix3RepresentationTest, RowMajorMutableAndConstElementAccess) {
    VerifyRepresentationAndAccess<float>();
    VerifyRepresentationAndAccess<double>();
}

TEST(Matrix3RepresentationTest, RawStorageAllowsGeneralMatrixValues) {
    auto matrix = DoubleMatrix::Identity();
    matrix.m[0] = 42.0;
    EXPECT_DOUBLE_EQ(matrix(0, 0), 42.0);

    matrix.m[0] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_TRUE(std::isnan(matrix(0, 0)));
}
