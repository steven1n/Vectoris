#pragma once

#include <concepts>
#include "AegisMath/Core/Result.h"
#include "AegisMath/Core/MathError.h"
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Geometry/Matrix3.h"
#include "AegisMath/Geometry/SymmetricLinearSolver3.h"
#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"
#include "AegisMath/Units/DerivedUnits/AngularMomentum.h"
#include "AegisMath/Units/DerivedUnits/AngularAcceleration.h"

namespace AegisMath::Dynamics {

    using AngularMomentumUnit = Units::AngularMomentumUnit;

    // 刚体惯量张量（强类型 MomentOfInertia 物理量约束，保证对称性与物理正定性）
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct InertiaTensor3 final {
        using InertiaQ = Units::Quantity<T, Units::KilogramMeterSquaredUnit>;

        InertiaQ ixx, ixy, ixz;
        InertiaQ iyx, iyy, iyz;
        InertiaQ izx, izy, izz;

        constexpr InertiaTensor3(
            InertiaQ _ixx, InertiaQ _ixy, InertiaQ _ixz,
            InertiaQ _iyx, InertiaQ _iyy, InertiaQ _iyz,
            InertiaQ _izx, InertiaQ _izy, InertiaQ _izz
        ) noexcept : ixx(_ixx), ixy(_ixy), ixz(_ixz),
                     iyx(_iyx), iyy(_iyy), iyz(_iyz),
                     izx(_izx), izy(_izy), izz(_izz) {}

        // 编译/运行期对称性及正定性严格验证 (Remediating AML-MED-003)
        // 遵循 Engineering Standard Section 68：
        // 1. 有限性检测 (isfinite)
        // 2. 尺度自适应对称性检测 (|I_ij - I_ji| <= scale * eps * 100)
        // 3. LDL^T 解析主元正定性检测 (d1 > 0, d2 > 0, d3 > 0, 杜绝对角元为正但行列式为负的不定矩阵)
        // 注：若张量不在主轴对角坐标系下，不可直接套用主惯量三角不等式；物理主轴可实现性留待后续特征值求解器扩充
        constexpr bool IsValid() const noexcept {
            // 1. 有限性
            if (!Traits::IsFinite(ixx.value()) || !Traits::IsFinite(ixy.value()) || !Traits::IsFinite(ixz.value()) ||
                !Traits::IsFinite(iyx.value()) || !Traits::IsFinite(iyy.value()) || !Traits::IsFinite(iyz.value()) ||
                !Traits::IsFinite(izx.value()) || !Traits::IsFinite(izy.value()) || !Traits::IsFinite(izz.value())) {
                return false;
            }

            // 2. 元素尺度
            auto abs_fn = [](T val) { return (val >= T{0}) ? val : -val; };
            auto max_fn = [](T a, T b) { return (a >= b) ? a : b; };
            T scale = abs_fn(ixx.value());
            scale = max_fn(scale, abs_fn(ixy.value()));
            scale = max_fn(scale, abs_fn(ixz.value()));
            scale = max_fn(scale, abs_fn(iyx.value()));
            scale = max_fn(scale, abs_fn(iyy.value()));
            scale = max_fn(scale, abs_fn(iyz.value()));
            scale = max_fn(scale, abs_fn(izx.value()));
            scale = max_fn(scale, abs_fn(izy.value()));
            scale = max_fn(scale, abs_fn(izz.value()));

            if (scale <= T{0}) {
                return false;
            }

            const T eps = Traits::NumericTraits<T>::epsilon();

            // 3. 对称性校验
            const T tol_sym = scale * eps * static_cast<T>(100.0);
            if (abs_fn((ixy - iyx).value()) > tol_sym ||
                abs_fn((ixz - izx).value()) > tol_sym ||
                abs_fn((iyz - izy).value()) > tol_sym) {
                return false;
            }

            // 4. 正定性校验 (LDL^T 主元尺度敏感判定)
            const T tol_sing = scale * eps * static_cast<T>(10.0);

            const T d1 = ixx.value();
            if (d1 <= tol_sing) {
                return false;
            }

            const T l21 = ixy.value() / d1;
            const T l31 = ixz.value() / d1;

            const T d2 = iyy.value() - l21 * ixy.value();
            if (d2 <= tol_sing) {
                return false;
            }

            const T l32 = (iyz.value() - l31 * ixy.value()) / d2;
            const T d3 = izz.value() - l31 * ixz.value() - l32 * (iyz.value() - l31 * ixy.value());
            if (d3 <= tol_sing) {
                return false;
            }

            return true;
        }

        // 惯量张量乘角速度：I * omega (返回角动量)
        constexpr AngularMomentum3<Frame, T>
        Multiply(const AngularVelocity3<Frame, T>& w) const noexcept {
            using ResQ = Units::Quantity<T, AngularMomentumUnit>;
            return AngularMomentum3<Frame, T>(
                ResQ(ixx * w.x + ixy * w.y + ixz * w.z),
                ResQ(iyx * w.x + iyy * w.y + iyz * w.z),
                ResQ(izx * w.x + izy * w.y + izz * w.z)
            );
        }

        // 强类型转动方程直接求解：I * alpha = tau ==> alpha = SolveSPD(I, tau)
        // 严格遵循 Solve-Not-Invert 原则，杜绝显式矩阵求逆与对角近似
        template <Geometry::FrameTag TorqueFrame>
        requires std::same_as<TorqueFrame, Frame>
        [[nodiscard]] constexpr Core::Result<AngularAcceleration3<Frame, T>, Core::MathError>
        Solve(const Torque3<TorqueFrame, T>& tau) const noexcept {
            using ResultType = Core::Result<AngularAcceleration3<Frame, T>, Core::MathError>;

            Geometry::Matrix3<T> A(
                ixx.value(), ixy.value(), ixz.value(),
                iyx.value(), iyy.value(), iyz.value(),
                izx.value(), izy.value(), izz.value()
            );
            Geometry::Vector3<T, Frame> b(tau.x.value(), tau.y.value(), tau.z.value());

            auto res = Geometry::SolveSymmetricPositiveDefinite3x3(A, b);
            if (!res.has_value()) {
                return ResultType(res.error());
            }

            using AngAccelQ = Units::Quantity<T, Units::RadianPerSecondSquaredUnit>;
            const auto& sol = res.value();
            return ResultType::success(
                AngularAcceleration3<Frame, T>(
                    AngAccelQ(sol.x),
                    AngAccelQ(sol.y),
                    AngAccelQ(sol.z)
                )
            );
        }
    };

    // 强类型对称正定转动解算自由函数
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    [[nodiscard]] constexpr Core::Result<AngularAcceleration3<Frame, T>, Core::MathError>
    SolveSPD(const InertiaTensor3<T, Frame>& I, const Torque3<Frame, T>& tau) noexcept {
        return I.Solve(tau);
    }

} // namespace AegisMath::Dynamics
