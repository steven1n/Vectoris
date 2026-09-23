// Negative Compile Probe: Unsupported Scalar Arithmetic Rejection
// Expected compile failure: Matrix3 multiplication by non-scalar types must be rejected.
#include <string>
#include "Vectoris/Numerics/Geometry/Matrix3.h"

int main() {
    using namespace vectoris::numerics::Geometry;
    Matrix3<double> m = Matrix3<double>::Identity();
    std::string str = "invalid_scalar";
    // Invalid scalar multiplication
    auto res = m * str;
    (void)res;
    return 0;
}
