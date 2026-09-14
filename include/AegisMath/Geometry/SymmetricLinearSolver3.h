#pragma once

#include <cstddef>
#include <limits>
#include "AegisMath/Core/Concepts.h"
#include "AegisMath/Core/MathError.h"
#include "AegisMath/Core/Result.h"
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Geometry/Concepts.h"
#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Geometry/Matrix3.h"
#include "AegisMath/Geometry/Vector3.h"

namespace AegisMath::Geometry {

    namespace Detail {
        template <Concepts::FloatingPoint T>
        [[nodiscard]] constexpr T ConstexprAbs(T val) noexcept {
            return (val >= T{0}) ? val : -val;
        }

        template <Concepts::FloatingPoint T>
        [[nodiscard]] constexpr T ConstexprMax(T a, T b) noexcept {
            return (a >= b) ? a : b;
        }

        template <Concepts::FloatingPoint T>
        [[nodiscard]] constexpr T ConstexprMin(T a, T b) noexcept {
            return (a <= b) ? a : b;
        }
    } // namespace Detail

    /**
     * @brief 求解 3x3 对称正定线性方程组: A * x = b
     * 
     * 遵循 Solve-Not-Invert 原则 (Engineering Standard v1.0 Section 18)。
     * 针对 3x3 对称正定线性系统设计，采用确定性无选主元的解析 LDL^T 分解 (A = L * D * L^T)
     * 与两层数值可靠性策略：
     *   - Layer A (结构与正定性判定): 严格检测输入有限性 (isfinite)、矩阵对称性 (|A_ij - A_ji| <= tol_sym)
     *     以及主元正定性 (d_k > tol_sing)。
     *   - Layer B (解算质量与反向误差控制):
     *       1. LDLT pivot-spread safeguard (主元跨度启发式防线): min(d) / max(d) <= eps * 100 即判定病态。
     *       2. Normwise relative backward error (无穷范数相对反向误差) 验证:
     *          ||r||_inf / (||A||_inf * ||x||_inf + ||b||_inf) <= 100 * eps。
     *   - 零平方根计算，保证 ISO C++20 纯 constexpr 语义。
     *   - 零堆动态分配，所有计算在寄存器/栈上就地完成。
     *
     * @note ill_conditioned 是数值安全诊断标识：当配置的 LDLT 主元扩展比率安全门限 (pivot-spread safeguard)
     *       或无穷范数相对反向误差验收准则 (normwise relative backward error acceptance criterion) 被突破时触发，
     *       并不代表显式计算了矩阵的精确条件数 kappa(A)。
     *
     * @tparam T 浮点精度类型 (float, double)
     * @tparam Frame 空间坐标系标签
     * @param A 3x3 对称正定矩阵
     * @param b 3x1 目标向量 (保留 FrameTag 坐标系语义)
     * @return Core::Result<Vector3<T, Frame>, Core::MathError> 解向量或明确数学错误码
     */
    template <Concepts::FloatingPoint T, FrameTag Frame = FrameUnknown>
    [[nodiscard]] constexpr Core::Result<Vector3<T, Frame>, Core::MathError>
    SolveSymmetricPositiveDefinite3x3(const Matrix3<T>& A, const Vector3<T, Frame>& b) noexcept {
        using ResultType = Core::Result<Vector3<T, Frame>, Core::MathError>;

        // 1. 输入有限性检查 (IEEE-754 防御)
        for (size_t i = 0; i < 9; ++i) {
            if (!Traits::IsFinite(A.m[i])) {
                return ResultType(Core::MathError::non_finite_input);
            }
        }
        if (!Traits::IsFinite(b.x) || !Traits::IsFinite(b.y) || !Traits::IsFinite(b.z)) {
            return ResultType(Core::MathError::non_finite_input);
        }

        // 2. 计算矩阵元素尺度 (Infinity Norm Proxy)
        T scale = T{0};
        for (size_t i = 0; i < 9; ++i) {
            scale = Detail::ConstexprMax(scale, Detail::ConstexprAbs(A.m[i]));
        }
        if (scale == T{0}) {
            return ResultType(Core::MathError::singular_matrix);
        }

        const T eps = Traits::NumericTraits<T>::epsilon();

        // 3. 对称性校验 (相对尺度敏感)
        const T tol_sym = scale * eps * static_cast<T>(100.0);
        if (Detail::ConstexprAbs(A(0, 1) - A(1, 0)) > tol_sym ||
            Detail::ConstexprAbs(A(0, 2) - A(2, 0)) > tol_sym ||
            Detail::ConstexprAbs(A(1, 2) - A(2, 1)) > tol_sym) {
            return ResultType(Core::MathError::invalid_argument);
        }

        // 4. LDL^T 解析分解与主元正定性/奇异性判定
        const T tol_sing = scale * eps * static_cast<T>(10.0);

        // 主元 1
        const T d1 = A(0, 0);
        if (d1 < -tol_sing) {
            return ResultType(Core::MathError::invalid_state); // 不定矩阵或负定
        }
        if (d1 <= tol_sing) {
            return ResultType(Core::MathError::singular_matrix);
        }

        const T l21 = A(1, 0) / d1;
        const T l31 = A(2, 0) / d1;

        // 主元 2
        const T d2 = A(1, 1) - l21 * A(1, 0);
        if (d2 < -tol_sing) {
            return ResultType(Core::MathError::invalid_state); // 不定矩阵
        }
        if (d2 <= tol_sing) {
            return ResultType(Core::MathError::singular_matrix);
        }

        const T l32 = (A(2, 1) - l31 * A(1, 0)) / d2;

        // 主元 3
        const T d3 = A(2, 2) - l31 * A(2, 0) - l32 * (A(2, 1) - l31 * A(1, 0));
        if (d3 < -tol_sing) {
            return ResultType(Core::MathError::invalid_state); // 不定矩阵
        }
        if (d3 <= tol_sing) {
            return ResultType(Core::MathError::singular_matrix);
        }

        // 5. LDLT 主元跨度启发式防线 (Pivot-Spread Safeguard)
        // 浮点与双精度自适应：比值接近机器浮点噪声下限即判定为病态
        const T min_d = Detail::ConstexprMin(d1, Detail::ConstexprMin(d2, d3));
        const T max_d = Detail::ConstexprMax(d1, Detail::ConstexprMax(d2, d3));
        const T tol_pivot_spread = eps * static_cast<T>(100.0);
        if (min_d / max_d <= tol_pivot_spread) {
            return ResultType(Core::MathError::ill_conditioned);
        }

        // 6. 前向代换: L * y = b
        const T y0 = b.x;
        const T y1 = b.y - l21 * y0;
        const T y2 = b.z - l31 * y0 - l32 * y1;

        // 7. 对角解算: D * z = y
        const T z0 = y0 / d1;
        const T z1 = y1 / d2;
        const T z2 = y2 / d3;

        // 8. 后向代换: L^T * x = z
        const T x2 = z2;
        const T x1 = z1 - l32 * x2;
        const T x0 = z0 - l21 * x1 - l31 * x2;

        if (!Traits::IsFinite(x0) || !Traits::IsFinite(x1) || !Traits::IsFinite(x2)) {
            return ResultType(Core::MathError::ill_conditioned);
        }

        // 9. 范数相对反向误差与残差控制 (Normwise relative backward error)
        // eta = ||r||_inf / (||A||_inf * ||x||_inf + ||b||_inf)
        // r = A*x - b
        const T r0 = A(0, 0)*x0 + A(0, 1)*x1 + A(0, 2)*x2 - b.x;
        const T r1 = A(1, 0)*x0 + A(1, 1)*x1 + A(1, 2)*x2 - b.y;
        const T r2 = A(2, 0)*x0 + A(2, 1)*x1 + A(2, 2)*x2 - b.z;

        const T r_inf = Detail::ConstexprMax(Detail::ConstexprAbs(r0),
            Detail::ConstexprMax(Detail::ConstexprAbs(r1), Detail::ConstexprAbs(r2)));

        const T row0_sum = Detail::ConstexprAbs(A(0, 0)) + Detail::ConstexprAbs(A(0, 1)) + Detail::ConstexprAbs(A(0, 2));
        const T row1_sum = Detail::ConstexprAbs(A(1, 0)) + Detail::ConstexprAbs(A(1, 1)) + Detail::ConstexprAbs(A(1, 2));
        const T row2_sum = Detail::ConstexprAbs(A(2, 0)) + Detail::ConstexprAbs(A(2, 1)) + Detail::ConstexprAbs(A(2, 2));
        const T A_inf = Detail::ConstexprMax(row0_sum, Detail::ConstexprMax(row1_sum, row2_sum));

        const T x_inf = Detail::ConstexprMax(Detail::ConstexprAbs(x0),
            Detail::ConstexprMax(Detail::ConstexprAbs(x1), Detail::ConstexprAbs(x2)));
        const T b_inf = Detail::ConstexprMax(Detail::ConstexprAbs(b.x),
            Detail::ConstexprMax(Detail::ConstexprAbs(b.y), Detail::ConstexprAbs(b.z)));

        const T denom = A_inf * x_inf + b_inf;
        if (denom > T{0}) {
            const T eta = r_inf / denom;
            constexpr T kBackwardErrorBound = static_cast<T>(100.0);
            if (eta > kBackwardErrorBound * eps) {
                return ResultType(Core::MathError::ill_conditioned);
            }
        }

        return ResultType::success(Vector3<T, Frame>(x0, x1, x2));
    }

} // namespace AegisMath::Geometry
