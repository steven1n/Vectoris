#pragma once
#include <limits>
#include "Compiler.h"

namespace AegisMath {

    // =========================================================================
    // 仿真精度控制器
    // =========================================================================
#if defined(AEGIS_REAL_FLOAT32)
    using Real = float;
#elif defined(AEGIS_REAL_EXTENDED)
    using Real = long double;
#elif defined(AEGIS_REAL_FLOAT64)
    using Real = double;
#else
    // 默认行为兜底为 64-bit
    using Real = double;
#endif

    // 为上层代数结构提供标准化泛型别名
    using Scalar = Real;

    // =========================================================================
    // 硬件确定性验证
    // =========================================================================
    static_assert(std::numeric_limits<Real>::is_iec559,
        "[AegisMath] FATAL: Platform floating-point representation does not conform to IEEE 754 (IEC 559).");

    static_assert(sizeof(Real) >= 4, "[AegisMath] FATAL: Real type must be at least 32-bit width.");

} // namespace AegisMath