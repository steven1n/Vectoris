// Negative Compile Probe: Cross-Frame Arithmetic Rejection
// Expected compile failure: Vector3 addition across incompatible coordinate frames must be rejected.
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"

int main() {
    using namespace vectoris::numerics::Geometry;
    Vector3<double, WorldFrame> v1(1.0, 2.0, 3.0);
    Vector3<double, BodyFrame> v2(4.0, 5.0, 6.0);
    // Invalid cross-frame addition
    auto sum = v1 + v2;
    (void)sum;
    return 0;
}
