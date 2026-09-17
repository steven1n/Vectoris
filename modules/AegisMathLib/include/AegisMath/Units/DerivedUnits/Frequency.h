#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Time.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct HertzUnit {
        using Dimension = FrequencyDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Frequency = Quantity<Scalar, HertzUnit>;
    using Hertz = Frequency;

    static_assert(Detail::ValidateQuantityABI<Frequency>(), "Frequency (Hertz) ABI contract violation!");

} // namespace AegisMath::Units