#pragma once
#include "Namespace.h"
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

    namespace Detail {
        // Preconditions: finite, unequal inputs and finite nonnegative tolerances.
        template <Concepts::FloatingPoint T>
        [[nodiscard]] inline Bool FiniteAlmostEqual(T a, T b, T absoluteTolerance,
                                                    T relativeTolerance) noexcept {
            const T high = std::max(std::abs(a), std::abs(b));
            const T low = std::min(std::abs(a), std::abs(b));
            if ((a < T{0}) == (b < T{0})) {
                // Same-sign subtraction cannot overflow. Keep the absolute check
                // at the original scale, including subnormal differences.
                const T diff = high - low;
                return diff <= absoluteTolerance || diff / high <= relativeTolerance;
            }
            // Opposite signs: |a-b| = high+low. Rearrange the absolute check
            // without forming that sum or overflowing a tolerance product.
            if (high <= absoluteTolerance && low <= absoluteTolerance - high) {
                return true;
            }
            // Relative difference = 1+low/high. Handle the exact endpoint before
            // dividing: low/high can underflow even though low is nonzero.
            if (relativeTolerance <= T{1}) {
                return relativeTolerance == T{1} && low == T{0};
            }
            return relativeTolerance >= T{2} || low / high <= relativeTolerance - T{1};
        }
    } // namespace Detail

    // =========================================================================
    // 工业级浮点安全比较算法
    // =========================================================================
    template <Concepts::FloatingPoint T>
    [[nodiscard]] inline Bool AlmostEqual(T a, T b, T absoluteTolerance, T relativeTolerance) noexcept {
        // Invalid tolerances always fail, including exact equality and infinity.
        if (!IsFinite(absoluteTolerance) || !IsFinite(relativeTolerance) ||
            absoluteTolerance < T{0} || relativeTolerance < T{0}) {
            return false;
        }
        // [防御] 拦截 NaN：NaN 与任何数值（包括其自身）比较均为 false
        if (IsNaN(a) || IsNaN(b)) {
            return false;
        }

        // [IEEE-754 无穷语义控制]
        // 若任意参数为无穷大，必须精确同号同值才相等；无穷大不参与有限容差松弛比较
        const Bool a_inf = IsInfinity(a);
        const Bool b_inf = IsInfinity(b);
        if (a_inf || b_inf) {
            return a == b;
        }

        // 快速路径：处理完全相等的情况 (包括 +0.0 == -0.0 及相同有限值)
        if (a == b) {
            return true;
        }

        return Detail::FiniteAlmostEqual(a, b, absoluteTolerance, relativeTolerance);
    }

} // namespace vectoris::numerics::Traits

// Selective public spelling for the existing comparison; no wrapper or Detail import.
namespace vectoris::numerics::Core {
    using Traits::AlmostEqual;
}
