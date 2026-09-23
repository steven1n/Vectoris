#pragma once
#include "Namespace.h"
#include <type_traits>
#include <concepts>

namespace vectoris::numerics::Geometry {

    // Geometry aggregates are not scalars, regardless of their operator set.
    // Specializations must precede use in scalar overload resolution.
    template <typename T>
    inline constexpr bool is_geometry_aggregate_v = false;

    // Reject aggregates before probing operators that may themselves require
    // ScalarArithmetic. Preserve support for user-defined scalar arithmetic.
    template<typename T>
    concept ScalarArithmetic =
        (!is_geometry_aggregate_v<std::remove_cvref_t<T>>) && requires(T a, T b) {
        { a + b }; { a - b }; { a * b }; { a / b };
        { -a };
    };

    // 增强的 FrameTag：确保幽灵类型在任何场景下(包括 Traits/序列化)绝对安全
    template<typename F>
    concept FrameTag = std::is_empty_v<F> &&
                       std::is_trivial_v<F> &&
                       std::is_standard_layout_v<F>;

} // namespace vectoris::numerics::Geometry