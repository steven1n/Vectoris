// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Unit { using Dimension = vectoris::numerics::units::LengthDimension; using Ratio = std::ratio<1>; enum { IsBaseUnit = true }; };
    static_assert(std::is_same_v<typename Unit::Dimension, vectoris::numerics::units::LengthDimension>);
    static_assert(std::is_same_v<typename Unit::Ratio, std::ratio<1>>);
    using Q = vectoris::numerics::units::Quantity<T, Unit>;
    static_assert(std::is_same_v<Q, vectoris::numerics::Units::Quantity<T, Unit>>);
    [[maybe_unused]] const Q quantity{T{1}};
};
probe.template operator()<float>();
probe.template operator()<double>();
