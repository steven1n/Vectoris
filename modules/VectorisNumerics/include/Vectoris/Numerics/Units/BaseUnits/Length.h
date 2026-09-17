#pragma once
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

    // 语义明确：Meter satisfies ABI Contract
    static_assert(Detail::QuantityABIContract<Meter>, "Meter ABI contract violation!");

} // namespace vectoris::numerics::Units