#pragma once
#include <cstdint>
#include <cstddef>
#include "Precision.h"

namespace AegisMath {

    // 严格定宽有符号整数
    using Int8   = std::int8_t;
    using Int16  = std::int16_t;
    using Int32  = std::int32_t;
    using Int64  = std::int64_t;
    
    // 严格定宽无符号整数
    using UInt8  = std::uint8_t;
    using UInt16 = std::uint16_t;
    using UInt32 = std::uint32_t;
    using UInt64 = std::uint64_t;

    // 明确的 IEEE-754 定宽浮点别名 (用于与协议、GPU 或特定硬件交互)
    using Float32 = float;
    using Float64 = double;

    // 内存与维度索引
    using Index  = std::size_t;

    // 强语义布尔类型
    using Bool   = bool;

} // namespace AegisMath