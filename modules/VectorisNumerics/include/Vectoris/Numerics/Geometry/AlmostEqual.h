#pragma once

#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Point3.h"
#include "Matrix3.h"
#include "UnitVector3.h"
#include "RotationMatrix3.h"
#include "Quaternion.h"
#include "Transform3.h"

namespace vectoris::numerics::Geometry {

    // Centralized Geometry AlmostEqual & Equivalence Interface
    //
    // All geometry types in AegisMathLib support:
    // 1. operator== / operator!= : Exact component-wise stored-value equality under C++ floating-point == semantics.
    // 2. AlmostEqual(...)         : Tolerance-aware numerical closeness with absolute & relative tolerances.
    // 3. RotationEquivalent(...)   : SO(3) rotational equivalence for quaternions (q == q or q == -q).

} // namespace vectoris::numerics::Geometry
