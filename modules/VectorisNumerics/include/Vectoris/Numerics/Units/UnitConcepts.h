#pragma once
#include "Namespace.h"
#include <type_traits>
#include "../Core/Concepts.h"
#include "UnitTraits.h"

namespace vectoris::numerics::Units {

    // 前置声明
    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    class Quantity;

    // 1. 恢复严格的 IsQuantityTrait 检查，防止伪造弱类型
    template <typename T>
    struct IsQuantityTrait : std::false_type {};

    template <Concepts::FloatingPoint T, IsUnitTag Unit>
    struct IsQuantityTrait<Quantity<T, Unit>> : std::true_type {};

    template <typename T>
    concept IsQuantity = IsQuantityTrait<T>::value;

} // namespace vectoris::numerics::Units
