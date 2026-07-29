#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct KelvinUnit {
        using Dimension = TempDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Kelvin = Quantity<Scalar, KelvinUnit>;

    static_assert(Detail::ValidateQuantityABI<Kelvin>(), "Kelvin ABI contract violation!");

} // namespace AegisMath::Units