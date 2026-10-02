#pragma once
#include "Namespace.h"
#include <type_traits>
#include "Concepts.h"
#include "FrameTags.h"
#include "Matrix3.h"
#include "Vector3.h"
#include "Detail/RotationInvariant.h"
#include "Detail/ABI.h"
#include "../Core/Result.h"

namespace vectoris::numerics::Geometry {

    // [Phase 2.3] Direction Cosine Matrix (DCM) Core Engine
    // 强制绑定 FrameFrom -> FrameTo，防范坐标系混用灾难
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    struct RotationMatrix3 final {
    public:
        template <ScalarArithmetic, FrameTag, FrameTag>
        friend struct RotationMatrix3;

        template <ScalarArithmetic, FrameTag, FrameTag>
        friend struct Quaternion;

    private:
        Matrix3<T> dcm_;

        // 私有构造，封锁绕过正交性检查的非法实例
        constexpr explicit RotationMatrix3(const Matrix3<T>& raw_matrix) noexcept(std::is_arithmetic_v<T>)
            : dcm_(raw_matrix) {}

    public:
        // --- 核心防线：禁止未定义状态 ---
        RotationMatrix3() = delete;

        // --- 工厂方法 ---
        // 1. 恒等映射 (通常用于同 Frame 初始化，或默认无旋转状态)
        static constexpr RotationMatrix3 Identity() noexcept(std::is_arithmetic_v<T>) {
            return RotationMatrix3(Matrix3<T>::Identity());
        }

        // 2. 安全构建 (执行正交性和行列式检查)
        static Core::Result<RotationMatrix3> TryCreate(const Matrix3<T>& raw_matrix) noexcept(std::is_arithmetic_v<T>) {
            for (size_t i = 0; i < 9; ++i) {
                if (!Traits::IsFinite(raw_matrix.m[i])) {
                    return Core::Result<RotationMatrix3>::failure(Core::MathError::non_finite_input);
                }
            }
            if (!Detail::CheckRotationInvariants(raw_matrix)) {
                return Core::Result<RotationMatrix3>::failure(Core::MathError::invalid_state);
            }
            return Core::Result<RotationMatrix3>::success(RotationMatrix3(raw_matrix));
        }

        static constexpr bool TryCreate(const Matrix3<T>& raw_matrix, RotationMatrix3& out) noexcept(std::is_arithmetic_v<T>) {
            auto res = TryCreate(raw_matrix);
            if (!res.IsSuccess()) {
                return false;
            }
            out = res.Value();
            return true;
        }

        // --- 坐标系映射代数 (Frame Mapping Algebra) ---

        // 1. 向量变换: Rotation<A, B> * Vector3<A> -> Vector3<B>
        template <ScalarArithmetic U>
        constexpr auto operator*(const Vector3<U, FrameFrom>& v) const noexcept(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            using ResT = decltype(dcm_(0,0) * v.x);
            // 内部解包数学矩阵，运算后重新封装至目标 Frame
            return Vector3<ResT, FrameTo>{
                dcm_(0,0)*v.x + dcm_(0,1)*v.y + dcm_(0,2)*v.z,
                dcm_(1,0)*v.x + dcm_(1,1)*v.y + dcm_(1,2)*v.z,
                dcm_(2,0)*v.x + dcm_(2,1)*v.y + dcm_(2,2)*v.z
            };
        }

        // 拦截跨坐标系非法向量乘法
        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, FrameFrom>)
        constexpr auto operator*(const Vector3<U, OtherFrame>&) const = delete;

        // 2. 旋转级联: Rotation<A, B> * Rotation<B, C> -> Rotation<A, C>
        // 遵循 pipeline 级联定义: (R_AB * R_BC) * v_A = R_BC * (R_AB * v_A) = M_BC * (M_AB * v_A)
        // 对应底层矩阵乘法: M_AC = M_BC * M_AB (即 rhs.ToMatrix() * dcm_)
        template <FrameTag FrameNext>
        constexpr auto operator*(const RotationMatrix3<T, FrameTo, FrameNext>& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return RotationMatrix3<T, FrameFrom, FrameNext>(rhs.ToMatrix() * dcm_);
        }

        // 3. 非法级联拦截
        // 故意阻断 Rotation<A, B> * Rotation<C, D>
        template <FrameTag OtherFrom, FrameTag OtherTo>
        constexpr auto operator*(const RotationMatrix3<T, OtherFrom, OtherTo>&) const = delete;

        // --- 逆运算 (无开销) ---
        // 正交矩阵的逆即为其转置。物理意义: R_A->B 的逆即为 R_B->A
        constexpr RotationMatrix3<T, FrameTo, FrameFrom> Transposed() const noexcept(std::is_arithmetic_v<T>) {
            return RotationMatrix3<T, FrameTo, FrameFrom>(dcm_.transposed());
        }
        
        constexpr RotationMatrix3<T, FrameTo, FrameFrom> Inverse() const noexcept(std::is_arithmetic_v<T>) {
            return Transposed();
        }

        // --- 四元数转换构造工厂 ---
        template <typename QuatType>
        [[nodiscard]] static constexpr Core::Result<RotationMatrix3> FromQuaternion(const QuatType& q) noexcept(noexcept(Core::Result<RotationMatrix3>(q.ToRotationMatrix()))) {
            return q.ToRotationMatrix();
        }

        // --- 数据提取 ---
        constexpr const Matrix3<T>& ToMatrix() const noexcept {
            return dcm_;
        }

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const RotationMatrix3& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return dcm_ == rhs.dcm_;
        }

        constexpr bool operator!=(const RotationMatrix3& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return !(*this == rhs);
        }
    };

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <Concepts::FloatingPoint T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool AlmostEqual(
        const RotationMatrix3<T, FrameFrom, FrameTo>& a,
        const RotationMatrix3<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept(std::is_arithmetic_v<T>) {
        return a.ToMatrix().AlmostEqual(b.ToMatrix(), absoluteTolerance, relativeTolerance);
    }

    // --- Geometry Traits 与 ABI 联合注册 ---
    template<typename T, FrameTag FrameFrom, FrameTag FrameTo>
    struct GeometryTraits<RotationMatrix3<T, FrameFrom, FrameTo>> {
        static constexpr size_t Elements = 9;
        using ScalarType = T;
        using SourceFrame = FrameFrom;
        using TargetFrame = FrameTo;

        static_assert(Detail::GeometryABIValidator<RotationMatrix3<T, FrameFrom, FrameTo>>::value, 
            "RotationMatrix3 failed source layout constraints.");
            
        static_assert(sizeof(RotationMatrix3<T, FrameFrom, FrameTo>) == sizeof(T) * 9, 
            "RotationMatrix3 source representation must contain nine scalar slots.");
    };

} // namespace vectoris::numerics::Geometry
