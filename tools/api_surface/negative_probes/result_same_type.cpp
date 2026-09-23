// Negative Compile Probe: Result Same Type (T == E) Rejection
// Expected compile failure: Result<T, E> where T == E must trigger static_assert failure.
#include "Vectoris/Numerics/Core/Result.h"

int main() {
    using namespace vectoris::numerics::Core;
    // Invalid Result instantiation with identical value and error types
    Result<int, int> r = Result<int, int>::success(42);
    (void)r;
    return 0;
}
