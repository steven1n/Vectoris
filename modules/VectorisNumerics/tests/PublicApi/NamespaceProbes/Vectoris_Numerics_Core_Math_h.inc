// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    static_assert(vectoris::numerics::core::sqrt(T{4}) == T{2});
    static_assert(&vectoris::numerics::core::sqrt<T> == &vectoris::numerics::Core::sqrt<T>);
};
probe.template operator()<float>();
probe.template operator()<double>();
