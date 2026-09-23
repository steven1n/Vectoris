#pragma once
#include "Namespace.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <limits>
#include "Vectoris/Numerics/Core/Concepts.h"
#include "Vectoris/Numerics/Core/MathError.h"
#include "Vectoris/Numerics/Core/Result.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Numerics/Geometry/Concepts.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/Vector3.h"

namespace vectoris::numerics::Geometry {

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

    namespace Detail {
        // Runtime frexp/scalbn, with bounded C++20 constant-evaluation equivalents.
        template <Concepts::FloatingPoint T>
        constexpr T SolverFractionFallback(T value, int& exponent) noexcept {
            exponent = 0;
            constexpr int max_iterations = std::numeric_limits<T>::max_exponent
                - std::numeric_limits<T>::min_exponent + std::numeric_limits<T>::digits;
            for (int i = 0; i < max_iterations && ConstexprAbs(value) >= T{1}; ++i) {
                value *= T{0.5};
                ++exponent;
            }
            for (int i = 0; i < max_iterations && ConstexprAbs(value) < T{0.5}
                    && value != T{0}; ++i) {
                value *= T{2};
                --exponent;
            }
            return value;
        }

        template <Concepts::FloatingPoint T>
        constexpr T SolverScaleExponentFallback(T value, int exponent) noexcept {
            // Collapse the subnormal tail into one division to avoid repeated rounding.
            constexpr int max_iterations = 3 * (std::numeric_limits<T>::max_exponent
                - std::numeric_limits<T>::min_exponent + std::numeric_limits<T>::digits);
            T divisor = T{1};
            for (int i = 0; i < max_iterations && exponent < 0; ++i, ++exponent) {
                if (divisor <= std::numeric_limits<T>::max() * std::numeric_limits<T>::epsilon() * T{0.25}) {
                    divisor *= T{2};
                } else {
                    value /= divisor;
                    divisor = T{2};
                }
            }
            value /= divisor;
            for (int i = 0; i < max_iterations && exponent > 0; ++i, --exponent) {
                if (ConstexprAbs(value) > std::numeric_limits<T>::max() / T{2}) {
                    return value < T{0} ? -std::numeric_limits<T>::infinity()
                                        : std::numeric_limits<T>::infinity();
                }
                value *= T{2};
            }
            return value;
        }

        template <Concepts::FloatingPoint T>
        constexpr T SolverFraction(T value, int& exponent) noexcept {
            if (std::is_constant_evaluated()) return SolverFractionFallback(value, exponent);
            return std::frexp(value, &exponent);
        }

        template <Concepts::FloatingPoint T>
        constexpr T SolverScaleExponent(T value, int exponent) noexcept {
            if (std::is_constant_evaluated()) return SolverScaleExponentFallback(value, exponent);
            return std::scalbn(value, exponent);
        }

        // value * numerator / denominator without forming either dangerous scale ratio.
        // Preconditions: finite value, finite positive numerator and denominator.
        template <Concepts::FloatingPoint T>
        constexpr T SolverRescale(T value, T numerator, T denominator) noexcept {
            int ev = 0;
            int en = 0;
            int ed = 0;
            const T fv = SolverFraction(value, ev);
            const T fn = SolverFraction(numerator, en);
            const T fd = SolverFraction(denominator, ed);
            return SolverScaleExponent((fv * fn) / fd, ev + en - ed);
        }

        template <Concepts::FloatingPoint T>
        struct LDLT3Factors {
            T d0, d1, d2, l10, l20, l21;
        };

        template <Concepts::FloatingPoint T>
        constexpr Core::Result<T, Core::MathError> SolverPivot(T pivot) noexcept {
            using R = Core::Result<T, Core::MathError>;
            const T tolerance = T{10} * Traits::NumericTraits<T>::epsilon();
            if (!Traits::IsFinite(pivot)) return R::failure(Core::MathError::ill_conditioned);
            if (pivot < -tolerance) return R::failure(Core::MathError::invalid_state);
            if (pivot <= tolerance) return R::failure(Core::MathError::singular_matrix);
            return R::success(pivot);
        }

        // The original sqrt-free LDLT equations, now applied to A / max(abs(A)).
        template <Concepts::FloatingPoint T>
        constexpr Core::Result<LDLT3Factors<T>, Core::MathError>
        SolverFactor(const Matrix3<T>& a) noexcept {
            using R = Core::Result<LDLT3Factors<T>, Core::MathError>;
            const auto d0 = SolverPivot(a(0, 0));
            if (!d0) return R::failure(d0.error());
            const T l10 = a(1, 0) / d0.value();
            const T l20 = a(2, 0) / d0.value();
            const auto d1 = SolverPivot(a(1, 1) - l10 * a(1, 0));
            if (!d1) return R::failure(d1.error());
            const T l21 = (a(2, 1) - l20 * a(1, 0)) / d1.value();
            const auto d2 = SolverPivot(a(2, 2) - l20 * a(2, 0)
                - l21 * (a(2, 1) - l20 * a(1, 0)));
            if (!d2) return R::failure(d2.error());
            const T min_d = ConstexprMin(d0.value(), ConstexprMin(d1.value(), d2.value()));
            const T max_d = ConstexprMax(d0.value(), ConstexprMax(d1.value(), d2.value()));
            if (min_d / max_d <= T{100} * Traits::NumericTraits<T>::epsilon()) {
                return R::failure(Core::MathError::ill_conditioned);
            }
            return R::success(LDLT3Factors<T>{d0.value(), d1.value(), d2.value(), l10, l20, l21});
        }

        template <Concepts::FloatingPoint T, FrameTag F>
        constexpr Vector3<T, F> SolverSubstitute(const LDLT3Factors<T>& f,
                                                const Vector3<T, F>& b) noexcept {
            const T y1 = b.y - f.l10 * b.x;
            const T y2 = b.z - f.l20 * b.x - f.l21 * y1;
            const T x2 = y2 / f.d2;
            const T x1 = y1 / f.d1 - f.l21 * x2;
            const T x0 = b.x / f.d0 - f.l10 * x1 - f.l20 * x2;
            return {x0, x1, x2};
        }

        template <Concepts::FloatingPoint T, FrameTag F>
        constexpr bool SolverVectorFinite(const Vector3<T, F>& v) noexcept {
            return Traits::IsFinite(v.x) && Traits::IsFinite(v.y) && Traits::IsFinite(v.z);
        }

        // Kept separate so non-finite diagnostics can be tested by direct fault injection.
        template <Concepts::FloatingPoint T>
        constexpr bool SolverBackwardErrorAccepted(T residual, T denominator) noexcept {
            if (!Traits::IsFinite(residual) || !Traits::IsFinite(denominator)) return false;
            if (denominator == T{0}) return residual == T{0};
            const T eta = residual / denominator;
            if (!Traits::IsFinite(eta)) return false;
            return eta <= T{100} * Traits::NumericTraits<T>::epsilon();
        }

        template <Concepts::FloatingPoint T>
        constexpr bool SolverResidualAccepted(const Matrix3<T>& a,
            const std::array<T, 3>& x, const std::array<T, 3>& b) noexcept {
            T q = T{1};
            for (size_t i = 0; i < 3; ++i) {
                if (!Traits::IsFinite(x[i]) || !Traits::IsFinite(b[i])) return false;
                q = ConstexprMax(q, ConstexprAbs(x[i]));
            }
            T a_norm = T{0};
            T x_norm = T{0};
            T b_norm = T{0};
            T r_norm = T{0};
            for (size_t i = 0; i < 3; ++i) {
                T row_norm = T{0};
                T residual = -b[i] / q;
                for (size_t j = 0; j < 3; ++j) {
                    row_norm += ConstexprAbs(a(i, j));
                    residual += a(i, j) * (x[j] / q);
                }
                // Check before max: a comparison-based max can hide a NaN operand.
                if (!Traits::IsFinite(row_norm) || !Traits::IsFinite(residual)) return false;
                a_norm = ConstexprMax(a_norm, row_norm);
                r_norm = ConstexprMax(r_norm, ConstexprAbs(residual));
                x_norm = ConstexprMax(x_norm, ConstexprAbs(x[i] / q));
                b_norm = ConstexprMax(b_norm, ConstexprAbs(b[i] / q));
            }
            return SolverBackwardErrorAccepted(r_norm, a_norm * x_norm + b_norm);
        }
    } // namespace Detail

    /**
     * Fixed-size, sqrt-free SPD LDLT. Normalize A by sA=max(abs(A)) and b by
     * sb=max(abs(b)); solve (A/sA)y=b/sb and restore x=y*sb/sA using exponents.
     * Symmetry tolerance is 100*eps in normalized coordinates; the lower triangle
     * is factored, but the original (normalized) matrix is used for the residual.
     * Pivots must exceed 10*eps, pivot spread must exceed 100*eps (not cond(A)).
     * Accept only finite output and finite normwise backward error <=100*eps.
     * See docs/geometry.md for rounding, conditioning and representability limits.
     */
    template <Concepts::FloatingPoint T, FrameTag Frame = FrameUnknown>
    [[nodiscard]] constexpr Core::Result<Vector3<T, Frame>, Core::MathError>
    SolveSymmetricPositiveDefinite3x3(const Matrix3<T>& A, const Vector3<T, Frame>& b) noexcept {
        using R = Core::Result<Vector3<T, Frame>, Core::MathError>;
        T sa = T{0};
        for (size_t i = 0; i < 9; ++i) {
            if (!Traits::IsFinite(A.m[i])) return R::failure(Core::MathError::non_finite_input);
            sa = Detail::ConstexprMax(sa, Detail::ConstexprAbs(A.m[i]));
        }
        if (!Detail::SolverVectorFinite(b)) {
            return R::failure(Core::MathError::non_finite_input);
        }
        if (sa == T{0}) return R::failure(Core::MathError::singular_matrix);
        Matrix3<T> a;
        for (size_t i = 0; i < 9; ++i) a.m[i] = A.m[i] / sa;
        const T symmetry_tolerance = T{100} * Traits::NumericTraits<T>::epsilon();
        if (Detail::ConstexprAbs(a(0, 1) - a(1, 0)) > symmetry_tolerance ||
            Detail::ConstexprAbs(a(0, 2) - a(2, 0)) > symmetry_tolerance ||
            Detail::ConstexprAbs(a(1, 2) - a(2, 1)) > symmetry_tolerance) {
            return R::failure(Core::MathError::invalid_argument);
        }
        const auto factors = Detail::SolverFactor(a);
        if (!factors) return R::failure(factors.error());
        const T sb = Detail::ConstexprMax(Detail::ConstexprAbs(b.x),
            Detail::ConstexprMax(Detail::ConstexprAbs(b.y), Detail::ConstexprAbs(b.z)));
        if (sb == T{0}) return R::success(Vector3<T, Frame>{});
        const Vector3<T, Frame> bn{b.x / sb, b.y / sb, b.z / sb};
        const auto y = Detail::SolverSubstitute(factors.value(), bn);
        if (!Detail::SolverVectorFinite(y)) {
            return R::failure(Core::MathError::ill_conditioned);
        }
        const Vector3<T, Frame> x{Detail::SolverRescale(y.x, sb, sa),
            Detail::SolverRescale(y.y, sb, sa), Detail::SolverRescale(y.z, sb, sa)};
        if (!Detail::SolverVectorFinite(x)) {
            return R::failure(Core::MathError::ill_conditioned);
        }
        // Validate the rounded output, including subnormal rounding, not just y.
        const std::array<T, 3> restored{Detail::SolverRescale(x.x, sa, sb),
            Detail::SolverRescale(x.y, sa, sb), Detail::SolverRescale(x.z, sa, sb)};
        if (!Detail::SolverResidualAccepted(a, restored, {bn.x, bn.y, bn.z})) {
            return R::failure(Core::MathError::ill_conditioned);
        }
        return R::success(x);
    }
} // namespace vectoris::numerics::Geometry
