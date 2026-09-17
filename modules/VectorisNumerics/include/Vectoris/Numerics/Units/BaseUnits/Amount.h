#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct MoleUnit {
        using Dimension = AmountDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Mole = Quantity<Scalar, MoleUnit>;

    static_assert(Detail::ValidateQuantityABI<Mole>(), "Mole ABI contract violation!");

} // namespace vectoris::numerics::Units