#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct HertzUnit {
        using Dimension = FrequencyDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Frequency = Quantity<Scalar, HertzUnit>;
    using Hertz = Frequency;

    static_assert(Detail::ValidateQuantityABI<Frequency>(), "Frequency (Hertz) ABI contract violation!");

} // namespace vectoris::numerics::Units