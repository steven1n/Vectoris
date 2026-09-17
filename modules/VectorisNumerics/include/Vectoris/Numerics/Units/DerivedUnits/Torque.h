#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Mass.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "Force.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct NewtonMeterUnit {
        using Dimension = TorqueDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using NewtonMeter = Quantity<Scalar, NewtonMeterUnit>;
    using Torque      = NewtonMeter;
    using Moment      = Torque;

    static_assert(Detail::ValidateQuantityABI<Torque>(), "Torque ABI contract violation!");

} // namespace vectoris::numerics::Units
