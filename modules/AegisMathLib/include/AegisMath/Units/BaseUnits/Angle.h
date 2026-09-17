#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct RadianUnit {
        using Dimension = AngleDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Radian = Quantity<Scalar, RadianUnit>;

    static_assert(Detail::ValidateQuantityABI<Radian>(), "Radian ABI contract violation!");

} // namespace AegisMath::Units