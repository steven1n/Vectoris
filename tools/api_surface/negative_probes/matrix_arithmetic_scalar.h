#pragma once
#include <Vectoris/Numerics/Geometry/Matrix3.h>
struct ThrowingScalar {
 double v;
 ThrowingScalar(double x=0):v(x){}
 friend ThrowingScalar operator+(ThrowingScalar a,ThrowingScalar b){return a.v+b.v;}
 friend ThrowingScalar operator-(ThrowingScalar a,ThrowingScalar b){return a.v-b.v;}
 friend ThrowingScalar operator*(ThrowingScalar a,ThrowingScalar b){return a.v*b.v;}
 friend ThrowingScalar operator/(ThrowingScalar a,ThrowingScalar b){return a.v/b.v;}
 friend ThrowingScalar operator-(ThrowingScalar a){return -a.v;}
};
using M=vectoris::numerics::geometry::Matrix3<ThrowingScalar>;
