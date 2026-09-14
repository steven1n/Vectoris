#pragma once
#include <numbers>
#include "Concepts.h"
#include "Precision.h"

namespace AegisMath::Constants {

    // =========================================================================
    // 纯数学常数 (Pure Mathematical Constants)
    // =========================================================================

    // 圆周率 (Pi)
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T Pi = std::numbers::pi_v<T>;

    // 2 * Pi (360度)
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T TwoPi = Pi<T> * static_cast<T>(2.0);

    // Pi / 2 (90度)
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T HalfPi = Pi<T> / static_cast<T>(2.0);

    // Pi / 4 (45度)
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T Pi_4 = Pi<T> / static_cast<T>(4.0);

    // 根号 2
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T Sqrt2 = std::numbers::sqrt2_v<T>;

    // 自然对数底数 e
    template <Concepts::FloatingPoint T = Real>
    inline constexpr T E = std::numbers::e_v<T>;

    // =========================================================================
    // 角度转换乘子系数 (编译期常量折叠)
    // =========================================================================

    template <Concepts::FloatingPoint T = Real>
    inline constexpr T DegToRadMult = Pi<T> / static_cast<T>(180.0);

    template <Concepts::FloatingPoint T = Real>
    inline constexpr T RadToDegMult = static_cast<T>(180.0) / Pi<T>;

} // namespace AegisMath::Constants