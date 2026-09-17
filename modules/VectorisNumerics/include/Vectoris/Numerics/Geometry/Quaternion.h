#pragma once
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "RotationMatrix3.h"
#include "CoordinateConvention.h"
#include "Detail/ABI.h"
#include "../Core/MathFunctions.h"
#include "../Core/Result.h"

namespace vectoris::numerics::Geometry {

    struct ValidatedTag final {};

    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    struct Quaternion final {
    public:
        // 组件采用公开成员以满足嵌入式 GNC 与遥测体系的 Standard Layout / Trivially Copyable ABI 约束。
        // 符号规范化 (Canonicalization) 是构造期约定 (TryCreate 保证 w >= 0 when w != 0)，
        // 而非对象生命周期强制不变式 (Lifetime Invariant)。
        T w;
        T x;
        T y;
        T z;

        // [修复 1] 友元声明严格匹配 C++20 Concept (ScalarArithmetic)
        // 允许不同 FrameTag 实例相互访问私有构造函数
        template <ScalarArithmetic, FrameTag, FrameTag>
        friend struct Quaternion;

    private:
        constexpr Quaternion(T _w, T _x, T _y, T _z, ValidatedTag) noexcept
            : w(_w), x(_x), y(_y), z(_z) {}

    public:
        Quaternion() = delete;

        // 仅在起始系和目标系相同时，才允许调用 Identity()
        template <FrameTag F1 = FrameFrom, FrameTag F2 = FrameTo>
        requires std::same_as<F1, F2>
        static constexpr Quaternion Identity() noexcept {
            return Quaternion(T{1}, T{0}, T{0}, T{0}, ValidatedTag{});
        }

        static constexpr Core::Result<Quaternion> TryCreate(T w, T x, T y, T z) noexcept {
            if (!Traits::IsFinite(w) || !Traits::IsFinite(x) || !Traits::IsFinite(y) || !Traits::IsFinite(z)) {
                return Core::Result<Quaternion>::failure(Core::MathError::non_finite_input);
            }
            T sq_len = w*w + x*x + y*y + z*z;
            if (Traits::IsZero(sq_len)) {
                return Core::Result<Quaternion>::failure(Core::MathError::zero_norm);
            }
            T inv_len = T{1} / Core::Math::sqrt(sq_len);
            return Core::Result<Quaternion>::success(
                Quaternion(w * inv_len, x * inv_len, y * inv_len, z * inv_len, ValidatedTag{}).Canonicalized()
            );
        }

        // 构造期符号规范化: 采用精确 w < T{0} 判定，杜绝平台相关浮点容差噪声。
        // 保证非零实部满足 w >= 0；对于 w == 0 (180度纯向量旋转)，符号不作强制翻转 (Option A 约定)。
        // 空间旋转的严格等价性由 RotationEquivalent 双覆盖判定承担。
        constexpr Quaternion Canonicalized() const noexcept {
            if (w < T{0}) {
                return Quaternion(-w, -x, -y, -z, ValidatedTag{});
            }
            return *this;
        }

        constexpr Quaternion<T, FrameTo, FrameFrom> Conjugate() const noexcept {
            return Quaternion<T, FrameTo, FrameFrom>(w, -x, -y, -z, ValidatedTag{});
        }

        // [修复 3] 姿态级联: Q_AC = Q_AB * Q_BC (C++ API 顺序)
        // 物理数学: Q_AC = Q_BC ⊗ Q_AB (Hamilton Product 逆向)
        template <FrameTag FrameNext>
        constexpr auto operator*(const Quaternion<T, FrameTo, FrameNext>& rhs) const noexcept {
            // 注意：这里用 rhs 的元素乘以 this 的元素，实现自动数学倒置
            return Quaternion<T, FrameFrom, FrameNext>(
                rhs.w*w - rhs.x*x - rhs.y*y - rhs.z*z,
                rhs.w*x + rhs.x*w + rhs.y*z - rhs.z*y,
                rhs.w*y - rhs.x*z + rhs.y*w + rhs.z*x,
                rhs.w*z + rhs.x*y - rhs.y*x + rhs.z*w,
                ValidatedTag{}
            ).Canonicalized();
        }

        // [修复 2] 向量旋转：内联叉乘展开，榨干性能且摆脱 Vector3 的 API 依赖
        template <ScalarArithmetic U>
        constexpr auto operator*(const Vector3<U, FrameFrom>& v) const noexcept {
            using ResT = decltype(T{} * U{});

            // 第一次叉乘: uv = q_vec x v
            ResT uv_x = static_cast<ResT>(y) * v.z - static_cast<ResT>(z) * v.y;
            ResT uv_y = static_cast<ResT>(z) * v.x - static_cast<ResT>(x) * v.z;
            ResT uv_z = static_cast<ResT>(x) * v.y - static_cast<ResT>(y) * v.x;

            // 第二次叉乘: uuv = q_vec x uv
            ResT uuv_x = static_cast<ResT>(y) * uv_z - static_cast<ResT>(z) * uv_y;
            ResT uuv_y = static_cast<ResT>(z) * uv_x - static_cast<ResT>(x) * uv_z;
            ResT uuv_z = static_cast<ResT>(x) * uv_y - static_cast<ResT>(y) * uv_x;

            ResT w2 = static_cast<ResT>(w) * T{2};

            return Vector3<ResT, FrameTo>(
                v.x + uv_x * w2 + uuv_x * T{2},
                v.y + uv_y * w2 + uuv_y * T{2},
                v.z + uv_z * w2 + uuv_z * T{2}
            );
        }

        // 拦截跨坐标系非法向量乘法
        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, FrameFrom>)
        constexpr auto operator*(const Vector3<U, OtherFrame>&) const = delete;

        // 导出方向余弦旋转矩阵 (DCM)
        [[nodiscard]] constexpr RotationMatrix3<T, FrameFrom, FrameTo> ToRotationMatrix() const noexcept {
            const T w2 = w * w;
            const T x2 = x * x;
            const T y2 = y * y;
            const T z2 = z * z;

            const T xy = x * y;
            const T xz = x * z;
            const T yz = y * z;
            const T wx = w * x;
            const T wy = w * y;
            const T wz = w * z;

            Matrix3<T> m(
                w2 + x2 - y2 - z2,  T{2} * (xy - wz),    T{2} * (xz + wy),
                T{2} * (xy + wz),   w2 - x2 + y2 - z2,  T{2} * (yz - wx),
                T{2} * (xz - wy),   T{2} * (yz + wx),   w2 - x2 - y2 + z2
            );
            return RotationMatrix3<T, FrameFrom, FrameTo>::TryCreate(m).Value();
        }

        constexpr Core::Result<Quaternion> Slerp(const Quaternion& target, T t) const noexcept {
            if (!Traits::IsFinite(t)) {
                return Core::Result<Quaternion>::failure(Core::MathError::non_finite_input);
            }
            T cos_theta = w*target.w + x*target.x + y*target.y + z*target.z;
            
            Quaternion end = target;
            if (cos_theta < T{0}) {
                cos_theta = -cos_theta;
                end = Quaternion(-target.w, -target.x, -target.y, -target.z, ValidatedTag{});
            }

            if (cos_theta > T{1} - Traits::NumericTraits<T>::epsilon()) {
                return TryCreate(
                    w + t*(end.w - w), x + t*(end.x - x),
                    y + t*(end.y - y), z + t*(end.z - z)
                );
            }

            T theta = Core::Math::acos(cos_theta);
            T sin_theta = Core::Math::sqrt(T{1} - cos_theta*cos_theta);
            T scale_0 = Core::Math::sin((T{1} - t) * theta) / sin_theta;
            T scale_1 = Core::Math::sin(t * theta) / sin_theta;

            return TryCreate(
                scale_0*w + scale_1*end.w, scale_0*x + scale_1*end.x,
                scale_0*y + scale_1*end.y, scale_0*z + scale_1*end.z
            );
        }

        // 精确逐分量数值相等性判定 (Exact component-wise stored-value equality under C++ == semantics)
        constexpr bool operator==(const Quaternion& rhs) const noexcept {
            return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(const Quaternion& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    // 容差自适应逐分量近似相等
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool AlmostEqual(
        const Quaternion<T, FrameFrom, FrameTo>& a,
        const Quaternion<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        return Traits::AlmostEqual(a.w, b.w, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
    }

    // SO(3) 旋转几何等价判定 (双覆盖性质: q 与 -q 表达空间同一旋转)
    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool RotationEquivalent(
        const Quaternion<T, FrameFrom, FrameTo>& a,
        const Quaternion<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept {
        const bool pos_match =
            Traits::AlmostEqual(a.w, b.w, absoluteTolerance, relativeTolerance) &&
            Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
            Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
            Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
        if (pos_match) {
            return true;
        }
        return Traits::AlmostEqual(a.w, -b.w, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.x, -b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, -b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, -b.z, absoluteTolerance, relativeTolerance);
    }

    template<typename T, FrameTag FrameFrom, FrameTag FrameTo>
    struct GeometryTraits<Quaternion<T, FrameFrom, FrameTo>> {
        static constexpr size_t Elements = 4;
        using ScalarType = T;

        static_assert(Detail::GeometryABIValidator<Quaternion<T, FrameFrom, FrameTo>>::value, 
            "Quaternion failed base ABI constraints.");
            
        static_assert(sizeof(Quaternion<T, FrameFrom, FrameTo>) == sizeof(T) * 4, 
            "Quaternion must be exactly 4 scalars with no padding.");
    };

} // namespace vectoris::numerics::Geometry