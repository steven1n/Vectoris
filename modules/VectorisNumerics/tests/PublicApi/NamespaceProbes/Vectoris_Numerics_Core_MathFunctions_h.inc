// VRT-14: compatibility sqrt is independently usable from its own public header.
const auto probe = []<typename T>() {
    static_assert(vectoris::numerics::core::Math::sqrt(T{4}) == T{2});
    static_assert(&vectoris::numerics::core::Math::sqrt<T> == &vectoris::numerics::Core::Math::sqrt<T>);
};
probe.template operator()<float>();
probe.template operator()<double>();
