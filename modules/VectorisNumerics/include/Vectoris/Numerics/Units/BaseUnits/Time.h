#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct SecondUnit {
        using Dimension = TimeDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Second = Quantity<Scalar, SecondUnit>;
    using Time   = Second;

    static_assert(Detail::ValidateQuantityABI<Second>(), "Second ABI contract violation!");

} // namespace vectoris::numerics::Units