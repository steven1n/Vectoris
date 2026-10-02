#pragma once
#include "Namespace.h"
#include <type_traits>
#include <algorithm>
#include <limits>
#include "Concepts.h"
#include "FrameTags.h"
#include "Matrix3.h"
#include "Vector3.h"
#include "Detail/RotationInvariant.h"
#include "Detail/ABI.h"
#include "../Core/Result.h"
#include "../Units/UnitConcepts.h"

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

        // AFA2-009: checked rotations require overflow-resistant row evaluation.
        // Raw Matrix3 arithmetic keeps its existing unchecked scalar semantics.
        template <std::floating_point R>
        struct RowWide final { R hi; R lo; };

        template <std::floating_point R>
        static constexpr RowWide<R> AddRowWide(RowWide<R> a, RowWide<R> b) noexcept {
            const R sum = a.hi + b.hi;
            const R virtual_b = sum - a.hi;
            const R error = ((a.hi - (sum - virtual_b)) + (b.hi - virtual_b)) + (a.lo + b.lo);
            const R hi = sum + error;
            return {hi, error - (hi - sum)};
        }

        template <std::floating_point R>
        static constexpr RowWide<R> BoundedProduct(R a, R b) noexcept {
            constexpr R splitter = [] {
                R power = R{1};
                for (int i = 0; i < (std::numeric_limits<R>::digits + 1) / 2; ++i) power *= R{2};
                return power + R{1};
            }();
            const R split_a = splitter * a, split_b = splitter * b;
            const R hi_a = split_a - (split_a - a), hi_b = split_b - (split_b - b);
            const R lo_a = a - hi_a, lo_b = b - hi_b;
            const R product = a * b;
            const R error = ((hi_a * hi_b - product) + hi_a * lo_b + lo_a * hi_b) + lo_a * lo_b;
            return {product, error};
        }

        template <std::floating_point R>
        static constexpr bool RowSumAtBoundary(R a, R b) noexcept {
            return b > R{0} ? a >= std::numeric_limits<R>::max() - b
                            : a <= -std::numeric_limits<R>::max() - b;
        }

        template <std::floating_point R>
        static constexpr R ApplyRow(R r0, R r1, R r2, R x, R y, R z) noexcept {
            const R a = r0 * x, b = r1 * y, c = r2 * z;
            // Measured ordinary-path shortcut: three products bounded by max/4
            // cannot overflow their two additions. Keep existing rounding here.
            constexpr R ordinary_limit = std::numeric_limits<R>::max() / R{4};
            if (a >= -ordinary_limit && a <= ordinary_limit &&
                b >= -ordinary_limit && b <= ordinary_limit &&
                c >= -ordinary_limit && c <= ordinary_limit) return (a + b) + c;
            const R lo = std::min({a, b, c}), hi = std::max({a, b, c});
            const R mid = std::clamp(b, std::min(a, c), std::max(a, c));
            // Opposite signs first: preserve isolated tiny terms and avoid
            // intermediate overflow without scaling ordinary/subnormal inputs.
            constexpr R max = std::numeric_limits<R>::max();
            if (lo >= -max && hi <= max && !RowSumAtBoundary(lo, hi)) {
                const R partial = lo + hi;
                if (!RowSumAtBoundary(partial, mid)) return partial + mid;
            }
            // Existing IEEE propagation for inputs outside the finite contract.
            if (!(Traits::IsFinite(x) && Traits::IsFinite(y) && Traits::IsFinite(z))) {
                return (a + b) + c;
            }
            // Only the overflow boundary uses a two-component dot product.
            // Valid rotation coefficients are near [-1,1]; scaled inputs <= 2.
            // Exact powers of two prevent scale division error. This does not
            // require wider long double, FMA, or compiler FP contraction.
            constexpr R down = std::numeric_limits<R>::min() / R{2};
            constexpr R up = max / (R{2} - std::numeric_limits<R>::epsilon());
            const auto sum = AddRowWide(AddRowWide(BoundedProduct(r0, x * down),
                                                  BoundedProduct(r1, y * down)),
                                       BoundedProduct(r2, z * down));
            return (sum.hi + sum.lo) * up;
        }

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
            if constexpr (std::floating_point<T> && std::floating_point<U>) {
                return Vector3<ResT, FrameTo>{
                    ApplyRow<ResT>(dcm_(0,0), dcm_(0,1), dcm_(0,2), v.x, v.y, v.z),
                    ApplyRow<ResT>(dcm_(1,0), dcm_(1,1), dcm_(1,2), v.x, v.y, v.z),
                    ApplyRow<ResT>(dcm_(2,0), dcm_(2,1), dcm_(2,2), v.x, v.y, v.z)
                };
            } else if constexpr (std::floating_point<T> && Units::IsQuantity<U>) {
                // Unwrap only within the dimension-preserving numerical adapter.
                // Existing Quantity scalar multiplication requires the same T.
                return Vector3<ResT, FrameTo>{
                    ResT{ApplyRow<T>(dcm_(0,0), dcm_(0,1), dcm_(0,2), v.x.value(), v.y.value(), v.z.value())},
                    ResT{ApplyRow<T>(dcm_(1,0), dcm_(1,1), dcm_(1,2), v.x.value(), v.y.value(), v.z.value())},
                    ResT{ApplyRow<T>(dcm_(2,0), dcm_(2,1), dcm_(2,2), v.x.value(), v.y.value(), v.z.value())}
                };
            } else {
                // Generic custom scalar operations retain exception propagation.
                return Vector3<ResT, FrameTo>{
                    dcm_(0,0)*v.x + dcm_(0,1)*v.y + dcm_(0,2)*v.z,
                    dcm_(1,0)*v.x + dcm_(1,1)*v.y + dcm_(1,2)*v.z,
                    dcm_(2,0)*v.x + dcm_(2,1)*v.y + dcm_(2,2)*v.z
                };
            }
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
