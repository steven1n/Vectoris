#pragma once
#include <cstddef>
#include <limits>
#include "Concepts.h"
#include "Detail/ABI.h"
#include "Traits.h"
#include "Vector3.h"

namespace vectoris::numerics::Geometry {

    // [Phase 2.1] Matrix3 Core Engine
    // 纯数学 3x3 矩阵，无坐标系(Frame)约束，基于 Row-Major (行主序) 存储
    template <ScalarArithmetic T>
    struct Matrix3 final {
        // [GEO-REV-003] Row-major 连续内存存储 (MISRA & DMA 友好)
        T m[9];

        // [GEO-REV-002] 强制值初始化，避免不可预测的随机内存态
        constexpr Matrix3() noexcept : m{T{}, T{}, T{}, T{}, T{}, T{}, T{}, T{}, T{}} {}

        // 行主序显式构造
        constexpr Matrix3(T m00, T m01, T m02,
                          T m10, T m11, T m12,
                          T m20, T m21, T m22) noexcept
            : m{m00, m01, m02, m10, m11, m12, m20, m21, m22} {}

        // Factory: Zero Matrix
        static constexpr Matrix3 Zero() noexcept {
            return Matrix3();
        }

        // Factory: Identity Matrix
        static constexpr Matrix3 Identity() noexcept {
            return Matrix3(
                T{1}, T{0}, T{0},
                T{0}, T{1}, T{0},
                T{0}, T{0}, T{1}
            );
        }

        // Row-Major 索引访问 (0-indexed)
        constexpr T& operator()(size_t row, size_t col) noexcept {
            return m[row * 3 + col];
        }

        constexpr const T& operator()(size_t row, size_t col) const noexcept {
            return m[row * 3 + col];
        }

        // 矩阵加法
        constexpr Matrix3 operator+(const Matrix3& rhs) const noexcept {
            return Matrix3(
                m[0]+rhs.m[0], m[1]+rhs.m[1], m[2]+rhs.m[2],
                m[3]+rhs.m[3], m[4]+rhs.m[4], m[5]+rhs.m[5],
                m[6]+rhs.m[6], m[7]+rhs.m[7], m[8]+rhs.m[8]
            );
        }

        // 矩阵减法
        constexpr Matrix3 operator-(const Matrix3& rhs) const noexcept {
            return Matrix3(
                m[0]-rhs.m[0], m[1]-rhs.m[1], m[2]-rhs.m[2],
                m[3]-rhs.m[3], m[4]-rhs.m[4], m[5]-rhs.m[5],
                m[6]-rhs.m[6], m[7]-rhs.m[7], m[8]-rhs.m[8]
            );
        }

        // 标量乘法 (显式约束排他，防止 ScalarArithmetic 自指递归)
        template <typename S>
        requires (!std::same_as<std::remove_cvref_t<S>, Matrix3>) && ScalarArithmetic<S>
        constexpr auto operator*(const S& scalar) const noexcept {
            using ResT = decltype(m[0] * scalar);
            return Matrix3<ResT>(
                m[0]*scalar, m[1]*scalar, m[2]*scalar,
                m[3]*scalar, m[4]*scalar, m[5]*scalar,
                m[6]*scalar, m[7]*scalar, m[8]*scalar
            );
        }

        // 矩阵乘法 (Matrix * Matrix)
        constexpr Matrix3 operator*(const Matrix3& rhs) const noexcept {
            return Matrix3(
                m[0]*rhs.m[0] + m[1]*rhs.m[3] + m[2]*rhs.m[6],
                m[0]*rhs.m[1] + m[1]*rhs.m[4] + m[2]*rhs.m[7],
                m[0]*rhs.m[2] + m[1]*rhs.m[5] + m[2]*rhs.m[8],

                m[3]*rhs.m[0] + m[4]*rhs.m[3] + m[5]*rhs.m[6],
                m[3]*rhs.m[1] + m[4]*rhs.m[4] + m[5]*rhs.m[7],
                m[3]*rhs.m[2] + m[4]*rhs.m[5] + m[5]*rhs.m[8],

                m[6]*rhs.m[0] + m[7]*rhs.m[3] + m[8]*rhs.m[6],
                m[6]*rhs.m[1] + m[7]*rhs.m[4] + m[8]*rhs.m[7],
                m[6]*rhs.m[2] + m[7]*rhs.m[5] + m[8]*rhs.m[8]
            );
        }

        // 矩阵乘向量 (Matrix * Vector3) -> 保持 Frame 标签流转
        template <ScalarArithmetic U, FrameTag Frame>
        constexpr auto operator*(const Vector3<U, Frame>& v) const noexcept {
            using ResT = decltype(m[0] * v.x);
            return Vector3<ResT, Frame>{
                m[0]*v.x + m[1]*v.y + m[2]*v.z,
                m[3]*v.x + m[4]*v.y + m[5]*v.z,
                m[6]*v.x + m[7]*v.y + m[8]*v.z
            };
        }

        // 矩阵转置
        constexpr Matrix3 transposed() const noexcept {
            return Matrix3(
                m[0], m[3], m[6],
                m[1], m[4], m[7],
                m[2], m[5], m[8]
            );
        }

        // 行列式
        constexpr T det() const noexcept {
            return m[0] * (m[4]*m[8] - m[5]*m[7])
                 - m[1] * (m[3]*m[8] - m[5]*m[6])
                 + m[2] * (m[3]*m[7] - m[4]*m[6]);
        }

        // 计算 Frobenius 范数的平方: ||M||_F^2 = sum(m_i^2)
        constexpr T frobenius_norm_squared() const noexcept {
            T sum = T{0};
            for (size_t i = 0; i < 9; ++i) {
                sum += m[i] * m[i];
            }
            return sum;
        }

        // 伴随矩阵求逆 (无抛出原则, 失败返回 false)
        // 遵循 Solve-Not-Invert 原则：仅在需要显式矩阵逆时使用；解线性方程应使用消元求解器
        constexpr bool TryInverse(Matrix3& out) const noexcept {
            // 1. 评估矩阵元素最大模长尺度 (Scale / Infinity Norm Proxy)
            T max_val = T{0};
            for (size_t i = 0; i < 9; ++i) {
                T abs_val = (m[i] >= T{0}) ? m[i] : -m[i];
                if (abs_val > max_val) {
                    max_val = abs_val;
                }
            }

            // 零矩阵或非有限数值直接返回奇异
            if (max_val == T{0} || max_val != max_val || max_val > std::numeric_limits<T>::max()) {
                return false;
            }

            // 2. 尺度归一化元素 (Scale-Normalized Elements in [-1, 1])
            // 避免 scale^3 直接计算溢出 IEEE-754 指数范围 (如 double > 1e102 或 float > 1e12)
            T sm[9];
            for (size_t i = 0; i < 9; ++i) {
                sm[i] = m[i] / max_val;
            }

            // 3. 归一化行列式计算 (|det_tilde| <= 6，绝对不会发生指数上溢)
            T d_tilde = sm[0] * (sm[4]*sm[8] - sm[5]*sm[7])
                      - sm[1] * (sm[3]*sm[8] - sm[5]*sm[6])
                      + sm[2] * (sm[3]*sm[7] - sm[4]*sm[6]);

            if (d_tilde != d_tilde || d_tilde == T{0}) {
                return false;
            }

            // 4. 无量纲尺度奇异性阈值 (Scale-invariant singularity cutoff)
            T abs_d_tilde = (d_tilde >= T{0}) ? d_tilde : -d_tilde;
            T eps = std::numeric_limits<T>::epsilon();
            if (abs_d_tilde <= eps) {
                return false;
            }

            // 5. 伴随矩阵元素计算写入临时栈缓冲，杜绝原地自赋值踩踏 (Anti-Aliasing)
            // 逐项除以 (d_tilde * max_val) 保证全尺度数值稳定
            T inv[9];
            inv[0] =  ((sm[4]*sm[8] - sm[5]*sm[7]) / d_tilde) / max_val;
            inv[1] = -((sm[1]*sm[8] - sm[2]*sm[7]) / d_tilde) / max_val;
            inv[2] =  ((sm[1]*sm[5] - sm[2]*sm[4]) / d_tilde) / max_val;

            inv[3] = -((sm[3]*sm[8] - sm[5]*sm[6]) / d_tilde) / max_val;
            inv[4] =  ((sm[0]*sm[8] - sm[2]*sm[6]) / d_tilde) / max_val;
            inv[5] = -((sm[0]*sm[5] - sm[2]*sm[3]) / d_tilde) / max_val;

            inv[6] =  ((sm[3]*sm[7] - sm[4]*sm[6]) / d_tilde) / max_val;
            inv[7] = -((sm[0]*sm[7] - sm[1]*sm[6]) / d_tilde) / max_val;
            inv[8] =  ((sm[0]*sm[4] - sm[1]*sm[3]) / d_tilde) / max_val;

            // 6. 检查逆矩阵元素有限性
            for (size_t i = 0; i < 9; ++i) {
                if (inv[i] != inv[i] || inv[i] > std::numeric_limits<T>::max() || inv[i] < std::numeric_limits<T>::lowest()) {
                    return false;
                }
            }

            // 7. 写入输出对象 (安全拷贝，支持 &out == this)
            for (size_t i = 0; i < 9; ++i) {
                out.m[i] = inv[i];
            }

            return true;
        }

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const Matrix3& rhs) const noexcept {
            for (size_t i = 0; i < 9; ++i) {
                if (m[i] != rhs.m[i]) {
                    return false;
                }
            }
            return true;
        }

        constexpr bool operator!=(const Matrix3& rhs) const noexcept {
            return !(*this == rhs);
        }

        // 容差自适应数值近似判定
        [[nodiscard]] bool AlmostEqual(
            const Matrix3& rhs,
            T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
            T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
        ) const noexcept {
            for (size_t i = 0; i < 9; ++i) {
                if (!Traits::AlmostEqual(m[i], rhs.m[i], absoluteTolerance, relativeTolerance)) {
                    return false;
                }
            }
            return true;
        }
    };

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <ScalarArithmetic T>
    [[nodiscard]] inline bool AlmostEqual(
        const Matrix3<T>& a,
        const Matrix3<T>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        return a.AlmostEqual(b, absoluteTolerance, relativeTolerance);
    }

    // [GEO-REV-001] Geometry Layout Traits (不再直接暴露 ValueType)
    template<typename T>
    struct GeometryTraits<Matrix3<T>> {
        static constexpr size_t Elements = 9;
        using ScalarType = T;
    };

    // ABI Contract
    template<typename T>
    struct Matrix3ABIContract {
        using M = Matrix3<T>;
        
        static_assert(Detail::GeometryABIValidator<M>::value, 
            "Matrix3 failed base ABI.");
            
        static_assert(sizeof(M) == sizeof(T) * GeometryTraits<M>::Elements, 
            "Matrix3 size contains padding.");
            
        static constexpr bool value = true;
    };

} // namespace vectoris::numerics::Geometry