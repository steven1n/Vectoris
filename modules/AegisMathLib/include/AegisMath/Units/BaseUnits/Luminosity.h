#pragma once
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct CandelaUnit {
        using Dimension = LumDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Candela = Quantity<Scalar, CandelaUnit>;

    static_assert(Detail::ValidateQuantityABI<Candela>(), "Candela ABI contract violation!");

} // namespace AegisMath::Units