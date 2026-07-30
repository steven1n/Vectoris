#pragma once

#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Concepts.h"
#include <stdexcept>

namespace AegisMath::Dynamics {

    // 刚体惯量张量（独立于普通 Matrix3，保证对称性与物理正定性）
    template <DynamicsScalar T, Geometry::FrameTag Frame>
    struct InertiaTensor3 final {
        T ixx, ixy, ixz;
        T iyx, iyy, iyz;
        T izx, izy, izz;

        constexpr InertiaTensor3(
            T _ixx, T _ixy, T _ixz,
            T _iyx, T _iyy, T _iyz,
            T _izx, T _izy, T _izz
        ) noexcept : ixx(_ixx), ixy(_ixy), ixz(_ixz),
                     iyx(_iyx), iyy(_iyy), iyz(_iyz),
                     izx(_izx), izy(_izy), izz(_izz) {}

        // 编译/运行期对称性及正定性验证
        constexpr bool IsValid() const noexcept {
            constexpr T eps = static_cast<T>(1e-9);
            // 对称性检查
            bool symmetric = (ixy - iyx >= -eps && ixy - iyx <= eps) &&
                             (ixz - izx >= -eps && ixz - izx <= eps) &&
                             (iyz - izy >= -eps && iyz - izy <= eps);
            // 正定性（对角线必须大于0）
            bool positive_diag = (ixx > 0) && (iyy > 0) && (izz > 0);
            return symmetric && positive_diag;
        }

        // 惯量张量乘角速度：返回角动量或力矩耦合项 (I * omega)
        constexpr Geometry::Vector3<T, Frame> Multiply(const Geometry::Vector3<T, Frame>& w) const noexcept {
            return Geometry::Vector3<T, Frame>(
                ixx * w.x + ixy * w.y + ixz * w.z,
                iyx * w.x + iyy * w.y + iyz * w.z,
                izx * w.x + izy * w.y + izz * w.z
            );
        }
    };

} // namespace AegisMath::Dynamics