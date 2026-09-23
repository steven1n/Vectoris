// VRT-12: direct-header symbol lookup and compatibility for both scalar types.
const auto probe = []<typename T>() {
    struct Frame {};
    [[maybe_unused]] const auto solved = vectoris::numerics::geometry::SolveSymmetricPositiveDefinite3x3(vectoris::numerics::geometry::Matrix3<T>::Identity(), vectoris::numerics::geometry::Vector3<T, Frame>{T{1}, T{2}, T{3}});
    static_assert(&vectoris::numerics::geometry::SolveSymmetricPositiveDefinite3x3<T, Frame> == &vectoris::numerics::Geometry::SolveSymmetricPositiveDefinite3x3<T, Frame>);
};
probe.template operator()<float>();
probe.template operator()<double>();
