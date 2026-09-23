// VRT-12: isolated include permutation, canonical and compatibility lookup.
#include <Vectoris/Numerics/Units/Dimension.h>
static_assert(vectoris::numerics::units::LengthDimension::length == 1);
static_assert(vectoris::numerics::Units::LengthDimension::length == 1);
#include <Vectoris/Numerics/Geometry/CoordinateConvention.h>
static_assert(vectoris::numerics::geometry::SystemConvention::FrameMapping == vectoris::numerics::geometry::RotationConvention::SourceToTarget);
static_assert(vectoris::numerics::Geometry::SystemConvention::FrameMapping == vectoris::numerics::Geometry::RotationConvention::SourceToTarget);
