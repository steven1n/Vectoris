#pragma once
#include "Namespace.h"
#include <cmath>
#include <numbers>
#include "NumericTraits.h"
#include "Constants.h"
#include "Math.h"

namespace vectoris::numerics::Core::Math {

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

    // Compatibility spelling; all type/domain/constexpr semantics come from core::sqrt.
    template <Concepts::SupportedSqrtScalar T>
    [[nodiscard]] inline constexpr T sqrt(T value) noexcept {
        return ::vectoris::numerics::Core::sqrt(value);
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

    // 三角函数: 反余弦 (带边界浮点微小舍入截断，明显越界输入遵循 IEEE-754 返回 NaN)
    template <typename T>
    [[nodiscard]] inline T acos(T value) noexcept {
        constexpr T boundaryTolerance = Traits::NumericTraits<T>::epsilon() * T{10};

        if (value >= T{1}) {
            if (value <= T{1} + boundaryTolerance) {
                return T{0};
            }
            return std::acos(value); // IEEE-754: 明显越界输入 (value > 1 + tol) 返回 NaN
        }
        if (value <= -T{1}) {
            if (value >= -T{1} - boundaryTolerance) {
                return std::numbers::pi_v<T>;
            }
            return std::acos(value); // IEEE-754: 明显越界输入 (value < -1 - tol) 返回 NaN
        }
        return std::acos(value);
    }

} // namespace vectoris::numerics::Core::Math