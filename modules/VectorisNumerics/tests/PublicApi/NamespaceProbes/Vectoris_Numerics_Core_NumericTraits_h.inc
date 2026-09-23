// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    [[maybe_unused]] const bool same = vectoris::numerics::core::AlmostEqual(T{1}, T{1}, T{0}, T{0});
    static_assert(&vectoris::numerics::core::AlmostEqual<T> == &vectoris::numerics::Traits::AlmostEqual<T>);
    static_assert(&vectoris::numerics::core::AlmostEqual<T> == &vectoris::numerics::Core::AlmostEqual<T>);
};
probe.template operator()<float>();
probe.template operator()<double>();
