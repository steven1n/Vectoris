// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    using R = vectoris::numerics::core::Result<T, int>;
    static_assert(std::is_same_v<R, vectoris::numerics::Core::Result<T, int>>);
    [[maybe_unused]] const auto result = R::success(T{1});
};
probe.template operator()<float>();
probe.template operator()<double>();
