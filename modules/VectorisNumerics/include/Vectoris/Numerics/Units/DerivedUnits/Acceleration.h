#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct MeterPerSecondSquaredUnit {
        using Dimension = AccelerationDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Acceleration = Quantity<Scalar, MeterPerSecondSquaredUnit>;

    static_assert(Detail::ValidateQuantityABI<Acceleration>(), "Acceleration ABI contract violation!");

} // namespace vectoris::numerics::Units