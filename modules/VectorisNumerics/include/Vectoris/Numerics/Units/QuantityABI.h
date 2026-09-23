#pragma once
#include "Namespace.h"
#include <type_traits>

namespace vectoris::numerics::Units {

    template <typename Q>
    struct QuantityABIValidator {
        static constexpr bool Validate() {
            static_assert(
                sizeof(Q) == sizeof(typename Q::ValueType),
                "Quantity ABI violation: Size mismatch with underlying scalar!"
            );
            static_assert(
                alignof(Q) == alignof(typename Q::ValueType),
                "Quantity ABI violation: Alignment mismatch with underlying scalar!"
            );
            static_assert(
                std::is_trivially_copyable_v<Q>,
                "Quantity ABI violation: Type must be trivially copyable!"
            );
            return true;
        }
    };

    // 显式注册与编译期断言锁存器
    template <typename Q>
    inline constexpr bool QuantityABIRegistration = QuantityABIValidator<Q>::Validate();

} // namespace vectoris::numerics::Units