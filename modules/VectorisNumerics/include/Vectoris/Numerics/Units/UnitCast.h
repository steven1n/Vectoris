#pragma once
#include "Quantity.h"
#include <ratio>

namespace vectoris::numerics::Units {

    template <IsUnitTag ToUnit, Concepts::FloatingPoint T, IsUnitTag FromUnit>
    requires DimensionEqual<typename ToUnit::Dimension, typename FromUnit::Dimension>
    [[nodiscard]] constexpr Quantity<T, ToUnit> unit_cast(const Quantity<T, FromUnit>& q) noexcept {
        if constexpr (std::is_same_v<ToUnit, FromUnit>) {
            return q;
        } else {
            using ConversionRatio = std::ratio_divide<typename FromUnit::Ratio, typename ToUnit::Ratio>;

            // 采用高精度 long double 过渡计算转换因子，规避直接转换带来的截断或警告
            constexpr auto numerator   = static_cast<long double>(ConversionRatio::num);
            constexpr auto denominator = static_cast<long double>(ConversionRatio::den);
            constexpr T factor         = static_cast<T>(numerator / denominator);

            return Quantity<T, ToUnit>(q.value() * factor);
        }
    }

} // namespace vectoris::numerics::Units