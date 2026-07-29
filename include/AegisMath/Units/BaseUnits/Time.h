#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct SecondUnit {
        using Dimension = TimeDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Second = Quantity<Scalar, SecondUnit>;

    static_assert(Detail::ValidateQuantityABI<Second>(), "Second ABI contract violation!");

} // namespace AegisMath::Units