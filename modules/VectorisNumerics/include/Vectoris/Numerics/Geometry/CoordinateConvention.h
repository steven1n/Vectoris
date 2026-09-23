#pragma once
#include "Namespace.h"

namespace vectoris::numerics::Geometry {

    // [GNC Convention] 航空航天坐标与旋转约定
    enum class RotationConvention {
        // SourceToTarget (主动映射 / Active Rotation)
        // 物理意义: 将 Source 坐标系下的向量，映射到 Target 坐标系下的坐标表达。
        // 公式: v_target = R_SourceToTarget * v_source
        // 示例: v_ecef = R_body_to_ecef * v_body
        SourceToTarget,

        // TargetToSource (被动映射 / Passive Rotation)
        TargetToSource
    };

    // Vectoris 强制冻结的系统级约定
    struct SystemConvention {
        static constexpr RotationConvention FrameMapping = RotationConvention::SourceToTarget;
        
        // Quaternion 乘法约定: Hamilton (非 JPL)
        // 元素顺序: [w, x, y, z] (w 为标量实部)
        // 右手法则
    };

} // namespace vectoris::numerics::Geometry