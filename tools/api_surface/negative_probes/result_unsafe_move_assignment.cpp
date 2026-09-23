// VRT-13 controlled negative move-assignment probe.
#include "Vectoris/Numerics/Core/Result.h"
#include <utility>
struct Payload final {
    int id;
    explicit Payload(int value) noexcept : id(value) {}
    Payload(const Payload& other) noexcept(false) : id(other.id) {}
    Payload(Payload&& other) noexcept(false) : id(other.id) {}
    Payload& operator=(const Payload&) = default;
    Payload& operator=(Payload&&) = default;
    ~Payload() = default;
};
int main() {
    using R = vectoris::numerics::core::Result<Payload, int>;
    auto target = R::failure(17);
    auto source = R::success(42);
    target = std::move(source);
    return target.has_value() ? 0 : 1;
}
