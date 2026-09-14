#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include "NumericTraits.h"
#include "Concepts.h"

namespace AegisMath::Core {

    // 绝对值
    template<typename T>
    constexpr T abs(T v) noexcept {
        return v < T{} ? -v : v;
    }

    namespace Detail {
        template <typename T>
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
            } else if constexpr (std::is_same_v<T, float>) {
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
            } else {
                return x >= T{1} ? x : T{1};
            }
        }

        template <typename T>
        constexpr T BoundedNewtonSqrt(T x, std::size_t* iterations_out = nullptr) noexcept {
            if (Traits::IsNaN(x)) {
                if (iterations_out != nullptr) {
                    *iterations_out = 0;
                }
                return Traits::NumericTraits<T>::quietNaN();
            }
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

    // 编译期平方根 (牛顿迭代法实现，具备硬上限与尺度感知收敛)
    template<typename T>
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

} // namespace AegisMath::Core