#pragma once

#include "AegisMath/Geometry/Concepts.h"
#include "AegisMath/Core/Concepts.h" // 确保引入 Core 层的基础 Concepts

namespace AegisMath::Dynamics {

    // 严格对齐 AegisMathLib 既有的标量与数值概念体系
    template <typename T>
    concept DynamicsScalar = AegisMath::Geometry::ScalarArithmetic<T> || AegisMath::Concepts::Numeric<T>;

} // namespace AegisMath::Dynamics