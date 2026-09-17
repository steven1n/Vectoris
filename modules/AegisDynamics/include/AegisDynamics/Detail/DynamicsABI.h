#pragma once

#include <type_traits>
#include <cstddef>

namespace AegisDynamics::Detail {
    template<typename T>
    struct DynamicsABIValidator {
        static constexpr bool value =
                std::is_standard_layout_v<T> &&
                std::is_trivially_copyable_v<T>;
    };
} // namespace AegisDynamics::Detail