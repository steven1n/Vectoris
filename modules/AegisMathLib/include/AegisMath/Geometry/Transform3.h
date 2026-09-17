#pragma once
#include <cstddef> // For offsetof
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "Point3.h"
#include "Quaternion.h"
#include "Detail/ABI.h"

namespace AegisMath::Geometry {

    // [T-002] 内部构造安全标签：保全 ABI 的同时封闭非法实例化
    struct TransformValidatedTag final {};

    // [Phase 2.5] Rigid Body Transform Engine SE(3)
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    struct Transform3 final {
    public:
        // 姿态映射: R_{From}^{To}
        Quaternion<T, FrameFrom, FrameTo> rotation_;
        
        // 位移映射 (Origin Offset): FrameFrom 原点在 FrameTo 坐标系下的位置
        Vector3<T, FrameTo> originOffset_;

    private:
        constexpr Transform3(const Quaternion<T, FrameFrom, FrameTo>& rot, 
                             const Vector3<T, FrameTo>& offset,
                             TransformValidatedTag) noexcept 
            : rotation_(rot), originOffset_(offset) {}

    public:
        Transform3() = delete;

        static constexpr Transform3 Create(const Quaternion<T, FrameFrom, FrameTo>& rot, 
                                           const Vector3<T, FrameTo>& offset) noexcept {
            return Transform3(rot, offset, TransformValidatedTag{});
        }

        template <FrameTag F1 = FrameFrom, FrameTag F2 = FrameTo>
        requires std::same_as<F1, F2>
        static constexpr Transform3 Identity() noexcept {
            return Transform3(
                Quaternion<T, FrameFrom, FrameTo>::Identity(),
                Vector3<T, FrameTo>{},
                TransformValidatedTag{}
            );
        }

        // 1. 点的变换: P_B = R_AB * P_A + O_B
        template <ScalarArithmetic U>
        constexpr auto operator*(const Point3<U, FrameFrom>& p) const noexcept {
            using ResT = decltype(T{} * U{});
            Vector3<U, FrameFrom> p_vec(p.x, p.y, p.z);
            Vector3<ResT, FrameTo> rotated = rotation_ * p_vec;
            
            auto trans_res = rotated + originOffset_;
            return Point3<ResT, FrameTo>(trans_res.x, trans_res.y, trans_res.z);
        }

        // 2. 向量的变换 (仅受姿态影响)
        template <ScalarArithmetic U>
        constexpr auto operator*(const Vector3<U, FrameFrom>& v) const noexcept {
            return rotation_ * v;
        }

        // 3. 刚体变换级联: T_AC = T_BC ∘ T_AB (this = T_AB, rhs = T_BC)
        template <FrameTag FrameNext>
        constexpr auto operator*(const Transform3<T, FrameTo, FrameNext>& rhs) const noexcept {
            auto new_rot = rotation_ * rhs.rotation_; 
            auto new_offset = rhs.rotation_ * originOffset_ + rhs.originOffset_;
            
            return Transform3<T, FrameFrom, FrameNext>::Create(new_rot, new_offset);
        }

        // 4. 逆变换: T^{-1}
        constexpr Transform3<T, FrameTo, FrameFrom> Inverse() const noexcept {
            auto inv_rot = rotation_.Conjugate();
            auto inv_offset = -(inv_rot * originOffset_);
            return Transform3<T, FrameTo, FrameFrom>::Create(inv_rot, inv_offset);
        }

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const Transform3& rhs) const noexcept {
            return rotation_ == rhs.rotation_ && originOffset_ == rhs.originOffset_;
        }

        constexpr bool operator!=(const Transform3& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    // 容差自适应近似相等 (Tolerance-Aware Numerical Comparison)
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool AlmostEqual(
        const Transform3<T, FrameFrom, FrameTo>& a,
        const Transform3<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        return AlmostEqual(a.rotation_, b.rotation_, absoluteTolerance, relativeTolerance) &&
               AlmostEqual(a.originOffset_, b.originOffset_, absoluteTolerance, relativeTolerance);
    }

    // --- Geometry Traits 与 ABI 联合注册 ---
    template<typename T, FrameTag FrameFrom, FrameTag FrameTo>
    struct GeometryTraits<Transform3<T, FrameFrom, FrameTo>> {
        using QuatT = Quaternion<T, FrameFrom, FrameTo>;
        using VecT = Vector3<T, FrameTo>;
        using ScalarType = T;
        using TransformT = Transform3<T, FrameFrom, FrameTo>; // 定义别名绕过宏的逗号限制

        static_assert(Detail::GeometryABIValidator<TransformT>::value, 
            "Transform3 failed base ABI constraints.");
            
        // 精确的 offsetof 校验
        static_assert(offsetof(TransformT, originOffset_) == sizeof(QuatT), 
            "Transform3 ABI offset mismatch: padding detected between Quaternion and Vector3.");
            
        static_assert(sizeof(TransformT) == sizeof(QuatT) + sizeof(VecT), 
            "Transform3 size mismatch: trailing padding detected.");
    };

} // namespace AegisMath::Geometry