// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Unit { using Dimension = vectoris::numerics::units::LengthDimension; using Ratio = std::ratio<1>; enum { IsBaseUnit = true }; };
    const vectoris::numerics::units::Quantity<T, Unit> quantity{T{1}};
    [[maybe_unused]] const auto converted = vectoris::numerics::units::unit_cast<Unit>(quantity);
    static_assert(&vectoris::numerics::units::unit_cast<Unit, T, Unit> == &vectoris::numerics::Units::unit_cast<Unit, T, Unit>);
};
probe.template operator()<float>();
probe.template operator()<double>();
