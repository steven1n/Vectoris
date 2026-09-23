// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    using M = vectoris::numerics::geometry::Matrix3<T>;
    static_assert(std::is_same_v<M, vectoris::numerics::Geometry::Matrix3<T>>);
    [[maybe_unused]] const auto matrix = M::Identity();
};
probe.template operator()<float>();
probe.template operator()<double>();
