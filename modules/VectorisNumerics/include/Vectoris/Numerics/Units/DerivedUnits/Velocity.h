#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct MeterPerSecondUnit {
        using Dimension = VelocityDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Velocity = Quantity<Scalar, MeterPerSecondUnit>;

    // Exported units satisfy the declared C++ object-representation checks.
    static_assert(Detail::ValidateQuantityABI<Velocity>(), "Velocity ABI contract violation!");

} // namespace vectoris::numerics::Units
