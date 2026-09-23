// VRT-13 controlled negative payload-destruction probe.
#include "Vectoris/Numerics/Core/Result.h"
struct Payload final {
    explicit Payload(int) noexcept {}
    ~Payload() noexcept(false) {}
};
int main() {
    using R = vectoris::numerics::core::Result<Payload, int>;
    auto result = R::success(42);
    return result.value_if() == nullptr ? 1 : 0;
}
