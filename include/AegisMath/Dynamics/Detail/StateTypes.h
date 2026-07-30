#pragma once

#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Geometry/Point3.h"
#include "AegisMath/Geometry/Quaternion.h"
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Dynamics/Concepts.h"
#include "AegisMath/Dynamics/Detail/DynamicsABI.h"

namespace AegisMath::Dynamics {
    struct StateValidatedTag final {
    };

    // 刚体运动学状态（双坐标系绑定：ReferenceFrame 为导航/惯性系，BodyFrame 为机体系）
    template<DynamicsScalar T, Geometry::FrameTag ReferenceFrame, Geometry::FrameTag BodyFrame>
    struct KinematicState final {
    public:
        // 位置：在参考系（如世界/导航系）中的坐标
        Geometry::Point3<T, ReferenceFrame> position;

        // 姿态：机体系到参考系的四元数映射 (Body -> Reference)
        Geometry::Quaternion<T, BodyFrame, ReferenceFrame> attitude;

        // 线速度：在机体系下解析的 Velocity [u, v, w]
        Geometry::Vector3<T, BodyFrame> linearVelocity;

        // 角速度：在机体系下解析的 Angular Velocity [p, q, r]
        Geometry::Vector3<T, BodyFrame> angularVelocity;

    private:
        constexpr KinematicState(
            const Geometry::Point3<T, ReferenceFrame> &pos,
            const Geometry::Quaternion<T, BodyFrame, ReferenceFrame> &att,
            const Geometry::Vector3<T, BodyFrame> &vel,
            const Geometry::Vector3<T, BodyFrame> &angVel,
            StateValidatedTag) noexcept
            : position(pos), attitude(att), linearVelocity(vel), angularVelocity(angVel) {
        }

    public:
        KinematicState() = delete;

        static constexpr KinematicState Create(
            const Geometry::Point3<T, ReferenceFrame> &pos,
            const Geometry::Quaternion<T, BodyFrame, ReferenceFrame> &att,
            const Geometry::Vector3<T, BodyFrame> &vel,
            const Geometry::Vector3<T, BodyFrame> &angVel
        ) noexcept {
            return KinematicState(pos, att, vel, angVel, StateValidatedTag{});
        }
    };
} // namespace AegisMath::Dynamics