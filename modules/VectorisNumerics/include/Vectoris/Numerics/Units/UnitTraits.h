#pragma once
#include "Namespace.h"
#include "Dimension.h"
#include "../Core/Precision.h"
#include <concepts>
#include <ratio>

namespace vectoris::numerics::Units {

    template <typename R>
    struct IsStdRatio : std::false_type {};

    template <std::intmax_t Num, std::intmax_t Den>
    struct IsStdRatio<std::ratio<Num, Den>> : std::true_type {};

    template <typename U>
    concept IsUnitTag = requires {
        typename U::Dimension;
        typename U::Ratio;
    } &&
    IsStdRatio<typename U::Ratio>::value &&
    requires {
        { U::IsBaseUnit } -> std::convertible_to<bool>;
    };

} // namespace vectoris::numerics::Units