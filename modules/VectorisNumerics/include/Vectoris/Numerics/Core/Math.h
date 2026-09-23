#pragma once
#include "Namespace.h"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include "NumericTraits.h"
#include "Concepts.h"

// 依据 docs/ENGINEERING_STANDARD_V1.md Section 15 (IEEE-754 语义 baseline):
// constexpr 平方根的整数有效数字算法依赖 IEEE-754 binary32/binary64 布局。
static_assert(std::numeric_limits<float>::is_iec559, "[VectorisNumerics] float must conform to IEEE-754 binary32");
static_assert(std::numeric_limits<float>::digits == 24 && std::numeric_limits<float>::max_exponent == 128);
static_assert(sizeof(float) == 4, "[VectorisNumerics] sizeof(float) must be exactly 4 bytes");

static_assert(std::numeric_limits<double>::is_iec559, "[VectorisNumerics] double must conform to IEEE-754 binary64");
static_assert(std::numeric_limits<double>::digits == 53 && std::numeric_limits<double>::max_exponent == 1024);
static_assert(sizeof(double) == 8, "[VectorisNumerics] sizeof(double) must be exactly 8 bytes");

namespace vectoris::numerics::Core {

    // 绝对值
    template<typename T>
    constexpr T abs(T v) noexcept {
        return v < T{} ? -v : v;
    }

    namespace Detail {
        // Extract a pair from M << shift without constructing the 106-bit radicand.
        // Preconditions: M has p<=53 bits; 0<=low<=2*(p-1), shift is p-1 or p.
        constexpr std::uint64_t SqrtRadicandPair(std::uint64_t significand, int low, int shift) noexcept {
            if (low >= shift) {
                return (significand >> (low - shift)) & 3U;
            }
            if (low + 1 == shift) {
                return (significand & 1U) << 1;
            }
            return 0;
        }

        // Precondition: x is positive and finite. Internal implementation only.
        // Exactly p digit steps; prefix = root^2 + remainder, remainder < 2*root+1.
        // All intermediates fit uint64_t: even the shifted remainder is < 2^55.
        template <typename T>
        requires (std::same_as<T, float> || std::same_as<T, double>)
        constexpr T DigitSqrtPositive(T x) noexcept {
            using UInt = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;
            constexpr int digits = std::numeric_limits<T>::digits;
            constexpr int bias = std::numeric_limits<T>::max_exponent - 1;
            const UInt bits = std::bit_cast<UInt>(x);
            const UInt raw_exponent = bits >> (digits - 1);
            std::uint64_t significand = bits & ((UInt{1} << (digits - 1)) - 1);
            int exponent = static_cast<int>(raw_exponent) - bias;
            if (raw_exponent == 0) {
                const int shift = std::countl_zero(significand) - (64 - digits);
                significand <<= shift;
                exponent = 1 - bias - shift;
            } else {
                significand |= std::uint64_t{1} << (digits - 1);
            }
            const int odd = exponent % 2 != 0 ? 1 : 0;
            std::uint64_t root = 0;
            std::uint64_t remainder = 0;
            constexpr int max_iterations = digits; // binary32: 24; binary64: 53.
            for (int step = 0; step < max_iterations; ++step) {
                remainder = (remainder << 2) |
                    SqrtRadicandPair(significand, 2 * (digits - step - 1), digits - 1 + odd);
                const std::uint64_t trial = (root << 2) | 1U;
                root <<= 1;
                if (remainder >= trial) {
                    remainder -= trial;
                    ++root;
                }
            }
            // Nearest rounding: N > (root+1/2)^2 iff integer remainder > root.
            // An exact halfway case is impossible because N is an integer.
            if (remainder > root) {
                ++root;
            }
            // Scale exponents: [-98,40] for float, [-589,459] for double.
            // The encoded scale and every positive result are normal and finite.
            const int scale_exponent = (exponent - odd) / 2 - (digits - 1);
            const UInt scale_bits = static_cast<UInt>(scale_exponent + bias) << (digits - 1);
            return static_cast<T>(root) * std::bit_cast<T>(scale_bits);
        }
    } // namespace Detail

    /**
     * Canonical scalar square root. float/double: constexpr and runtime;
     * long double: runtime support (no portable C++20 constexpr guarantee).
     * Preserve signed zero and +Inf; negative nonzero inputs and NaNs yield NaN.
     * Runtime positive inputs use std::sqrt. Constant float/double evaluation
     * extracts exactly 24/53 binary digits and rounds to nearest, with no
     * convergence exit or fallback. See docs/core.md for proof and non-guarantees.
     */
    template <Concepts::SupportedSqrtScalar T>
    [[nodiscard]] constexpr T sqrt(T x) noexcept {
        if (Traits::IsNaN(x) || x < T{0}) {
            return Traits::NumericTraits<T>::quietNaN();
        }
        // Exact zero classification preserves -0; this is not approximate comparison.
        if (x == T{0} || Traits::IsInfinity(x)) {
            return x;
        }
        if constexpr (std::same_as<T, float> || std::same_as<T, double>) {
            if (std::is_constant_evaluated()) {
                return Detail::DigitSqrtPositive(x);
            }
        }
        return std::sqrt(x);
    }

} // namespace vectoris::numerics::Core
