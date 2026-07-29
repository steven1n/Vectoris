import os

files_content = {
    "include/AegisMath/Units/Dimension.h": """#pragma once
#include <type_traits>

namespace AegisMath::Units {

    template<
        int Length,
        int Mass,
        int Time,
        int Current,
        int Temperature,
        int Amount,
        int Luminosity,
        int Angle
    >
    struct Dimension {
        static constexpr int length      = Length;
        static constexpr int mass        = Mass;
        static constexpr int time        = Time;
        static constexpr int current     = Current;
        static constexpr int temperature = Temperature;
        static constexpr int amount      = Amount;
        static constexpr int luminosity  = Luminosity;
        static constexpr int angle       = Angle;
    };

    using Dimensionless = Dimension<0,0,0,0,0,0,0,0>;
    using LengthDim     = Dimension<1,0,0,0,0,0,0,0>;
    using MassDim       = Dimension<0,1,0,0,0,0,0,0>;
    using TimeDim       = Dimension<0,0,1,0,0,0,0,0>;
    using CurrentDim    = Dimension<0,0,0,1,0,0,0,0>;
    using TempDim       = Dimension<0,0,0,0,1,0,0,0>;
    using AmountDim     = Dimension<0,0,0,0,0,1,0,0>;
    using LumDim        = Dimension<0,0,0,0,0,0,1,0>;
    using AngleDim      = Dimension<0,0,0,0,0,0,0,1>;

    template <typename D1, typename D2>
    struct DimensionMultiply {
        using type = Dimension<
            D1::length      + D2::length,
            D1::mass        + D2::mass,
            D1::time        + D2::time,
            D1::current     + D2::current,
            D1::temperature + D2::temperature,
            D1::amount      + D2::amount,
            D1::luminosity  + D2::luminosity,
            D1::angle       + D2::angle
        >;
    };

    template <typename D1, typename D2>
    struct DimensionDivide {
        using type = Dimension<
            D1::length      - D2::length,
            D1::mass        - D2::mass,
            D1::time        - D2::time,
            D1::current     - D2::current,
            D1::temperature - D2::temperature,
            D1::amount      - D2::amount,
            D1::luminosity  - D2::luminosity,
            D1::angle       - D2::angle
        >;
    };

    template <typename D1, typename D2>
    using DimensionMultiply_t = typename DimensionMultiply<D1, D2>::type;

    template <typename D1, typename D2>
    using DimensionDivide_t = typename DimensionDivide<D1, D2>::type;

    template <typename D1, typename D2>
    concept SameDimension = (
        D1::length      == D2::length &&
        D1::mass        == D2::mass &&
        D1::time        == D2::time &&
        D1::current     == D2::current &&
        D1::temperature == D2::temperature &&
        D1::amount      == D2::amount &&
        D1::luminosity  == D2::luminosity &&
        D1::angle       == D2::angle
    );

} // namespace AegisMath::Units
""",
    "include/AegisMath/Units/UnitTraits.h": """#pragma once
#include "Dimension.h"
#include "../Core/Precision.h"
#include <concepts>

namespace AegisMath::Units {

    template <typename U>
    concept IsUnitTag = requires {
        typename U::Dimension;
        { U::Ratio } -> std::convertible_to<Scalar>;
        { U::IsBaseUnit } -> std::convertible_to<bool>;
    };

} // namespace AegisMath::Units
""",
    "include/AegisMath/Units/Quantity.h": """#pragma once
#include <compare>
#include <type_traits>
#include "../Core/Precision.h"
#include "../Core/NumericTraits.h"
#include "../Core/Concepts.h"
#include "UnitTraits.h"
#include "Dimension.h"

namespace AegisMath::Units {

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    class Quantity;

    template <typename T>
    struct IsQuantityTrait : std::false_type {};

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    struct IsQuantityTrait<Quantity<T, Unit>> : std::true_type {};

    template <typename T>
    concept IsQuantity = IsQuantityTrait<T>::value;

    template <typename U1, typename U2, typename T>
    struct ProductUnitTag {
        using Dimension = DimensionMultiply_t<typename U1::Dimension, typename U2::Dimension>;
        static constexpr T Ratio = static_cast<T>(U1::Ratio * U2::Ratio);
        static constexpr bool IsBaseUnit = false;
    };

    template <typename U1, typename U2, typename T>
    struct QuotientUnitTag {
        using Dimension = DimensionDivide_t<typename U1::Dimension, typename U2::Dimension>;
        static constexpr T Ratio = static_cast<T>(U1::Ratio / U2::Ratio);
        static constexpr bool IsBaseUnit = false;
    };

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    class Quantity {
    public:
        using ValueType     = T;
        using UnitType      = Unit;
        using DimensionType = typename Unit::Dimension;

        constexpr Quantity() noexcept = default;
        explicit constexpr Quantity(T val) noexcept : m_value(val) {}

        [[nodiscard]] static constexpr Quantity Zero() noexcept {
            return Quantity(static_cast<T>(0));
        }

        [[nodiscard]] constexpr T value() const noexcept { return m_value; }

        [[nodiscard]] constexpr Quantity operator+(const Quantity& rhs) const noexcept {
            return Quantity(m_value + rhs.m_value);
        }
        [[nodiscard]] constexpr Quantity operator-(const Quantity& rhs) const noexcept {
            return Quantity(m_value - rhs.m_value);
        }
        constexpr Quantity& operator+=(const Quantity& rhs) noexcept {
            m_value += rhs.m_value; return *this;
        }
        constexpr Quantity& operator-=(const Quantity& rhs) noexcept {
            m_value -= rhs.m_value; return *this;
        }

        [[nodiscard]] constexpr Quantity operator*(T scalar) const noexcept { return Quantity(m_value * scalar); }
        [[nodiscard]] constexpr Quantity operator/(T scalar) const noexcept { return Quantity(m_value / scalar); }
        [[nodiscard]] constexpr Quantity operator-() const noexcept { return Quantity(-m_value); }

        template <IsUnitTag OtherUnit>
        requires (!SameDimension<DimensionType, typename OtherUnit::Dimension>)
        [[nodiscard]] constexpr auto operator*(const Quantity<T, OtherUnit>& rhs) const noexcept {
            using DerivedUnit = ProductUnitTag<Unit, OtherUnit, T>;
            return Quantity<T, DerivedUnit>(m_value * rhs.value());
        }

        template <IsUnitTag OtherUnit>
        requires (!SameDimension<DimensionType, typename OtherUnit::Dimension>)
        [[nodiscard]] constexpr auto operator/(const Quantity<T, OtherUnit>& rhs) const noexcept {
            using DerivedUnit = QuotientUnitTag<Unit, OtherUnit, T>;
            return Quantity<T, DerivedUnit>(m_value / rhs.value());
        }

        [[nodiscard]] constexpr auto operator<=>(const Quantity&) const = default;

    private:
        T m_value{0};
    };

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    [[nodiscard]] constexpr Quantity<T, Unit> operator*(T scalar, const Quantity<T, Unit>& q) noexcept {
        return Quantity<T, Unit>(scalar * q.value());
    }

} // namespace AegisMath::Units
""",
    "include/AegisMath/Units/UnitCast.h": """#pragma once
#include "Quantity.h"

namespace AegisMath::Units {

    template <IsUnitTag ToUnit, Concepts::FloatingPoint T, IsUnitTag FromUnit>
    requires SameDimension<typename ToUnit::Dimension, typename FromUnit::Dimension>
    [[nodiscard]] constexpr Quantity<T, ToUnit> unit_cast(const Quantity<T, FromUnit>& q) noexcept {
        if constexpr (std::is_same_v<ToUnit, FromUnit>) {
            return q;
        } else {
            constexpr T factor = static_cast<T>(FromUnit::Ratio / ToUnit::Ratio);
            return Quantity<T, ToUnit>(q.value() * factor);
        }
    }

} // namespace AegisMath::Units
""",
    "include/AegisMath/Units/BaseUnits/Length.h": """#pragma once
#include "../Quantity.h"

namespace AegisMath::Units {
    struct MeterUnit        { using Dimension = LengthDim; static constexpr Scalar Ratio = 1.0;     static constexpr bool IsBaseUnit = true; };
    struct KilometerUnit    { using Dimension = LengthDim; static constexpr Scalar Ratio = 1000.0;  static constexpr bool IsBaseUnit = false; };
    struct NauticalMileUnit { using Dimension = LengthDim; static constexpr Scalar Ratio = 1852.0;  static constexpr bool IsBaseUnit = false; };

    using Meter        = Quantity<Scalar, MeterUnit>;
    using Kilometer    = Quantity<Scalar, KilometerUnit>;
    using NauticalMile = Quantity<Scalar, NauticalMileUnit>;
}
""",
    "include/AegisMath/Units/BaseUnits/Time.h": """#pragma once
#include "../Quantity.h"

namespace AegisMath::Units {
    struct SecondTag      { using Dimension = TimeDim; static constexpr Scalar Ratio = 1.0;     static constexpr bool IsBaseUnit = true; };
    struct MillisecondTag { using Dimension = TimeDim; static constexpr Scalar Ratio = 0.001;   static constexpr bool IsBaseUnit = false; };

    using Second      = Quantity<Scalar, SecondTag>;
    using Millisecond = Quantity<Scalar, MillisecondTag>;
}
""",
    "include/AegisMath/Units/BaseUnits/Mass.h": """#pragma once
#include "../Quantity.h"

namespace AegisMath::Units {
    struct KilogramTag { using Dimension = MassDim; static constexpr Scalar Ratio = 1.0; static constexpr bool IsBaseUnit = true; };
    struct GramTag     { using Dimension = MassDim; static constexpr Scalar Ratio = 0.001; static constexpr bool IsBaseUnit = false; };

    using Kilogram = Quantity<Scalar, KilogramTag>;
    using Gram     = Quantity<Scalar, GramTag>;
}
""",
    "include/AegisMath/Units/BaseUnits/Angle.h": """#pragma once
#include "../Quantity.h"
#include "../../Core/Constants.h"

namespace AegisMath::Units {
    struct RadianTag { using Dimension = AngleDim; static constexpr Scalar Ratio = 1.0;                         static constexpr bool IsBaseUnit = true; };
    struct DegreeTag { using Dimension = AngleDim; static constexpr Scalar Ratio = Constants::DegToRadMult<Scalar>; static constexpr bool IsBaseUnit = false; };

    using Radian = Quantity<Scalar, RadianTag>;
    using Degree = Quantity<Scalar, DegreeTag>;
}
""",
    "include/AegisMath/Units/BaseUnits/Temperature.h": """#pragma once
#include "../Quantity.h"

namespace AegisMath::Units {
    struct KelvinTag   { using Dimension = TempDim; static constexpr Scalar Ratio = 1.0; static constexpr bool IsBaseUnit = true; };
    struct CelsiusTag  { using Dimension = TempDim; static constexpr Scalar Ratio = 1.0; static constexpr bool IsBaseUnit = false; };

    using Kelvin  = Quantity<Scalar, KelvinTag>;
    using Celsius = Quantity<Scalar, CelsiusTag>;
}
""",
    "include/AegisMath/Units/DerivedUnits/Velocity.h": """#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"

namespace AegisMath::Units {
    using VelocityDim = DimensionDivide_t<LengthDim, TimeDim>;

    struct MeterPerSecTag { using Dimension = VelocityDim; static constexpr Scalar Ratio = 1.0;          static constexpr bool IsBaseUnit = true; };
    struct KnotTag        { using Dimension = VelocityDim; static constexpr Scalar Ratio = 1852.0/3600.0; static constexpr bool IsBaseUnit = false; };

    using MeterPerSecond = Quantity<Scalar, MeterPerSecTag>;
    using Knot           = Quantity<Scalar, KnotTag>;
}
""",
    "include/AegisMath/Units/DerivedUnits/Acceleration.h": """#pragma once
#include "../Quantity.h"
#include "Velocity.h"
#include "../BaseUnits/Time.h"

namespace AegisMath::Units {
    using AccelerationDim = DimensionDivide_t<VelocityDim, TimeDim>;

    struct MeterPerSecSqTag { using Dimension = AccelerationDim; static constexpr Scalar Ratio = 1.0; static constexpr bool IsBaseUnit = true; };

    using MeterPerSecondSquared = Quantity<Scalar, MeterPerSecSqTag>;
}
""",
    "include/AegisMath/Units/DerivedUnits/Frequency.h": """#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Time.h"

namespace AegisMath::Units {
    using FrequencyDim = DimensionDivide_t<Dimension<0,0,0,0,0,0,0,0>, TimeDim>;

    struct HertzTag { using Dimension = FrequencyDim; static constexpr Scalar Ratio = 1.0; static constexpr bool IsBaseUnit = true; };

    using Hertz = Quantity<Scalar, HertzTag>;
}
""",
    "include/AegisMath/Units/Literals.h": """#pragma once
#include "BaseUnits/Length.h"
#include "BaseUnits/Time.h"
#include "BaseUnits/Angle.h"

namespace AegisMath::Units::Literals {
    consteval Length::Meter operator""_m(long double val) { return Length::Meter(static_cast<Scalar>(val)); }
    consteval Length::Meter operator""_m(unsigned long long val) { return Length::Meter(static_cast<Scalar>(val)); }
    
    consteval Time::Second operator""_s(long double val) { return Time::Second(static_cast<Scalar>(val)); }
    consteval Time::Second operator""_s(unsigned long long val) { return Time::Second(static_cast<Scalar>(val)); }
}
""",
    "include/AegisMath/Units/Common.h": """#pragma once
#include "Dimension.h"
#include "UnitTraits.h"
#include "Quantity.h"
#include "UnitCast.h"

#include "BaseUnits/Length.h"
#include "BaseUnits/Time.h"
#include "BaseUnits/Angle.h"
#include "BaseUnits/Mass.h"
#include "BaseUnits/Temperature.h"

#include "DerivedUnits/Velocity.h"
#include "DerivedUnits/Acceleration.h"
#include "DerivedUnits/Frequency.h"
""",
    "tests/Units/UnitsTest.cpp": """#include <gtest/gtest.h>
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Core/Constants.h"
#include "AegisMath/Units/Common.h"

using namespace AegisMath;
using namespace AegisMath::Units;

TEST(UnitsSystemRevisionATest, MemoryAlignmentAndContracts) {
    static_assert(sizeof(Meter) == sizeof(Scalar), "Quantity memory footprint must match Scalar!");
    static_assert(alignof(Meter) == alignof(Scalar), "Quantity alignment must match Scalar!");
    static_assert(std::is_trivially_copyable_v<Meter>, "Quantity must be trivially copyable!");
}

TEST(UnitsSystemRevisionATest, SafeZeroAndArithmetic) {
    Meter zero_m = Meter::Zero();
    EXPECT_EQ(zero_m.value(), 0.0);

    Meter m1{100.0};
    Meter m2{50.0};
    EXPECT_EQ((m1 + m2).value(), 150.0);
}

TEST(UnitsSystemRevisionATest, DimensionAlgebraDerived) {
    Meter dist{200.0};
    Second time{10.0};

    auto velocity = dist / time;
    EXPECT_TRUE(Traits::AlmostEqual(velocity.value(), static_cast<Scalar>(20.0), 1e-6, 1e-6));
}
"""
}

if __name__ == "__main__":
    for path, content in files_content.items():
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(content.strip() + "\n")
        print(f"[SUCCESS] Updated/Created -> {path}")

    print("\n========================================")
    print("All Units v1.0 Revision A files corrected!")
    print("========================================")