#pragma once
#include "BaseUnits/Length.h"
#include "BaseUnits/Time.h"
#include "BaseUnits/Angle.h"

namespace AegisMath::Units::Literals {
    constexpr Meter operator""_m(long double val) noexcept { return Meter(static_cast<Scalar>(val)); }
    constexpr Meter operator""_m(unsigned long long val) noexcept { return Meter(static_cast<Scalar>(val)); }

    constexpr Second operator""_s(long double val) noexcept { return Second(static_cast<Scalar>(val)); }
    constexpr Second operator""_s(unsigned long long val) noexcept { return Second(static_cast<Scalar>(val)); }

    constexpr Radian operator""_rad(long double val) noexcept { return Radian(static_cast<Scalar>(val)); }
    constexpr Radian operator""_rad(unsigned long long val) noexcept { return Radian(static_cast<Scalar>(val)); }
}
