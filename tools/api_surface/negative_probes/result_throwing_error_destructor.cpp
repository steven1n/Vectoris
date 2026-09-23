// VRT-13 controlled negative payload-destruction probe.
#include "Vectoris/Numerics/Core/Result.h"
struct Payload final {
    explicit Payload(int) noexcept {}
    ~Payload() noexcept(false) {}
};
int main() {
    using R = vectoris::numerics::core::Result<int, Payload>;
    auto result = R::failure(42);
    return result.error_if() == nullptr ? 1 : 0;
}
