// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Frame {};
    using V = vectoris::numerics::geometry::Vector3<T, Frame>;
    static_assert(std::is_same_v<V, vectoris::numerics::Geometry::Vector3<T, Frame>>);
    [[maybe_unused]] const V vector{T{1}, T{2}, T{3}};
};
probe.template operator()<float>();
probe.template operator()<double>();
