#include "matrix_arithmetic_scalar.h"
int main(){const M a=M::Identity();static_cast<void>(a.AlmostEqual(a,ThrowingScalar{0.1},ThrowingScalar{0.1}));}
