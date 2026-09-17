#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct KilogramUnit {
        using Dimension = MassDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    struct GramUnit {
        using Dimension = MassDimension;
        using Ratio     = std::ratio<1, 1000>;
        static constexpr bool IsBaseUnit = false;
    };

    using Kilogram = Quantity<Scalar, KilogramUnit>;
    using Mass     = Kilogram;
    using Gram     = Quantity<Scalar, GramUnit>;

    static_assert(Detail::ValidateQuantityABI<Kilogram>(), "Kilogram ABI contract violation!");
    static_assert(Detail::ValidateQuantityABI<Gram>(), "Gram ABI contract violation!");

} // namespace vectoris::numerics::Units