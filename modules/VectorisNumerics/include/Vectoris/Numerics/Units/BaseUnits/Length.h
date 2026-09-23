#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct MeterUnit {
        using Dimension = LengthDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Meter  = Quantity<Scalar, MeterUnit>;
    using Length = Meter;

    // Meter satisfies the declared C++ object-representation checks.
    static_assert(Detail::QuantityABIContract<Meter>, "Meter ABI contract violation!");

} // namespace vectoris::numerics::Units
