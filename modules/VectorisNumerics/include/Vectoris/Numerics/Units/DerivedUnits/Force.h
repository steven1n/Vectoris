#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Mass.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct NewtonUnit {
        using Dimension = ForceDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Force = Quantity<Scalar, NewtonUnit>;
    using Newton = Force;

    static_assert(Detail::ValidateQuantityABI<Force>(), "Force (Newton) ABI contract violation!");

} // namespace vectoris::numerics::Units