#pragma once

namespace AegisMath::Dynamics::Convention {

    // 坐标系规约：右手坐标系 (Right-Handed Coordinate System)
    // 航空航天体系 (Body Frame):
    // - X 轴：向前 (Forward / Longitudinal)
    // - Y 轴：向右 (Right / Lateral)
    // - Z 轴：向下 (Down / Normal / Vertical)

    // 空间角速度规约: omega = [p, q, r]^T
    // - p: 滚转速率 (Roll rate about X)
    // - q: 俯仰速率 (Pitch rate about Y)
    // - r: 偏航速率 (Yaw rate about Z)

    // 线速度规约 (Body Velocity): V = [u, v, w]^T
    // - u: 轴向纵向速度 (Forward)
    // - v: 侧向速度 (Side)
    // - w: 法向垂向速度 (Vertical)

} // namespace AegisMath::Dynamics::Convention