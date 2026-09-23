// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    using Q = vectoris::numerics::units::Quantity<T, vectoris::numerics::units::MeterUnit>;
    static_assert(std::is_same_v<Q, vectoris::numerics::Units::Quantity<T, vectoris::numerics::Units::MeterUnit>>);
    [[maybe_unused]] const Q length{T{1}};
};
probe.template operator()<float>();
probe.template operator()<double>();
