// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    static_assert(std::is_same_v<typename Unit::Dimension, vectoris::numerics::units::LengthDimension>);
    static_assert(std::is_same_v<typename Unit::Ratio, std::ratio<1>>);
    const vectoris::numerics::units::Quantity<T, Unit> quantity{T{1}};
    [[maybe_unused]] const auto converted = vectoris::numerics::units::unit_cast<Unit>(quantity);
    static_assert(&vectoris::numerics::units::unit_cast<Unit, T, Unit> == &vectoris::numerics::Units::unit_cast<Unit, T, Unit>);
};
probe.template operator()<float>();
probe.template operator()<double>();
