#pragma once
#include <type_traits>

namespace AegisMath::Geometry::Detail {

    // 仅作通用的 C 兼容和 Trivially Copyable 验证
    template<typename V>
    struct GeometryABIValidator {
        static_assert(std::is_standard_layout_v<V>,
            "Geometry type must be standard layout.");

        static_assert(std::is_trivially_copyable_v<V>,
            "Geometry type must be trivially copyable for DMA.");

        static constexpr bool value = true;
    };

} // namespace AegisMath::Geometry::Detail