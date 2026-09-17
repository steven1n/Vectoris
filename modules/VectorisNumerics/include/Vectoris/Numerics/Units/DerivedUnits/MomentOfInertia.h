#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Mass.h"
#include "../BaseUnits/Length.h"
#include "Torque.h"
#include "AngularAcceleration.h"
#include "../Detail/ABI.h"

namespace vectoris::numerics::Units {

    struct KilogramMeterSquaredUnit {
        using Dimension = MomentOfInertiaDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using KilogramMeterSquared = Quantity<Scalar, KilogramMeterSquaredUnit>;
    using MomentOfInertia      = KilogramMeterSquared;

    static_assert(Detail::ValidateQuantityABI<MomentOfInertia>(), "MomentOfInertia ABI contract violation!");

    // 刚体转动动力学: Inertia * AngularAcceleration -> Torque
    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Quantity<T, NewtonMeterUnit> RotationalTorque(
        const Quantity<T, KilogramMeterSquaredUnit>& I,
        const Quantity<T, RadianPerSecondSquaredUnit>& alpha) noexcept
    {
        return I * alpha;
    }

    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Quantity<T, RadianPerSecondSquaredUnit> RotationalAcceleration(
        const Quantity<T, NewtonMeterUnit>& tau,
        const Quantity<T, KilogramMeterSquaredUnit>& I) noexcept
    {
        return tau / I;
    }

    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Quantity<T, KilogramMeterSquaredUnit> RotationalInertia(
        const Quantity<T, NewtonMeterUnit>& tau,
        const Quantity<T, RadianPerSecondSquaredUnit>& alpha) noexcept
    {
        return tau / alpha;
    }

} // namespace vectoris::numerics::Units
