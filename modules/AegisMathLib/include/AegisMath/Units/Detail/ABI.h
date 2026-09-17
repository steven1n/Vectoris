#pragma once
#include <type_traits>

namespace AegisMath::Units::Detail {

    // 鸭子类型（Duck Typing）要求 Q 提供 ValueType 即可，无需知道 Quantity 的完整定义
    template <typename Q>
    struct QuantityABIValidator {
        static constexpr bool Validate() {
            return std::is_standard_layout_v<Q> &&           // 保证内存布局 (DMA/序列化安全)
                   std::is_trivially_copyable_v<Q> &&        // 保证可以通过 memcpy 拷贝
                   sizeof(Q) == sizeof(typename Q::ValueType); // 保证无额外开销 (Zero-overhead)
        }
    };

    // 语义更正：Contract 而不是 Registration
    template <typename Q>
    inline constexpr bool QuantityABIContract = QuantityABIValidator<Q>::Validate();

    // 供外部主动触发校验的友好接口
    template <typename Q>
    constexpr bool ValidateQuantityABI() {
        return QuantityABIContract<Q>;
    }

} // namespace AegisMath::Units::Detail