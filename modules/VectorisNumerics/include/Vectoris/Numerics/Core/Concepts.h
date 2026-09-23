#pragma once
#include "Namespace.h"
#include <concepts>
#include <type_traits>

namespace vectoris::numerics::Concepts {

    // 限定浮点类型
    template <typename T>
    concept FloatingPoint = std::floating_point<T>;

    // 公共 sqrt 标量域；float/double 保证 constexpr，long double 保留运行期支持。
    template <typename T>
    concept SupportedSqrtScalar = std::floating_point<std::remove_cvref_t<T>>;

    // 屏蔽字符与布尔类型的辅助概念
    template <typename T>
    concept Character = std::same_as<std::remove_cv_t<T>, char> ||
                        std::same_as<std::remove_cv_t<T>, wchar_t> ||
                        std::same_as<std::remove_cv_t<T>, char8_t> ||
                        std::same_as<std::remove_cv_t<T>, char16_t> ||
                        std::same_as<std::remove_cv_t<T>, char32_t>;

    template <typename T>
    concept Boolean = std::same_as<std::remove_cv_t<T>, bool>;

    // 纯粹的整数概念 (排除了 bool 和 char)
    template <typename T>
    concept NumericInteger = std::integral<T> && !Boolean<T> && !Character<T>;

    template <typename T>
    concept SignedInteger = NumericInteger<T> && std::is_signed_v<T>;

    template <typename T>
    concept UnsignedInteger = NumericInteger<T> && std::is_unsigned_v<T>;

    // 纯数学计算数值类型
    template <typename T>
    concept Numeric = FloatingPoint<T> || NumericInteger<T>;

    // 强制类型一致性约束
    template <typename T, typename U>
    concept SameType = std::same_as<T, U>;

} // namespace vectoris::numerics::Concepts