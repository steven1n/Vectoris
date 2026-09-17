#pragma once

#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Dynamics/Concepts.h"
#include "Vectoris/Dynamics/Detail/DynamicsABI.h"
#include "Vectoris/Dynamics/QuantityVector3.h"

namespace vectoris::dynamics {
    struct StateValidatedTag final {
    };

    // 刚体运动学状态（双坐标系绑定：ReferenceFrame 为导航/惯性系，BodyFrame 为机体系，强类型物理量）
    template<DynamicsScalar T, Geometry::FrameTag ReferenceFrame, Geometry::FrameTag BodyFrame>
    struct KinematicState final {
    public:
        // 位置：在参考系（如世界/导航系）中的坐标 (m)
        Position3<ReferenceFrame, T> position;

        // 姿态：机体系到参考系的四元数映射 (Body -> Reference)
        Geometry::Quaternion<T, BodyFrame, ReferenceFrame> attitude;

        // 线速度：在机体系下解析的 Velocity [u, v, w] (m/s)
        Velocity3<BodyFrame, T> linearVelocity;

        // 角速度：在机体系下解析的 Angular Velocity [p, q, r] (rad/s)
        AngularVelocity3<BodyFrame, T> angularVelocity;

    private:
        constexpr KinematicState(
            const Position3<ReferenceFrame, T> &pos,
            const Geometry::Quaternion<T, BodyFrame, ReferenceFrame> &att,
            const Velocity3<BodyFrame, T> &vel,
            const AngularVelocity3<BodyFrame, T> &angVel,
            StateValidatedTag) noexcept
            : position(pos), attitude(att), linearVelocity(vel), angularVelocity(angVel) {
        }

    public:
        KinematicState() = delete;

        static constexpr KinematicState Create(
            const Position3<ReferenceFrame, T> &pos,
            const Geometry::Quaternion<T, BodyFrame, ReferenceFrame> &att,
            const Velocity3<BodyFrame, T> &vel,
            const AngularVelocity3<BodyFrame, T> &angVel
        ) noexcept {
            return KinematicState(pos, att, vel, angVel, StateValidatedTag{});
        }
    };
} // namespace vectoris::dynamics