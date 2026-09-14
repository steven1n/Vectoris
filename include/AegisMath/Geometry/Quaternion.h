#pragma once
#include "Concepts.h"
#include "FrameTags.h"
#include "Vector3.h"
#include "RotationMatrix3.h"
#include "CoordinateConvention.h"
#include "Detail/ABI.h"
#include "../Core/MathFunctions.h"
#include "../Core/Result.h"

namespace AegisMath::Geometry {

    struct ValidatedTag final {};

    template <ScalarArithmetic T, FrameTag FrameFrom, FrameTag FrameTo>
    struct Quaternion final {
    public:
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
            T sq_len = w*w + x*x + y*y + z*z;
            if (Traits::IsZero(sq_len)) {
                return Core::Result<Quaternion>();
            }
            T inv_len = T{1} / Core::Math::sqrt(sq_len);
            return Core::Result<Quaternion>(
                Quaternion(w * inv_len, x * inv_len, y * inv_len, z * inv_len, ValidatedTag{}).Canonicalized()
            );
        }

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

        constexpr Core::Result<Quaternion> Slerp(const Quaternion& target, T t) const noexcept {
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
    };

    template<typename T, FrameTag FrameFrom, FrameTag FrameTo>
    struct GeometryTraits<Quaternion<T, FrameFrom, FrameTo>> {
        static constexpr size_t Elements = 4;
        using ScalarType = T;

        static_assert(Detail::GeometryABIValidator<Quaternion<T, FrameFrom, FrameTo>>::value, 
            "Quaternion failed base ABI constraints.");
            
        static_assert(sizeof(Quaternion<T, FrameFrom, FrameTo>) == sizeof(T) * 4, 
            "Quaternion must be exactly 4 scalars with no padding.");
    };

} // namespace AegisMath::Geometry