// Positive Control Probe for Result Same Type Rejection
// Proves the compiler, include paths, and Result<T, E> work for valid distinct types (int != double).
#include "Vectoris/Numerics/Core/Result.h"

int main() {
    using namespace vectoris::numerics::Core;
    Result<int, double> r = Result<int, double>::success(42);
    (void)r;
    return 0;
}
