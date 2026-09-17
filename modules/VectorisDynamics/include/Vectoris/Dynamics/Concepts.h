#pragma once

#include "Vectoris/Numerics/Core/BasicTypes.h"
#include "Vectoris/Numerics/Core/Concepts.h"
#include "Vectoris/Numerics/Geometry/Concepts.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Units/UnitConcepts.h"

namespace vectoris::numerics {
    namespace Concepts {}
    namespace Core {}
    namespace Geometry {}
    namespace Units {}
    namespace Traits {}
}

namespace vectoris::dynamics {

    namespace Concepts = vectoris::numerics::Concepts;
    namespace Core     = vectoris::numerics::Core;
    namespace Geometry = vectoris::numerics::Geometry;
    namespace Units    = vectoris::numerics::Units;
    namespace Traits   = vectoris::numerics::Traits;

    using Scalar = vectoris::numerics::Scalar;

    // 严格对齐 VectorisNumerics 既有的标量与数值概念体系
    template <typename T>
    concept DynamicsScalar = Geometry::ScalarArithmetic<T> || Concepts::Numeric<T>;

} // namespace vectoris::dynamics