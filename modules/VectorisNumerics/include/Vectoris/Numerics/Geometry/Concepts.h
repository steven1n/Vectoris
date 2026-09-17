#pragma once
#include <type_traits>
#include <concepts>

namespace vectoris::numerics::Geometry {

    // 约束 T 必须支持基础数学代数，且结果仍为算术类型
    template<typename T>
    concept ScalarArithmetic = requires(T a, T b) {
        { a + b }; { a - b }; { a * b }; { a / b };
        { -a };
    };

    // 增强的 FrameTag：确保幽灵类型在任何场景下(包括 Traits/序列化)绝对安全
    template<typename F>
    concept FrameTag = std::is_empty_v<F> &&
                       std::is_trivial_v<F> &&
                       std::is_standard_layout_v<F>;

} // namespace vectoris::numerics::Geometry