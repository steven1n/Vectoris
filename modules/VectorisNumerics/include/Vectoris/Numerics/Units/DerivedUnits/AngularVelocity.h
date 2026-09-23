#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../BaseUnits/Angle.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct RadianPerSecondUnit {
        using Dimension = AngularVelocityDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using RadianPerSecond = Quantity<Scalar, RadianPerSecondUnit>;
    using AngularVelocity = RadianPerSecond;

    static_assert(Detail::ValidateQuantityABI<AngularVelocity>(), "AngularVelocity ABI contract violation!");

} // namespace vectoris::numerics::Units
