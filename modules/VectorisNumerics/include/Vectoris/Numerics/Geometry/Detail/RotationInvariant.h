#pragma once
#include "../Namespace.h"
#include "../../Core/MathFunctions.h"
#include "../../Core/NumericTraits.h"
#include "../Matrix3.h"

namespace vectoris::numerics::Geometry::Detail {

    // 严苛验证 DCM (方向余弦矩阵) 的数学不变量
    template <ScalarArithmetic T>
    constexpr bool CheckRotationInvariants(const Matrix3<T>& m, 
                                           T tolerance = Traits::NumericTraits<T>::epsilon() * T{100}) noexcept {
        // 1. 验证行列式: |det(R) - 1| <= tolerance
        // 旋转矩阵必须是保向的 (行列式为1)
        T det = m.det();
        if (Core::Math::abs(det - T{1}) > tolerance) {
            return false;
        }

        // 2. 验证正交性: R^T * R = I
        // 计算误差矩阵: diff = (R^T * R) - I
        Matrix3<T> rT_r = m.transposed() * m;
        Matrix3<T> identity = Matrix3<T>::Identity();
        Matrix3<T> diff = rT_r - identity;

        // 使用 Frobenius Norm 评估误差
        // 必须满足 ||R^T * R - I||_F <= tolerance，即 ||R^T * R - I||_F^2 <= tolerance^2
        return diff.frobenius_norm_squared() <= tolerance * tolerance;
    }

} // namespace vectoris::numerics::Geometry::Detail