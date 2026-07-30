#pragma once
#include <cstddef>
#include "AegisMath/Dynamics/Concepts.h"
#include "Detail/ABI.h"
#include "Traits.h"
#include "Vector3.h"

namespace AegisMath::Geometry {

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

        // 标量乘法
        template <ScalarArithmetic S>
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

        // 伴随矩阵求逆 (无抛出原则, 失败返回 false)
        constexpr bool TryInverse(Matrix3& out) const noexcept {
            T d = det();
            // 奇异矩阵判定
            if (d == T{}) {
                return false; 
            }

            out.m[0] =  (m[4]*m[8] - m[5]*m[7]) / d;
            out.m[1] = -(m[1]*m[8] - m[2]*m[7]) / d;
            out.m[2] =  (m[1]*m[5] - m[2]*m[4]) / d;

            out.m[3] = -(m[3]*m[8] - m[5]*m[6]) / d;
            out.m[4] =  (m[0]*m[8] - m[2]*m[6]) / d;
            out.m[5] = -(m[0]*m[5] - m[2]*m[3]) / d;

            out.m[6] =  (m[3]*m[7] - m[4]*m[6]) / d;
            out.m[7] = -(m[0]*m[7] - m[2]*m[6]) / d;
            out.m[8] =  (m[0]*m[4] - m[1]*m[3]) / d;

            return true;
        }
    };

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

} // namespace AegisMath::Geometry