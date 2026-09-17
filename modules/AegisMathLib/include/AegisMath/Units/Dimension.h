#pragma once
#include <type_traits>
#include <ratio>

namespace AegisMath::Units {

    template<
        int Length,
        int Mass,
        int Time,
        int Current,
        int Temperature,
        int Amount,
        int Luminosity,
        int Angle
    >
    struct Dimension {
        static constexpr int length      = Length;
        static constexpr int mass        = Mass;
        static constexpr int time        = Time;
        static constexpr int current     = Current;
        static constexpr int temperature = Temperature;
        static constexpr int amount      = Amount;
        static constexpr int luminosity  = Luminosity;
        static constexpr int angle       = Angle;
    };

    using Dimensionless       = Dimension<0,0,0,0,0,0,0,0>;
    using LengthDimension     = Dimension<1,0,0,0,0,0,0,0>;
    using MassDimension       = Dimension<0,1,0,0,0,0,0,0>;
    using TimeDimension       = Dimension<0,0,1,0,0,0,0,0>;
    using CurrentDimension    = Dimension<0,0,0,1,0,0,0,0>;
    using TempDimension       = Dimension<0,0,0,0,1,0,0,0>;
    using AmountDimension     = Dimension<0,0,0,0,0,1,0,0>;
    using LumDimension        = Dimension<0,0,0,0,0,0,1,0>;
    using AngleDimension      = Dimension<0,0,0,0,0,0,0,1>;

    using VelocityDimension     = Dimension<1, 0, -1, 0, 0, 0, 0, 0>;
    using AccelerationDimension = Dimension<1, 0, -2, 0, 0, 0, 0, 0>;
    using ForceDimension        = Dimension<1, 1, -2, 0, 0, 0, 0, 0>;
    using FrequencyDimension    = Dimension<0, 0, -1, 0, 0, 0, 0, 0>; // 频率量纲 (T^-1)
    using AngularVelocityDimension     = Dimension<0, 0, -1, 0, 0, 0, 0,  1>;
    using AngularAccelerationDimension = Dimension<0, 0, -2, 0, 0, 0, 0,  1>;
    using AngularMomentumDimension     = Dimension<2, 1, -1, 0, 0, 0, 0, -1>;
    using TorqueDimension              = Dimension<2, 1, -2, 0, 0, 0, 0, -1>;
    using MomentOfInertiaDimension     = Dimension<2, 1,  0, 0, 0, 0, 0, -2>;
    using PowerDimension               = Dimension<2, 1, -3, 0, 0, 0, 0,  0>;
    using EnergyDimension              = Dimension<2, 1, -2, 0, 0, 0, 0,  0>;

    using LengthDim = LengthDimension;
    using MassDim   = MassDimension;
    using TimeDim   = TimeDimension;
    using CurrentDim = CurrentDimension;
    using TempDim   = TempDimension;
    using AmountDim = AmountDimension;
    using LumDim    = LumDimension;
    using AngleDim  = AngleDimension;
    using VelocityDim = VelocityDimension;
    using AccelerationDim = AccelerationDimension;
    using FrequencyDim  = FrequencyDimension;
    using AngularVelocityDim     = AngularVelocityDimension;
    using AngularAccelerationDim = AngularAccelerationDimension;
    using AngularMomentumDim     = AngularMomentumDimension;
    using TorqueDim              = TorqueDimension;
    using MomentOfInertiaDim     = MomentOfInertiaDimension;
    using PowerDim               = PowerDimension;
    using EnergyDim              = EnergyDimension;

    template <typename D1, typename D2>
    struct DimensionAdd {
        using type = Dimension<
            D1::length      + D2::length,
            D1::mass        + D2::mass,
            D1::time        + D2::time,
            D1::current     + D2::current,
            D1::temperature + D2::temperature,
            D1::amount      + D2::amount,
            D1::luminosity  + D2::luminosity,
            D1::angle       + D2::angle
        >;
    };

    template <typename D1, typename D2>
    struct DimensionSubtract {
        using type = Dimension<
            D1::length      - D2::length,
            D1::mass        - D2::mass,
            D1::time        - D2::time,
            D1::current     - D2::current,
            D1::temperature - D2::temperature,
            D1::amount      - D2::amount,
            D1::luminosity  - D2::luminosity,
            D1::angle       - D2::angle
        >;
    };

    template <typename D1, typename D2>
    using DimensionAdd_t = typename DimensionAdd<D1, D2>::type;

    template <typename D1, typename D2>
    using DimensionSubtract_t = typename DimensionSubtract<D1, D2>::type;

    template <typename D1, typename D2>
    concept DimensionEqual = (
        D1::length      == D2::length &&
        D1::mass        == D2::mass &&
        D1::time        == D2::time &&
        D1::current     == D2::current &&
        D1::temperature == D2::temperature &&
        D1::amount      == D2::amount &&
        D1::luminosity  == D2::luminosity &&
        D1::angle       == D2::angle
    );

} // namespace AegisMath::Units