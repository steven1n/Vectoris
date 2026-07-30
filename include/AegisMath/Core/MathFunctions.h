#pragma once
#include <cmath>
#include "NumericTraits.h"

namespace AegisMath::Core::Math {

    // =========================================================================
    // 基础数学函数隔离层 (Math Function Isolation Layer)
    // 目的: 将 Geometry / GNC 层与具体的平台数学实现 (<cmath>, DSP, FPGA) 解耦
    // =========================================================================

    // 绝对值
    template <typename T>
    [[nodiscard]] inline constexpr T abs(T value) noexcept {
        // 对于浮点数，在 C++20 中 std::abs 尚非完全 constexpr
        // 这里提供一个简单的安全分支，支持编译期计算
        return (value >= T{0}) ? value : -value;
    }

    // 平方根
    template <typename T>
    [[nodiscard]] inline T sqrt(T value) noexcept {
        // 防御性拦截：负数开方在实数域无意义，返回 0 以避免 NaN 传播
        if (value <= T{0}) {
            return T{0};
        }
        return std::sqrt(value);
    }

    // 三角函数: 正弦
    template <typename T>
    [[nodiscard]] inline T sin(T value) noexcept {
        return std::sin(value);
    }

    // 三角函数: 余弦
    template <typename T>
    [[nodiscard]] inline T cos(T value) noexcept {
        return std::cos(value);
    }

    // 三角函数: 反余弦 (带定义域安全截断，防止浮点误差导致 domain error)
    template <typename T>
    [[nodiscard]] inline T acos(T value) noexcept {
        if (value >= T{1}) return T{0};
        if (value <= -T{1}) return Traits::NumericTraits<T>::max(); // 或返回 std::numbers::pi_v<T>，视项目常量定义而定
        return std::acos(value);
    }

} // namespace AegisMath::Core::Math