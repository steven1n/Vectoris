#pragma once
#include "NumericTraits.h"

namespace AegisMath::Core {
    
    // 绝对值
    template<typename T>
    constexpr T abs(T v) noexcept {
        return v < T{} ? -v : v;
    }

    // 编译期平方根 (牛顿迭代法实现，此处作简要声明)
    // 实际工程中需确保满足 MISRA C++ 标准且支持 constexpr
    template<typename T>
    constexpr T sqrt(T x) noexcept {
        if (x <= T{}) return T{};
        T curr = x;
        T prev = T{};
        while (abs(curr - prev) > Traits::NumericTraits<T>::epsilon()) {
            prev = curr;
            curr = static_cast<T>(0.5) * (curr + x / curr);
        }
        return curr;
    }

} // namespace AegisMath::Core