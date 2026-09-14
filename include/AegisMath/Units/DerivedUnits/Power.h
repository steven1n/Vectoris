#pragma once
#include "../Quantity.h"
#include "../BaseUnits/Mass.h"
#include "../BaseUnits/Length.h"
#include "../BaseUnits/Time.h"
#include "Force.h"
#include "Velocity.h"
#include "Torque.h"
#include "AngularVelocity.h"
#include "../Detail/ABI.h"

namespace AegisMath::Units {

    struct WattUnit {
        using Dimension = PowerDimension;
        using Ratio     = std::ratio<1>;
        static constexpr bool IsBaseUnit = false;
    };

    using Watt  = Quantity<Scalar, WattUnit>;
    using Power = Watt;

    static_assert(Detail::ValidateQuantityABI<Power>(), "Power ABI contract violation!");

    // 转动功率: Torque * AngularVelocity -> Power
    template <Concepts::FloatingPoint T>
    [[nodiscard]] constexpr Quantity<T, WattUnit> RotationalPower(
        const Quantity<T, NewtonMeterUnit>& tau,
        const Quantity<T, RadianPerSecondUnit>& omega) noexcept
    {
        return Quantity<T, WattUnit>(tau.value() * omega.value());
    }

} // namespace AegisMath::Units
