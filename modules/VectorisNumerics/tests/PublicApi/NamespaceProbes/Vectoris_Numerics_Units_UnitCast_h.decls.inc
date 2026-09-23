// Required unit-tag members live outside the function-local typedef scope.
namespace {
struct Unit {
    using Dimension = vectoris::numerics::units::LengthDimension;
    using Ratio = std::ratio<1>;
    enum { IsBaseUnit = true };
};
}
