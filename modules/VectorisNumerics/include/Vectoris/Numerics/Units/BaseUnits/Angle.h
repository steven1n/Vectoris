#pragma once
#include "../Namespace.h"
#include "../Quantity.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct RadianUnit {
        using Dimension = AngleDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = true;
    };

    using Radian = Quantity<Scalar, RadianUnit>;

    static_assert(Detail::ValidateQuantityABI<Radian>(), "Radian ABI contract violation!");

} // namespace vectoris::numerics::Units