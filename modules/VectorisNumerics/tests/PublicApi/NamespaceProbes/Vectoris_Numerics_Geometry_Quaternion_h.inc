// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Frame {};
    using Q = vectoris::numerics::geometry::Quaternion<T, Frame, Frame>;
    static_assert(std::is_same_v<Q, vectoris::numerics::Geometry::Quaternion<T, Frame, Frame>>);
    [[maybe_unused]] const auto rotation = Q::TryCreate(T{1}, T{0}, T{0}, T{0});
};
probe.template operator()<float>();
probe.template operator()<double>();
