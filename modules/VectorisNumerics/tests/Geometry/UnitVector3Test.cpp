#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
#include "Vectoris/Numerics/Geometry/UnitVector3.h"

namespace {
struct UnitDirectionFrame {};
namespace G = vectoris::numerics::Geometry;
using Error = vectoris::numerics::Core::MathError;

template <typename T>
class UnitVector3ScaleTest : public ::testing::Test {};
using UnitDirectionScalars = ::testing::Types<float, double>;
TYPED_TEST_SUITE(UnitVector3ScaleTest, UnitDirectionScalars);

template <typename T, typename U>
void check_direction(const G::Vector3<U, UnitDirectionFrame>& input) {
    const auto result = G::UnitVector3<T, UnitDirectionFrame>::TryCreate(input);
    ASSERT_TRUE(result.IsSuccess());
    const auto& u = result.Value();
    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(std::isfinite(u.x()));
    EXPECT_TRUE(std::isfinite(u.y()));
    EXPECT_TRUE(std::isfinite(u.z()));
    // Independent exponent rescaling + library hypot. This also works when
    // long double has no more exponent range than double (for example MSVC).
    const long double largest = std::max({std::abs(static_cast<long double>(input.x)),
        std::abs(static_cast<long double>(input.y)), std::abs(static_cast<long double>(input.z))});
    int exponent = 0;
    static_cast<void>(std::frexp(largest, &exponent));
    const long double rx = std::scalbn(static_cast<long double>(input.x), -exponent);
    const long double ry = std::scalbn(static_cast<long double>(input.y), -exponent);
    const long double rz = std::scalbn(static_cast<long double>(input.z), -exponent);
    const long double norm = std::hypot(rx, ry, rz);
    const long double tolerance = static_cast<long double>(std::numeric_limits<T>::epsilon()) * 8;
    EXPECT_NEAR(static_cast<long double>(u.x()), rx / norm, tolerance);
    EXPECT_NEAR(static_cast<long double>(u.y()), ry / norm, tolerance);
    EXPECT_NEAR(static_cast<long double>(u.z()), rz / norm, tolerance);
    EXPECT_NEAR(std::hypot(static_cast<long double>(u.x()),
        static_cast<long double>(u.y()), static_cast<long double>(u.z())), 1.0L, tolerance);
}

TYPED_TEST(UnitVector3ScaleTest, SingleAxisAcrossExponentRange) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    const std::array<T, 7> magnitudes{std::numeric_limits<T>::denorm_min(),
        std::numeric_limits<T>::min(), static_cast<T>(1e-5), T{1}, T{7},
        std::numeric_limits<T>::max()/T{4}, std::numeric_limits<T>::max()};
    for (T magnitude : magnitudes) {
        for (T sign : {T{-1}, T{1}}) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                SCOPED_TRACE(::testing::Message() << magnitude << " sign=" << sign << " axis=" << axis);
                std::array<T, 3> components{};
                components[axis] = sign * magnitude;
                const auto result = UV::TryCreate(G::Vector3<T, UnitDirectionFrame>{
                    components[0], components[1], components[2]});
                ASSERT_TRUE(result.IsSuccess());
                const auto v = result.Value().ToVector();
                const std::array<T, 3> actual{v.x, v.y, v.z};
                for (std::size_t i = 0; i < 3; ++i) {
                    EXPECT_NEAR(actual[i], i == axis ? sign : T{0}, std::numeric_limits<T>::epsilon());
                }
                EXPECT_TRUE(result.Value().IsValid());
            }
        }
    }
}

TYPED_TEST(UnitVector3ScaleTest, MixedDirectionsAgainstIndependentOracle) {
    using T = TypeParam;
    using V = G::Vector3<T, UnitDirectionFrame>;
    const T huge = std::numeric_limits<T>::max()/T{4};
    const T tiny = std::numeric_limits<T>::denorm_min();
    const T normal_min = std::numeric_limits<T>::min();
    const std::array<V, 7> inputs{V{huge, -T{2}*huge, T{3}*huge},
        V{tiny, -T{2}*tiny, T{3}*tiny}, V{normal_min, normal_min, -normal_min},
        V{huge, -normal_min, tiny}, V{-tiny, huge, -T{1}},
        V{T{-3}, T{4}, T{0}}, V{T{-1}, T{-1}, T{-1}}};
    for (const auto& input : inputs) {
        SCOPED_TRACE(::testing::Message() << input.x << ',' << input.y << ',' << input.z);
        check_direction<T>(input);
    }
}

TYPED_TEST(UnitVector3ScaleTest, PositiveScalePreservesDirection) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    using V = G::Vector3<T, UnitDirectionFrame>;
    const std::array<T, 7> scales{std::numeric_limits<T>::denorm_min(),
        std::numeric_limits<T>::min(), static_cast<T>(1e-5), T{1}, T{10},
        std::sqrt(std::numeric_limits<T>::max()), std::numeric_limits<T>::max()/T{4}};
    const auto baseline = UV::TryCreate(V{T{1}, T{-2}, T{3}});
    ASSERT_TRUE(baseline.IsSuccess());
    const T tolerance = std::numeric_limits<T>::epsilon() * T{8};
    for (T scale : scales) {
        SCOPED_TRACE(scale);
        const auto result = UV::TryCreate(V{scale, -T{2}*scale, T{3}*scale});
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(result.Value().IsValid());
        EXPECT_NEAR(result.Value().x(), baseline.Value().x(), tolerance);
        EXPECT_NEAR(result.Value().y(), baseline.Value().y(), tolerance);
        EXPECT_NEAR(result.Value().z(), baseline.Value().z(), tolerance);
        const long double root14 = std::sqrt(14.0L);
        EXPECT_NEAR(static_cast<long double>(result.Value().x()), 1.0L/root14, tolerance);
        EXPECT_NEAR(static_cast<long double>(result.Value().y()), -2.0L/root14, tolerance);
        EXPECT_NEAR(static_cast<long double>(result.Value().z()), 3.0L/root14, tolerance);
    }
}

TYPED_TEST(UnitVector3ScaleTest, ZeroAndNonFiniteErrorClassification) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    using V = G::Vector3<T, UnitDirectionFrame>;
    for (T zero : {T{0}, -T{0}}) {
        const auto result = UV::TryCreate(V{zero, zero, zero});
        ASSERT_FALSE(result.IsSuccess());
        EXPECT_EQ(result.error(), Error::zero_norm);
    }
    for (T bad : {std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity(),
                  -std::numeric_limits<T>::infinity()}) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            std::array<T, 3> components{T{1}, T{-2}, T{3}};
            components[axis] = bad;
            const auto result = UV::TryCreate(V{components[0], components[1], components[2]});
            ASSERT_FALSE(result.IsSuccess());
            EXPECT_EQ(result.error(), Error::non_finite_input);
        }
    }
}

TYPED_TEST(UnitVector3ScaleTest, PublicOperationsPreserveInvariant) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    using V = G::Vector3<T, UnitDirectionFrame>;
    const auto result = UV::TryCreate(V{T{1}, T{-2}, T{3}});
    ASSERT_TRUE(result.IsSuccess());
    auto copy = result.Value();
    auto moved = std::move(copy);
    auto assigned = -moved;
    assigned = moved;
    auto move_assigned = -assigned;
    move_assigned = std::move(assigned);
    for (const auto& u : {result.Value(), moved, move_assigned, -moved}) {
        EXPECT_TRUE(u.IsValid());
        EXPECT_NEAR(u.dot(u), T{1}, std::numeric_limits<T>::epsilon()*T{10});
        EXPECT_NEAR(u.getX(), u.x(), std::numeric_limits<T>::epsilon());
        EXPECT_NEAR(u.getY(), u.y(), std::numeric_limits<T>::epsilon());
        EXPECT_NEAR(u.getZ(), u.z(), std::numeric_limits<T>::epsilon());
    }
    const auto reversed = -moved;
    EXPECT_NEAR(reversed.x(), -moved.x(), std::numeric_limits<T>::epsilon());
    EXPECT_NEAR(reversed.y(), -moved.y(), std::numeric_limits<T>::epsilon());
    EXPECT_NEAR(reversed.z(), -moved.z(), std::numeric_limits<T>::epsilon());
    const auto left_scaled = T{2} * moved;
    const auto right_scaled = moved * T{2};
    EXPECT_TRUE(G::AlmostEqual(left_scaled, right_scaled));
    EXPECT_NEAR(std::hypot(left_scaled.x, left_scaled.y, left_scaled.z), T{2},
                std::numeric_limits<T>::epsilon()*T{16});
    const auto cancelling_sum = moved + reversed;
    const auto difference = moved - reversed;
    EXPECT_NEAR(std::hypot(cancelling_sum.x, cancelling_sum.y, cancelling_sum.z), T{0},
                std::numeric_limits<T>::epsilon());
    EXPECT_TRUE(G::AlmostEqual(difference, left_scaled));
    const auto snapshot = move_assigned;
    EXPECT_FALSE(UV::TryCreate(V{}, move_assigned));
    EXPECT_TRUE(move_assigned == snapshot); // exact unchanged stored state
    EXPECT_FALSE(UV::TryCreate(V{std::numeric_limits<T>::infinity(), T{0}, T{0}}, move_assigned));
    EXPECT_TRUE(move_assigned == snapshot);
    EXPECT_TRUE(UV::TryCreate(V{T{0}, std::numeric_limits<T>::denorm_min(), T{0}}, move_assigned));
    EXPECT_TRUE(move_assigned.IsValid());
    auto detached = move_assigned.ToVector();
    detached.y = T{7};
    EXPECT_NEAR(move_assigned.y(), T{1}, std::numeric_limits<T>::epsilon());
    EXPECT_NEAR(detached.y, T{7}, std::numeric_limits<T>::epsilon());
    static_assert(std::same_as<decltype(moved*T{2}), V>);
    static_assert(std::same_as<decltype(T{2}*moved), V>);
    static_assert(std::same_as<decltype(moved+moved), V>);
    static_assert(std::same_as<decltype(moved-moved), V>);
}

template <typename T> concept HasUnitVector = requires { typename G::UnitVector3<T, UnitDirectionFrame>; };
template <typename T> concept HasUnitFactory = requires(G::Vector3<T, UnitDirectionFrame> v) {
    G::UnitVector3<double, UnitDirectionFrame>::TryCreate(v);
};

// A conforming scalar can bind a const reference and modify a non-const operand.
// UnitVector3 must give it a temporary value rather than a reference to storage.
template <typename T> struct OperandMutatingScalar {
    friend OperandMutatingScalar operator+(OperandMutatingScalar, OperandMutatingScalar) { return {}; }
    friend OperandMutatingScalar operator-(OperandMutatingScalar, OperandMutatingScalar) { return {}; }
    friend OperandMutatingScalar operator*(OperandMutatingScalar, OperandMutatingScalar) { return {}; }
    friend OperandMutatingScalar operator/(OperandMutatingScalar, OperandMutatingScalar) { return {}; }
    friend OperandMutatingScalar operator-(OperandMutatingScalar) { return {}; }
    friend T operator*(const T& value, OperandMutatingScalar) {
        const T saved = value;
        const_cast<T&>(value) = T{0}; // operand is a mutable temporary (or mutable storage in the old code)
        return saved;
    }
    friend T operator*(OperandMutatingScalar scalar, const T& value) { return value * scalar; }
};

TYPED_TEST(UnitVector3ScaleTest, CustomScalarCannotMutateStoredComponents) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    const auto result = UV::TryCreate(G::Vector3<T, UnitDirectionFrame>{T{1}, T{-2}, T{3}});
    ASSERT_TRUE(result.IsSuccess());
    auto u = result.Value(); // mutable object, so the old reference escape is a valid C++ write
    const auto saved = u;
    const OperandMutatingScalar<T> scalar{};
    static_cast<void>(u * scalar);
    EXPECT_TRUE(u == saved);
    static_cast<void>(scalar * u);
    EXPECT_TRUE(u == saved);
    static_cast<void>(u.dot(G::Vector3<OperandMutatingScalar<T>, UnitDirectionFrame>{scalar, scalar, scalar}));
    EXPECT_TRUE(u == saved);
    EXPECT_TRUE(u.IsValid());
}

template <typename U> concept MutableX = requires(U& u) { u.x() = 0; };
template <typename U> concept MutableY = requires(U& u) { u.y() = 0; };
template <typename U> concept MutableZ = requires(U& u) { u.z() = 0; };

TYPED_TEST(UnitVector3ScaleTest, ReadOnlySurfaceAndLayout) {
    using T = TypeParam;
    using UV = G::UnitVector3<T, UnitDirectionFrame>;
    static_assert(HasUnitVector<float> && HasUnitVector<double> && !HasUnitVector<int>);
    static_assert(HasUnitFactory<float> && HasUnitFactory<double> && !HasUnitFactory<int>);
    static_assert(std::is_trivially_move_constructible_v<UV> && std::is_trivially_move_assignable_v<UV>);
    static_assert(!MutableX<UV> && !MutableY<UV> && !MutableZ<UV>);
    static_assert(std::same_as<decltype(std::declval<UV&>().x()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().y()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().z()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().getX()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().getY()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().getZ()), T>);
    static_assert(std::same_as<decltype(std::declval<UV&>().ToVector()), G::Vector3<T, UnitDirectionFrame>>);
    static_assert(!std::is_default_constructible_v<UV> && !std::is_aggregate_v<UV>);
    static_assert(!std::is_constructible_v<UV, T, T, T, G::UnitValidatedTag>);
    static_assert(std::is_standard_layout_v<UV> && std::is_trivially_copyable_v<UV>);
    static_assert(G::GeometryTraits<UV>::Dimension == 3);
    EXPECT_EQ(sizeof(UV), 3*sizeof(T));
    EXPECT_EQ(alignof(UV), alignof(T));
}

TEST(UnitVector3CrossPrecisionTest, FloatInputToDoubleStorage) {
    using V = G::Vector3<float, UnitDirectionFrame>;
    for (float scale : {std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::min(),
                        1.0F, std::numeric_limits<float>::max()/4.0F}) {
        check_direction<double>(V{scale, -2.0F*scale, 3.0F*scale});
    }
}

TEST(UnitVector3CrossPrecisionTest, DoubleInputToFloatStorage) {
    using V = G::Vector3<double, UnitDirectionFrame>;
    for (double scale : {std::numeric_limits<double>::denorm_min(), std::numeric_limits<double>::min(),
                         1.0, std::numeric_limits<double>::max()/4.0}) {
        check_direction<float>(V{scale, -2.0*scale, 3.0*scale});
    }
    const V wide{1.0, std::numeric_limits<double>::denorm_min(), 0.0};
    const auto result = G::UnitVector3<float, UnitDirectionFrame>::TryCreate(wide);
    ASSERT_TRUE(result.IsSuccess());
    EXPECT_TRUE(result.Value().IsValid());
    EXPECT_NEAR(result.Value().x(), 1.0F, std::numeric_limits<float>::epsilon());
    // The unrepresentable normalized component may round to zero in float.
    EXPECT_LE(std::abs(result.Value().y()), std::numeric_limits<float>::denorm_min());
}
} // namespace

// An application-owned Frame may legally appear in a GeometryTraits specialization.
// Such a specialization must not acquire private-storage access.
struct UnitVectorTraitProbeFrame {};
namespace vectoris::numerics::Geometry {
template <> struct GeometryTraits<UnitVector3<double, UnitVectorTraitProbeFrame>> {
    template <typename V> static constexpr bool writable_x = requires(V& v) { v.x_ = 0.0; };
    template <typename V> static constexpr bool writable_y = requires(V& v) { v.y_ = 0.0; };
    template <typename V> static constexpr bool writable_z = requires(V& v) { v.z_ = 0.0; };
};
}
using TraitProbeUnitVector = vectoris::numerics::Geometry::UnitVector3<double, UnitVectorTraitProbeFrame>;
using TraitProbe = vectoris::numerics::Geometry::GeometryTraits<TraitProbeUnitVector>;
static_assert(!TraitProbe::writable_x<TraitProbeUnitVector>);
static_assert(!TraitProbe::writable_y<TraitProbeUnitVector>);
static_assert(!TraitProbe::writable_z<TraitProbeUnitVector>);
