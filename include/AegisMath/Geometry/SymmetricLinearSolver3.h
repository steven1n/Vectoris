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
     * 采用无平方根的解析 LDL^T 分解 (A = L * D * L^T):
     *   - 无需计算平方根，保证 ISO C++20 纯 constexpr 语义与高执行效率。
     *   - 零堆动态分配，所有计算均在寄存器/栈上就地完成。
     *   - 严格具备尺度敏感的对称性检查、奇异性判定、负定/不定性诊断以及条件数超限诊断。
     * 
     * @tparam T 浮点精度类型 (float, double, long double)
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

        // 5. 条件数估算 (主元极值比)
        const T min_d = Detail::ConstexprMin(d1, Detail::ConstexprMin(d2, d3));
        const T max_d = Detail::ConstexprMax(d1, Detail::ConstexprMax(d2, d3));
        const T tol_ill = Detail::ConstexprMax(eps * static_cast<T>(100000.0), static_cast<T>(1e-10));
        if (min_d / max_d <= tol_ill) {
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

        return ResultType::success(Vector3<T, Frame>(x0, x1, x2));
    }

} // namespace AegisMath::Geometry
