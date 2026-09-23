#pragma once
#include "Namespace.h"
#include "BaseUnits/Length.h"
#include "BaseUnits/Time.h"
#include "BaseUnits/Angle.h"
#include "BaseUnits/Mass.h"
#include "DerivedUnits/Force.h"
#include "DerivedUnits/Torque.h"
#include "DerivedUnits/Power.h"
#include "DerivedUnits/Velocity.h"
#include "DerivedUnits/AngularVelocity.h"
#include "DerivedUnits/AngularMomentum.h"

namespace vectoris::numerics::Units::Literals {
    constexpr Meter operator""_m(long double val) noexcept { return Meter(static_cast<Scalar>(val)); }
    constexpr Meter operator""_m(unsigned long long val) noexcept { return Meter(static_cast<Scalar>(val)); }

    constexpr Second operator""_s(long double val) noexcept { return Second(static_cast<Scalar>(val)); }
    constexpr Second operator""_s(unsigned long long val) noexcept { return Second(static_cast<Scalar>(val)); }

    constexpr Radian operator""_rad(long double val) noexcept { return Radian(static_cast<Scalar>(val)); }
    constexpr Radian operator""_rad(unsigned long long val) noexcept { return Radian(static_cast<Scalar>(val)); }

    constexpr Kilogram operator""_kg(long double val) noexcept { return Kilogram(static_cast<Scalar>(val)); }
    constexpr Kilogram operator""_kg(unsigned long long val) noexcept { return Kilogram(static_cast<Scalar>(val)); }

    constexpr Force operator""_N(long double val) noexcept { return Force(static_cast<Scalar>(val)); }
    constexpr Force operator""_N(unsigned long long val) noexcept { return Force(static_cast<Scalar>(val)); }

    constexpr Torque operator""_Nm(long double val) noexcept { return Torque(static_cast<Scalar>(val)); }
    constexpr Torque operator""_Nm(unsigned long long val) noexcept { return Torque(static_cast<Scalar>(val)); }

    constexpr Watt operator""_W(long double val) noexcept { return Watt(static_cast<Scalar>(val)); }
    constexpr Watt operator""_W(unsigned long long val) noexcept { return Watt(static_cast<Scalar>(val)); }

    constexpr Velocity operator""_mps(long double val) noexcept { return Velocity(static_cast<Scalar>(val)); }
    constexpr Velocity operator""_mps(unsigned long long val) noexcept { return Velocity(static_cast<Scalar>(val)); }

    constexpr AngularVelocity operator""_rad_s(long double val) noexcept { return AngularVelocity(static_cast<Scalar>(val)); }
    constexpr AngularVelocity operator""_rad_s(unsigned long long val) noexcept { return AngularVelocity(static_cast<Scalar>(val)); }
}
