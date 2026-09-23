#pragma once
#include "Namespace.h"
#include <limits>
#include "Compiler.h"

namespace vectoris::numerics {

    // =========================================================================
    // 仿真精度控制器
    // =========================================================================
#if defined(VECTORIS_REAL_FLOAT32)
    using Real = float;
#elif defined(VECTORIS_REAL_EXTENDED)
    using Real = long double;
#elif defined(VECTORIS_REAL_FLOAT64)
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
        "[VectorisNumerics] FATAL: Platform floating-point representation does not conform to IEEE 754 (IEC 559).");

    static_assert(sizeof(Real) >= 4, "[VectorisNumerics] FATAL: Real type must be at least 32-bit width.");

} // namespace vectoris::numerics