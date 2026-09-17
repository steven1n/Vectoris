#pragma once

#include "AegisMath/Core/BasicTypes.h"
#include "AegisMath/Core/Concepts.h"
#include "AegisMath/Geometry/Concepts.h"
#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Units/UnitConcepts.h"

namespace AegisMath {
    namespace Concepts {}
    namespace Core {}
    namespace Geometry {}
    namespace Units {}
    namespace Traits {}
}

namespace AegisDynamics {

    namespace Concepts = AegisMath::Concepts;
    namespace Core     = AegisMath::Core;
    namespace Geometry = AegisMath::Geometry;
    namespace Units    = AegisMath::Units;
    namespace Traits   = AegisMath::Traits;

    using Scalar = AegisMath::Scalar;

    // 严格对齐 AegisMathLib 既有的标量与数值概念体系
    template <typename T>
    concept DynamicsScalar = Geometry::ScalarArithmetic<T> || Concepts::Numeric<T>;

} // namespace AegisDynamics