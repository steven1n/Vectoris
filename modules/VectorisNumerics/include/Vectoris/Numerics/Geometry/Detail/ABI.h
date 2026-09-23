#pragma once
#include "../Namespace.h"
#include <type_traits>

namespace vectoris::numerics::Geometry::Detail {

    // Checks C++ standard-layout and trivial-copy properties only. These traits do not promise
    // cross-language ABI, DMA, wire-format, or persistent-storage compatibility.
    template<typename V>
    struct GeometryABIValidator {
        static_assert(std::is_standard_layout_v<V>,
            "Geometry type must be standard layout.");

        static_assert(std::is_trivially_copyable_v<V>,
            "Geometry type must be trivially copyable.");

        static constexpr bool value = true;
    };

} // namespace vectoris::numerics::Geometry::Detail
