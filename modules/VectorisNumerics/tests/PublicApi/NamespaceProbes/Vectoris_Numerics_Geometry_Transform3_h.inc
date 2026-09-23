// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Frame {};
    using X = vectoris::numerics::geometry::Transform3<T, Frame, Frame>;
    static_assert(std::is_same_v<X, vectoris::numerics::Geometry::Transform3<T, Frame, Frame>>);
    [[maybe_unused]] const auto transform = X::Identity();
};
probe.template operator()<float>();
probe.template operator()<double>();
