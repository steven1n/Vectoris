#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include "NumericTraits.h"
#include "Concepts.h"

// 依据 docs/ENGINEERING_STANDARD_V1.md Section 15 (IEEE-754 语义 baseline):
// bit-level 指数折半初值算法严格依赖 IEEE-754 binary32 与 binary64 物理内存布局。
static_assert(std::numeric_limits<float>::is_iec559, "[VectorisNumerics] float must conform to IEEE-754 binary32");
static_assert(sizeof(float) == 4, "[VectorisNumerics] sizeof(float) must be exactly 4 bytes");

static_assert(std::numeric_limits<double>::is_iec559, "[VectorisNumerics] double must conform to IEEE-754 binary64");
static_assert(sizeof(double) == 8, "[VectorisNumerics] sizeof(double) must be exactly 8 bytes");

namespace vectoris::numerics::Core {

    // 绝对值
    template<typename T>
    constexpr T abs(T v) noexcept {
        return v < T{} ? -v : v;
    }

    namespace Detail {
        template <Concepts::SupportedSqrtScalar T>
        constexpr T InitialSqrtGuess(T x) noexcept {
            if constexpr (std::is_same_v<T, double>) {
                if (x >= std::numeric_limits<double>::min()) {
                    const uint64_t bits = std::bit_cast<uint64_t>(x);
                    const uint64_t guess_bits = (bits >> 1) + (1023ULL << 51);
                    return std::bit_cast<double>(guess_bits);
                } else {
                    // Subnormal double: scale by 2^52
                    const double scaled = x * 4503599627370496.0;
                    const uint64_t bits = std::bit_cast<uint64_t>(scaled);
                    const uint64_t guess_bits = (bits >> 1) + (1023ULL << 51);
                    return std::bit_cast<double>(guess_bits) * 1.4901161193847656e-08; // 2^-26
                }
            } else {
                // float (binary32)
                if (x >= std::numeric_limits<float>::min()) {
                    const uint32_t bits = std::bit_cast<uint32_t>(x);
                    const uint32_t guess_bits = (bits >> 1) + (127U << 22);
                    return std::bit_cast<float>(guess_bits);
                } else {
                    // Subnormal float: scale by 2^24
                    const float scaled = x * 16777216.0f;
                    const uint32_t bits = std::bit_cast<uint32_t>(scaled);
                    const uint32_t guess_bits = (bits >> 1) + (127U << 22);
                    return std::bit_cast<float>(guess_bits) * 0.000244140625f; // 2^-12
                }
            }
        }

        template <Concepts::SupportedSqrtScalar T>
        constexpr T BoundedNewtonSqrt(T x, std::size_t* iterations_out = nullptr) noexcept {
            if (Traits::IsNaN(x)) {
                if (iterations_out != nullptr) {
                    *iterations_out = 0;
                }
                return Traits::NumericTraits<T>::quietNaN();
            }
            // Vectoris Core::sqrt project-specific domain policy:
            // 非正数（包括 -0.0、负有限数、-Inf）防御性截断返回 +0.0，避免在非实数域传播 NaN
            if (x <= T{}) {
                if (iterations_out != nullptr) {
                    *iterations_out = 0;
                }
                return T{};
            }
            if (Traits::IsInfinity(x)) {
                if (iterations_out != nullptr) {
                    *iterations_out = 0;
                }
                return x;
            }

            T curr = InitialSqrtGuess(x);
            constexpr std::size_t kMaxIterations = 64;
            constexpr T eps = Traits::NumericTraits<T>::epsilon();
            std::size_t iter_count = 0;

            for (std::size_t i = 0; i < kMaxIterations; ++i) {
                ++iter_count;
                const T prev = curr;
                curr = static_cast<T>(0.5) * (curr + x / curr);

                const T diff = abs(curr - prev);
                const T scale = curr > prev ? curr : prev;
                if (curr == prev || diff <= eps * scale) {
                    break;
                }
            }

            if (iterations_out != nullptr) {
                *iterations_out = iter_count;
            }
            return curr;
        }
    } // namespace Detail

    /**
     * @brief 平方根计算函数 (限定 float / double)
     *
     * @domain Vectoris Core::sqrt project-specific domain policy:
     * - x > 0: 计算并返回平方根
     * - x == 0: 返回 +0.0 (保留既有契约: sqrt(-0.0) -> +0.0)
     * - x < 0: 防御性截断返回 +0.0 (避免非实数域 NaN 扩散)
     * - +Inf: 返回 +Inf
     * - -Inf: 防御性截断返回 +0.0
     * - NaN: 返回 quiet_NaN
     *
     * @note 编译期求值在 constexpr 条件下调用具备硬迭代上限 (kMaxIterations=64) 的
     *       牛顿迭代实现 Detail::BoundedNewtonSqrt；
     *       运行时求值通过 std::is_constant_evaluated() 委派给标准库实现 std::sqrt。
     */
    template <Concepts::SupportedSqrtScalar T>
    constexpr T sqrt(T x) noexcept {
        if (std::is_constant_evaluated()) {
            return Detail::BoundedNewtonSqrt(x);
        } else {
            if (Traits::IsNaN(x)) {
                return Traits::NumericTraits<T>::quietNaN();
            }
            if (x <= T{}) {
                return T{};
            }
            return std::sqrt(x);
        }
    }

} // namespace vectoris::numerics::Core