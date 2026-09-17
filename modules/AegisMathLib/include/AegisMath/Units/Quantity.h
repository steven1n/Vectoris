#pragma once
#include <compare>
#include <type_traits>
#include <ratio>
#include "../Core/Precision.h"
#include "../Core/NumericTraits.h"
#include "../Core/Concepts.h"
#include "UnitTraits.h"
#include "Dimension.h"
#include "UnitConcepts.h"
#include "QuantityABI.h"

namespace AegisMath::Units {

    struct MultiplyOperation {};
    struct DivideOperation {};

    template <typename R1, typename R2, template <typename, typename> typename RatioOp>
    struct SafeRatioOperation {
        using type = RatioOp<R1, R2>;
        static_assert(type::num != 0 && type::den != 0, "Ratio operation result invalid (zero denominator or overflow)!");
    };

    template <typename U1, typename U2, typename Op>
    struct DerivedUnitTagImpl;

    template <typename U1, typename U2>
    struct DerivedUnitTagImpl<U1, U2, MultiplyOperation> {
        using Dimension = DimensionAdd_t<typename U1::Dimension, typename U2::Dimension>;
        using Ratio     = typename SafeRatioOperation<typename U1::Ratio, typename U2::Ratio, std::ratio_multiply>::type;
        static constexpr bool IsBaseUnit = false;
    };

    template <typename U1, typename U2>
    struct DerivedUnitTagImpl<U1, U2, DivideOperation> {
        using Dimension = DimensionSubtract_t<typename U1::Dimension, typename U2::Dimension>;
        using Ratio     = typename SafeRatioOperation<typename U1::Ratio, typename U2::Ratio, std::ratio_divide>::type;
        static constexpr bool IsBaseUnit = false;
    };

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    class Quantity {
    public:
        using ValueType     = T;
        using UnitType      = Unit;
        using DimensionType = typename Unit::Dimension;
        using RatioType     = typename Unit::Ratio;

        Quantity() = delete;

        explicit constexpr Quantity(T val) noexcept : m_value(val) {}

        // 同量纲且同比率的导出单位可隐式转换构造 (例如 Force * Length 构造 Torque)
        template <IsUnitTag OtherUnit>
        requires (!std::is_same_v<Unit, OtherUnit>) &&
                 DimensionEqual<typename Unit::Dimension, typename OtherUnit::Dimension> &&
                 std::is_same_v<typename Unit::Ratio, typename OtherUnit::Ratio>
        constexpr Quantity(const Quantity<T, OtherUnit>& other) noexcept : m_value(other.value()) {}

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

        template <Concepts::FloatingPoint S>
        requires std::same_as<S, T>
        [[nodiscard]] constexpr Quantity operator*(S scalar) const noexcept { return Quantity(m_value * scalar); }

        template <Concepts::FloatingPoint S>
        requires std::same_as<S, T>
        [[nodiscard]] constexpr Quantity operator/(S scalar) const noexcept { return Quantity(m_value / scalar); }

        [[nodiscard]] constexpr Quantity operator-() const noexcept { return Quantity(-m_value); }

        template <IsUnitTag OtherUnit>
        [[nodiscard]] constexpr auto operator*(const Quantity<T, OtherUnit>& rhs) const noexcept {
            using NewUnit = DerivedUnitTagImpl<Unit, OtherUnit, MultiplyOperation>;
            return Quantity<T, NewUnit>(m_value * rhs.value());
        }

        template <IsUnitTag OtherUnit>
        [[nodiscard]] constexpr auto operator/(const Quantity<T, OtherUnit>& rhs) const noexcept {
            using NewUnit = DerivedUnitTagImpl<Unit, OtherUnit, DivideOperation>;
            return Quantity<T, NewUnit>(m_value / rhs.value());
        }

        [[nodiscard]] constexpr auto operator<=>(const Quantity&) const = default;

    private:
        T m_value;
    };

    // 标量左乘
    template <Concepts::FloatingPoint T, IsUnitTag Unit, Concepts::FloatingPoint S>
    requires std::same_as<S, T>
    [[nodiscard]] constexpr Quantity<T, Unit> operator*(S scalar, const Quantity<T, Unit>& q) noexcept {
        return Quantity<T, Unit>(scalar * q.value());
    }

} // namespace AegisMath::Units