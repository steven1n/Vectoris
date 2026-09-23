#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../BaseUnits/Mass.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "../BaseUnits/Angle.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct KilogramMeterSquaredPerSecondRadianUnit {
        using Dimension = AngularMomentumDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using KilogramMeterSquaredPerSecondRadian = Quantity<Scalar, KilogramMeterSquaredPerSecondRadianUnit>;
    using AngularMomentumUnit               = KilogramMeterSquaredPerSecondRadianUnit;
    using AngularMomentum                   = KilogramMeterSquaredPerSecondRadian;

    static_assert(Detail::ValidateQuantityABI<AngularMomentum>(), "AngularMomentum ABI contract violation!");

} // namespace vectoris::numerics::Units
