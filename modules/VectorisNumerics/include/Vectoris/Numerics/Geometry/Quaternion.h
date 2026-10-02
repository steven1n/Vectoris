#pragma once
#include "Namespace.h"
#include <type_traits>
#include <utility>
#include <algorithm>
#include <limits>
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
        // Public components follow AML-DEVIATION-002; their C++ layout traits do not promise a binary/telemetry wire format.
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
        // Preserve reference scalars; otherwise move only when it is safe.
        static constexpr decltype(auto) ComponentArgument(T& value) noexcept {
            if constexpr (std::is_lvalue_reference_v<T>) {
                return (value);
            } else {
                return std::move_if_noexcept(value);
            }
        }

        constexpr Quaternion(T _w, T _x, T _y, T _z, ValidatedTag) noexcept(std::is_arithmetic_v<T>)
            : w(ComponentArgument(_w)), x(ComponentArgument(_x)),
              y(ComponentArgument(_y)), z(ComponentArgument(_z)) {}

    public:
        Quaternion() = delete;

        // Keep the legacy same-frame arguments source-compatible. Availability
        // depends independently on this class's actual frame map.
        template <FrameTag F1 = FrameFrom, FrameTag F2 = FrameTo>
        requires std::same_as<FrameFrom, FrameTo> &&
                 std::same_as<F1, F2>
        static constexpr Quaternion Identity() noexcept(std::is_arithmetic_v<T>) {
            return Quaternion(T{1}, T{0}, T{0}, T{0}, ValidatedTag{});
        }

        static constexpr Core::Result<Quaternion> TryCreate(T w, T x, T y, T z) noexcept(std::is_arithmetic_v<T>) {
            if (!Traits::IsFinite(w) || !Traits::IsFinite(x) || !Traits::IsFinite(y) || !Traits::IsFinite(z)) {
                return Core::Result<Quaternion>::failure(Core::MathError::non_finite_input);
            }
            // Scale before squaring: the sum lies in [1, 4] for nonzero input.
            const T scale = std::max({Core::Math::abs(w), Core::Math::abs(x),
                                      Core::Math::abs(y), Core::Math::abs(z)});
            // Exact input-zero classification, not a tolerance on a computed norm.
            if (scale == T{0}) {
                return Core::Result<Quaternion>::failure(Core::MathError::zero_norm);
            }
            w /= scale;
            x /= scale;
            y /= scale;
            z /= scale;
            const T inv_len = T{1} / Core::Math::sqrt(w*w + x*x + y*y + z*z);
            return Core::Result<Quaternion>::success(
                Quaternion(w * inv_len, x * inv_len, y * inv_len, z * inv_len, ValidatedTag{}).Canonicalized()
            );
        }

        // 构造期符号规范化: 采用精确 w < T{0} 判定，杜绝平台相关浮点容差噪声。
        // 保证非零实部满足 w >= 0；对于 w == 0 (180度纯向量旋转)，符号不作强制翻转 (Option A 约定)。
        // 空间旋转的严格等价性由 RotationEquivalent 双覆盖判定承担。
        constexpr Quaternion Canonicalized() const noexcept(std::is_arithmetic_v<T>) {
            if (w < T{0}) {
                return Quaternion(-w, -x, -y, -z, ValidatedTag{});
            }
            return *this;
        }

        constexpr Quaternion<T, FrameTo, FrameFrom> Conjugate() const noexcept(std::is_arithmetic_v<T>) {
            return Quaternion<T, FrameTo, FrameFrom>(w, -x, -y, -z, ValidatedTag{});
        }

        // [修复 3] 姿态级联: Q_AC = Q_AB * Q_BC (C++ API 顺序)
        // 物理数学: Q_AC = Q_BC ⊗ Q_AB (Hamilton Product 逆向)
        template <FrameTag FrameNext>
        constexpr auto operator*(const Quaternion<T, FrameTo, FrameNext>& rhs) const noexcept(std::is_arithmetic_v<T>) {
            // 注意：这里用 rhs 的元素乘以 this 的元素，实现自动数学倒置
            return Quaternion<T, FrameFrom, FrameNext>(
                rhs.w*w - rhs.x*x - rhs.y*y - rhs.z*z,
                rhs.w*x + rhs.x*w + rhs.y*z - rhs.z*y,
                rhs.w*y - rhs.x*z + rhs.y*w + rhs.z*x,
                rhs.w*z + rhs.x*y - rhs.y*x + rhs.z*w,
                ValidatedTag{}
            ).Canonicalized();
        }

    private:
        // For finite products, opposite signs are added before the remaining term.
        // If all signs agree, a partial sum cannot exceed the final magnitude.
        // Unlike scaling the whole vector, this preserves isolated subnormal terms.
        template <std::floating_point R>
        static constexpr bool RotationSumNeedsWide(R a, R b) noexcept {
            // Equality takes the wide path too: max-b itself can round upward.
            return b > R{0} ? a >= std::numeric_limits<R>::max() - b
                            : a <= -std::numeric_limits<R>::max() - b;
        }

        template <std::floating_point R>
        static constexpr bool SumRotationProducts(R a, R b, R c, R& result) noexcept {
            const R lo = std::min({a, b, c});
            const R hi = std::max({a, b, c});
            const R mid = std::clamp(b, std::min(a, c), std::max(a, c));
            if (RotationSumNeedsWide(lo, hi)) return false;
            const R partial = lo + hi;
            if (RotationSumNeedsWide(partial, mid)) return false;
            result = partial + mid;
            return true;
        }

        // Two-component arithmetic is used only at the overflow boundary. Extra
        // precision must not depend on long double being wider (it is not on MSVC).
        template <std::floating_point R>
        struct RotationWide final { R hi; R lo; };

        template <std::floating_point R>
        static constexpr RotationWide<R> AddWide(RotationWide<R> a, RotationWide<R> b) noexcept {
            const R sum = a.hi + b.hi;
            const R b_virtual = sum - a.hi;
            const R error = ((a.hi - (sum - b_virtual)) + (b.hi - b_virtual)) + (a.lo + b.lo);
            const R hi = sum + error;
            return {hi, error - (hi - sum)};
        }

        template <std::floating_point R>
        static constexpr RotationWide<R> ProductWide(R a, R b) noexcept {
            constexpr R splitter = [] {
                R power = R{1};
                for (int i = 0; i < (std::numeric_limits<R>::digits + 1) / 2; ++i) power *= R{2};
                return power + R{1};
            }();
            const R a_split = splitter * a, b_split = splitter * b;
            const R a_hi = a_split - (a_split - a), b_hi = b_split - (b_split - b);
            const R a_lo = a - a_hi, b_lo = b - b_hi;
            const R product = a * b;
            const R error = ((a_hi * b_hi - product) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
            return {product, error};
        }

        template <std::floating_point R>
        static constexpr RotationWide<R> MultiplyWide(RotationWide<R> a, R b) noexcept {
            const auto product = ProductWide(a.hi, b);
            return AddWide(product, RotationWide<R>{a.lo * b, R{0}});
        }

        template <std::floating_point R>
        static constexpr RotationWide<R> NegateWide(RotationWide<R> a) noexcept {
            return {-a.hi, -a.lo};
        }

        template <std::floating_point R>
        static constexpr R BoundaryRotationComponent(R qw, R qx, R qy, R qz,
                                                       R vx, R vy, R vz) noexcept {
            // Power-of-two scaling is exact for the large terms. Underflow of tiny
            // terms here is insignificant to an output already near overflow;
            // finite ordinary outputs never take this path (including denormals).
            constexpr R down = std::numeric_limits<R>::min() / R{2};
            constexpr R up = std::numeric_limits<R>::max() /
                (R{2} - std::numeric_limits<R>::epsilon());
            const auto a = AddWide(ProductWide(qw, qw), ProductWide(qx, qx));
            const auto b = AddWide(ProductWide(qy, qy), ProductWide(qz, qz));
            const auto norm = AddWide(a, b);
            const auto diagonal = AddWide(a, NegateWide(b));
            const auto off_y = AddWide(ProductWide(qx, qy), NegateWide(ProductWide(qw, qz)));
            const auto off_z = AddWide(ProductWide(qx, qz), ProductWide(qw, qy));
            const auto numerator = AddWide(MultiplyWide(diagonal, vx * down),
                AddWide(MultiplyWide(off_y, R{2} * (vy * down)),
                        MultiplyWide(off_z, R{2} * (vz * down))));
            const R quotient = numerator.hi / norm.hi;
            const auto residual = AddWide(numerator, NegateWide(MultiplyWide(norm, quotient)));
            const auto corrected = AddWide(RotationWide<R>{quotient, R{0}},
                RotationWide<R>{(residual.hi + residual.lo) / norm.hi, R{0}});
            // Round once near magnitude 2, before exact power-of-two rescaling.
            // No clamp or fallback sentinel: genuinely overflowing results may be Inf.
            return (corrected.hi + corrected.lo) * up;
        }

    public:
        // AFA-001: valid unit quaternion + finite vector. No FP contraction is
        // required. Coefficients are bounded before multiplying unscaled inputs.
        template <ScalarArithmetic U>
        constexpr auto operator*(const Vector3<U, FrameFrom>& v) const noexcept(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            using ResT = decltype(T{} * U{});
            if constexpr (std::floating_point<T> && std::floating_point<U>) {
                // Exact zero classification; preserve the input zero signs.
                if (v.x == U{0} && v.y == U{0} && v.z == U{0}) {
                    return Vector3<ResT, FrameTo>{v.x, v.y, v.z};
                }
                const ResT qw = w, qx = x, qy = y, qz = z;
                const ResT ww = qw * qw, xx = qx * qx, yy = qy * qy, zz = qz * qz;
                const ResT norm2 = (ww + xx) + (yy + zz);
                // Divide by the unit quaternion's stored squared norm to avoid
                // amplification of normalization rounding. Exact rotation entries
                // lie in [-1,1]; clamping only bounds coefficient round-off.
                const auto coefficient = [norm2](ResT value) constexpr noexcept {
                    return std::clamp(value / norm2, ResT{-1}, ResT{1});
                };
                const ResT r00 = coefficient((ww + xx) - (yy + zz));
                const ResT r11 = coefficient((ww + yy) - (xx + zz));
                const ResT r22 = coefficient((ww + zz) - (xx + yy));
                const ResT r01 = coefficient(ResT{2} * (qx * qy - qw * qz));
                const ResT r02 = coefficient(ResT{2} * (qx * qz + qw * qy));
                const ResT r10 = coefficient(ResT{2} * (qx * qy + qw * qz));
                const ResT r12 = coefficient(ResT{2} * (qy * qz - qw * qx));
                const ResT r20 = coefficient(ResT{2} * (qx * qz - qw * qy));
                const ResT r21 = coefficient(ResT{2} * (qy * qz + qw * qx));
                auto result = Vector3<ResT, FrameTo>{};
                // Rounded coefficients can otherwise cause a false Inf at max().
                // Re-evaluate that component with compensated, bounded arithmetic.
                if (!SumRotationProducts(r00 * v.x, r01 * v.y, r02 * v.z, result.x)) {
                    result.x = BoundaryRotationComponent(qw, qx, qy, qz,
                        static_cast<ResT>(v.x), static_cast<ResT>(v.y), static_cast<ResT>(v.z));
                }
                if (!SumRotationProducts(r10 * v.x, r11 * v.y, r12 * v.z, result.y)) {
                    result.y = BoundaryRotationComponent(qw, qy, qz, qx,
                        static_cast<ResT>(v.y), static_cast<ResT>(v.z), static_cast<ResT>(v.x));
                }
                if (!SumRotationProducts(r20 * v.x, r21 * v.y, r22 * v.z, result.z)) {
                    result.z = BoundaryRotationComponent(qw, qz, qx, qy,
                        static_cast<ResT>(v.z), static_cast<ResT>(v.x), static_cast<ResT>(v.y));
                }
                return result;
            } else {
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
        }

        // 拦截跨坐标系非法向量乘法
        template <ScalarArithmetic U, FrameTag OtherFrame>
        requires (!std::same_as<OtherFrame, FrameFrom>)
        constexpr auto operator*(const Vector3<U, OtherFrame>&) const = delete;

        // 导出方向余弦旋转矩阵 (DCM)
        [[nodiscard]] constexpr Core::Result<RotationMatrix3<T, FrameFrom, FrameTo>>
        ToRotationMatrix() const noexcept(std::is_arithmetic_v<T>) {
            // Public components and accumulated products may no longer be unit length.
            // Validate before multiplication, including finite values whose square overflows.
            if (!Traits::IsFinite(w) || !Traits::IsFinite(x) ||
                !Traits::IsFinite(y) || !Traits::IsFinite(z)) {
                return Core::Result<RotationMatrix3<T, FrameFrom, FrameTo>>::failure(
                    Core::MathError::non_finite_input);
            }
            if (Core::Math::abs(w) > T{1} || Core::Math::abs(x) > T{1} ||
                Core::Math::abs(y) > T{1} || Core::Math::abs(z) > T{1}) {
                return Core::Result<RotationMatrix3<T, FrameFrom, FrameTo>>::failure(
                    Core::MathError::invalid_state);
            }
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
            return RotationMatrix3<T, FrameFrom, FrameTo>::TryCreate(m);
        }

        constexpr Core::Result<Quaternion> Slerp(const Quaternion& target, T t) const noexcept(std::is_arithmetic_v<T>) {
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
        constexpr bool operator==(const Quaternion& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(const Quaternion& rhs) const noexcept(std::is_arithmetic_v<T>) {
            return !(*this == rhs);
        }
    };

    // 容差自适应逐分量近似相等
    template <Concepts::FloatingPoint T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool AlmostEqual(
        const Quaternion<T, FrameFrom, FrameTo>& a,
        const Quaternion<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept(std::is_arithmetic_v<T>) {
        return Traits::AlmostEqual(a.w, b.w, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.x, b.x, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.y, b.y, absoluteTolerance, relativeTolerance) &&
               Traits::AlmostEqual(a.z, b.z, absoluteTolerance, relativeTolerance);
    }

    // SO(3) 旋转几何等价判定 (双覆盖性质: q 与 -q 表达空间同一旋转)
    template <Concepts::FloatingPoint T, FrameTag FrameFrom, FrameTag FrameTo>
    [[nodiscard]] inline bool RotationEquivalent(
        const Quaternion<T, FrameFrom, FrameTo>& a,
        const Quaternion<T, FrameFrom, FrameTo>& b,
        T absoluteTolerance = Traits::NumericTraits<T>::epsilon() * T{100},
        T relativeTolerance = Traits::NumericTraits<T>::epsilon() * T{100}
    ) noexcept(std::is_arithmetic_v<T>) {
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
            "Quaternion failed source layout constraints.");
            
        static_assert(sizeof(Quaternion<T, FrameFrom, FrameTo>) == sizeof(T) * 4, 
            "Quaternion must be exactly 4 scalars with no padding.");
    };

} // namespace vectoris::numerics::Geometry
