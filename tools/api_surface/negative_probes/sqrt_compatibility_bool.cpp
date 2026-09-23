// VRT-14 controlled negative scalar-domain probe.
#include <Vectoris/Numerics/Core/MathFunctions.h>
struct Convertible final { operator double() const noexcept { return 4.0; } };
int main() {
    const auto result = vectoris::numerics::core::Math::sqrt(true);
    return result > 0.0 ? 0 : 1;
}
