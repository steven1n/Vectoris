#pragma once
#include "Concepts.h"
#include "FrameTags.h"
#include "Matrix3.h"
#include "Vector3.h"
#include "Detail/RotationInvariant.h"
#include "Detail/ABI.h"

namespace AegisMath::Geometry {

    // [Phase 2.3] Direction Cosine Matrix (DCM) Core Engine
    // 强制绑定 FrameFrom -> FrameTo，防范坐标系混用灾难
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    struct RotationMatrix3 final {
    private:
        Matrix3<T> dcm_;

        // 私有构造，封锁绕过正交性检查的非法实例
        constexpr explicit RotationMatrix3(const Matrix3<T>& raw_matrix) noexcept 
            : dcm_(raw_matrix) {}

    public:
        // --- 核心防线：禁止未定义状态 ---
        RotationMatrix3() = delete;

        // --- 工厂方法 ---
        // 1. 恒等映射 (通常用于同 Frame 初始化，或默认无旋转状态)
        static constexpr RotationMatrix3 Identity() noexcept {
            return RotationMatrix3(Matrix3<T>::Identity());
        }

        // 2. 安全构建 (执行正交性和行列式检查)
        static constexpr bool TryCreate(const Matrix3<T>& raw_matrix, RotationMatrix3& out) noexcept {
            if (!Detail::CheckRotationInvariants(raw_matrix)) {
                return false; // 矩阵畸变，非合法旋转
            }
            out = RotationMatrix3(raw_matrix);
            return true;
        }

        // --- 坐标系映射代数 (Frame Mapping Algebra) ---

        // 1. 向量变换: Rotation<A, B> * Vector3<A> -> Vector3<B>
        template <ScalarArithmetic U>
        constexpr auto operator*(const Vector3<U, FrameFrom>& v) const noexcept {
            using ResT = decltype(dcm_(0,0) * v.x);
            // 内部解包数学矩阵，运算后重新封装至目标 Frame
            return Vector3<ResT, FrameTo>{
                dcm_(0,0)*v.x + dcm_(0,1)*v.y + dcm_(0,2)*v.z,
                dcm_(1,0)*v.x + dcm_(1,1)*v.y + dcm_(1,2)*v.z,
                dcm_(2,0)*v.x + dcm_(2,1)*v.y + dcm_(2,2)*v.z
            };
        }

        // 2. 旋转级联: Rotation<A, B> * Rotation<B, C> -> Rotation<A, C>
        template <FrameTag FrameNext>
        constexpr auto operator*(const RotationMatrix3<T, FrameTo, FrameNext>& rhs) const noexcept {
            // 右乘级联 (取决于具体 GNC 定义，此处依循 R_AC = R_AB * R_BC 规范)
            return RotationMatrix3<T, FrameFrom, FrameNext>(dcm_ * rhs.ToMatrix());
        }

        // 3. 非法级联拦截
        // 故意阻断 Rotation<A, B> * Rotation<C, D>
        template <FrameTag OtherFrom, FrameTag OtherTo>
        constexpr auto operator*(const RotationMatrix3<T, OtherFrom, OtherTo>&) const = delete;

        // --- 逆运算 (无开销) ---
        // 正交矩阵的逆即为其转置。物理意义: R_A->B 的逆即为 R_B->A
        constexpr RotationMatrix3<T, FrameTo, FrameFrom> Transposed() const noexcept {
            return RotationMatrix3<T, FrameTo, FrameFrom>(dcm_.transposed());
        }
        
        constexpr RotationMatrix3<T, FrameTo, FrameFrom> Inverse() const noexcept {
            return Transposed();
        }

        // --- 数据提取 ---
        constexpr const Matrix3<T>& ToMatrix() const noexcept {
            return dcm_;
        }
    };

    // --- Geometry Traits 与 ABI 联合注册 ---
    template<typename T, FrameTag FrameFrom, FrameTag FrameTo>
    struct GeometryTraits<RotationMatrix3<T, FrameFrom, FrameTo>> {
        static constexpr size_t Elements = 9;
        using ScalarType = T;
        using SourceFrame = FrameFrom;
        using TargetFrame = FrameTo;

        static_assert(Detail::GeometryABIValidator<RotationMatrix3<T, FrameFrom, FrameTo>>::value, 
            "RotationMatrix3 failed base ABI constraints.");
            
        static_assert(sizeof(RotationMatrix3<T, FrameFrom, FrameTo>) == sizeof(T) * 9, 
            "RotationMatrix3 must have zero padding overhead relative to Matrix3.");
    };

} // namespace AegisMath::Geometry