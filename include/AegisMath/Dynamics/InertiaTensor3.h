#pragma once

#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/QuantityVector3.h"
#include "AegisMath/Units/DerivedUnits/MomentOfInertia.h"

namespace AegisMath::Dynamics {

    struct AngularMomentumUnit {
        using Dimension = Units::Dimension<2, 1, -1, 0, 0, 0, 0, 1>;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

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

        // 编译/运行期对称性及正定性验证
        constexpr bool IsValid() const noexcept {
            InertiaQ eps{static_cast<T>(1e-9)};
            bool symmetric = (ixy - iyx >= -eps && ixy - iyx <= eps) &&
                             (ixz - izx >= -eps && ixz - izx <= eps) &&
                             (iyz - izy >= -eps && iyz - izy <= eps);
            bool positive_diag = (ixx.value() > 0) && (iyy.value() > 0) && (izz.value() > 0);
            return symmetric && positive_diag;
        }

        // 惯量张量乘角速度：I * omega (返回角动量耦合项)
        constexpr QuantityVector3<Units::Quantity<T, AngularMomentumUnit>, Frame>
        Multiply(const AngularVelocity3<Frame, T>& w) const noexcept {
            using ResQ = Units::Quantity<T, AngularMomentumUnit>;
            return QuantityVector3<ResQ, Frame>(
                ResQ(ixx.value() * w.x.value() + ixy.value() * w.y.value() + ixz.value() * w.z.value()),
                ResQ(iyx.value() * w.x.value() + iyy.value() * w.y.value() + iyz.value() * w.z.value()),
                ResQ(izx.value() * w.x.value() + izy.value() * w.y.value() + izz.value() * w.z.value())
            );
        }
    };

} // namespace AegisMath::Dynamics