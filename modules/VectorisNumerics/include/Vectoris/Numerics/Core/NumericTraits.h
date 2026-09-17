#pragma once
#include <limits>
#include <cmath>
#include <algorithm>
#include "Concepts.h"
#include "BasicTypes.h"

// 调整：所有数值特征与状态判定统一收拢到 Traits 命名空间下
namespace vectoris::numerics::Traits {

    // =========================================================================
    // 数值特征结构体
    // =========================================================================
    template <Concepts::Numeric T>
    struct NumericTraits {
        static constexpr T epsilon() noexcept { return std::numeric_limits<T>::epsilon(); }
        static constexpr T max() noexcept { return std::numeric_limits<T>::max(); }
        static constexpr T lowest() noexcept { return std::numeric_limits<T>::lowest(); }
        static constexpr T infinity() noexcept { return std::numeric_limits<T>::infinity(); }
        static constexpr T quietNaN() noexcept { return std::numeric_limits<T>::quiet_NaN(); }
    };

    // =========================================================================
    // 状态检测辅助函数
    // =========================================================================
    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Bool IsNaN(T value) noexcept {
        if (std::is_constant_evaluated()) {
            return value != value;
        }
        return std::isnan(value);
    }

    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Bool IsInfinity(T value) noexcept {
        if (std::is_constant_evaluated()) {
            return (value == NumericTraits<T>::infinity()) || (value == -NumericTraits<T>::infinity());
        }
        return std::isinf(value);
    }

    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Bool IsFinite(T value) noexcept {
        if (std::is_constant_evaluated()) {
            return (value == value) && (value <= NumericTraits<T>::max()) && (value >= -NumericTraits<T>::max());
        }
        return std::isfinite(value);
    }

    template <Concepts::FloatingPoint T>
    [[nodiscard]] inline Bool IsZero(T value, T tolerance = NumericTraits<T>::epsilon()) noexcept {
        return std::abs(value) <= tolerance;
    }

    // =========================================================================
    // 工业级浮点安全比较算法
    // =========================================================================
    template <Concepts::FloatingPoint T>
    [[nodiscard]] inline Bool AlmostEqual(T a, T b, T absoluteTolerance, T relativeTolerance) noexcept {
        // [防御] 拦截 NaN：NaN 与任何数值（包括其自身）比较均为 false
        if (IsNaN(a) || IsNaN(b)) {
            return false;
        }

        // 快速路径：处理完全相等的情况 (也同时正确处理了同号 Infinity == Infinity)
        if (a == b) {
            return true;
        }

        const T diff = std::abs(a - b);
        const T absA = std::abs(a);
        const T absB = std::abs(b);
        const T maxAbs = std::max(absA, absB);

        return diff <= std::max(absoluteTolerance, relativeTolerance * maxAbs);
    }

} // namespace vectoris::numerics::Traits