#pragma once
#include <ratio>
#include <limits>
#include <cstdint>

namespace vectoris::numerics::Units::Detail {

    constexpr std::intmax_t SafeAbs(std::intmax_t v) {
        return v < 0 ? -v : v;
    }

    template<typename R1, typename R2>
    struct SafeRatioMultiply {
        // 拦截分子溢出
        static_assert(
            R1::num == 0 || R2::num == 0 ||
            std::numeric_limits<std::intmax_t>::max() / SafeAbs(R1::num) >= SafeAbs(R2::num),
            "SafeRatioMultiply: Numerator overflow detected!"
        );
        // 拦截分母溢出
        static_assert(
            std::numeric_limits<std::intmax_t>::max() / SafeAbs(R1::den) >= SafeAbs(R2::den),
            "SafeRatioMultiply: Denominator overflow detected!"
        );

        using type = std::ratio_multiply<R1, R2>;
    };

    // (同理可根据需要补齐 SafeRatioDivide)

} // namespace vectoris::numerics::Units::Detail