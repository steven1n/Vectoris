#pragma once
#include "Concepts.h"

namespace vectoris::numerics::Geometry {

    // Unknown/Degenerate Coordinate System
    struct FrameUnknown final {};

    // Common Aerospace Position
    struct FrameECEF final {}; // Earth-Centered, Earth-Fixed
    struct FrameENU final {};  // East-North-Up
    struct FrameNED final {};  // North-East-Down (Common in Aerospace)
    struct FrameBody final {}; // Body Frame (AirCraft Body Frame)
    
    // 惯性系
    struct FrameECI final {};  // Earth-Centered Inertial

    static_assert(FrameTag<FrameUnknown>);
    static_assert(FrameTag<FrameECEF>);
    static_assert(FrameTag<FrameBody>);

} // namespace vectoris::numerics::Geometry

namespace vectoris::numerics {
    namespace geometry = Geometry;
}