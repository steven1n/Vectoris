#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Angle.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct RadianPerSecondSquaredUnit {
        using Dimension = AngularAccelerationDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using RadianPerSecondSquared = Quantity<Scalar, RadianPerSecondSquaredUnit>;
    using AngularAcceleration     = RadianPerSecondSquared;

    static_assert(Detail::ValidateQuantityABI<AngularAcceleration>(), "AngularAcceleration ABI contract violation!");

} // namespace AegisMath::Units
