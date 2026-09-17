#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct AmpereUnit {
        using Dimension = CurrentDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Ampere = Quantity<Scalar, AmpereUnit>;

    static_assert(Detail::ValidateQuantityABI<Ampere>(), "Ampere ABI contract violation!");

} // namespace vectoris::numerics::Units