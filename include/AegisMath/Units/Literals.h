#pragma once
#include "BaseUnits/Length.h"
#include "BaseUnits/Time.h"
#include "BaseUnits/Angle.h"

namespace AegisMath::Units::Literals {
    consteval Length::Meter operator""_m(long double val) { return Length::Meter(static_cast<Scalar>(val)); }
    consteval Length::Meter operator""_m(unsigned long long val) { return Length::Meter(static_cast<Scalar>(val)); }
    
    consteval Time::Second operator""_s(long double val) { return Time::Second(static_cast<Scalar>(val)); }
    consteval Time::Second operator""_s(unsigned long long val) { return Time::Second(static_cast<Scalar>(val)); }
}
