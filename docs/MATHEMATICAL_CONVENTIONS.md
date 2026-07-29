# AegisMathLib Mathematical Conventions

Version:

1.0


Status:

Foundation Architecture Specification


Applies to:

- Algebra
- Geometry
- Coordinate
- Navigation
- Physics
- Simulation


---

# 1. Purpose


This document defines the mathematical conventions used throughout AegisMathLib.


The purpose is to eliminate ambiguity between:

- coordinate systems
- transformations
- rotations
- vector operations
- physical quantities


All modules MUST follow these conventions.


---

# 2. Numerical Convention


## Floating Point Type


Default scalar type:


```cpp
using Scalar = double;
Reason:
aerospace simulation accuracy
navigation calculation stability
long propagation periods
Single precision may only be used explicitly.
Example:
Quantity<float, MeterUnit>
requires explicit declaration.
3. Coordinate System Convention
AegisMathLib uses:
Right-Handed Coordinate System
The positive rotation follows:
Right Hand Rule
3.1 Axis Convention
Default Cartesian frame:
        Z+
        |
        |
        |
        O------ X+
       /
      /
    Y+
Definition:
Axis	Direction
X	Forward
Y	Right
Z	Up

This is the default mathematical frame.
3.2 Aerospace Frame Convention
Aerospace systems frequently use:
NED Frame
North-East-Down:
        North(X)

          |
          |
          O------ East(Y)

         /
        /
      Down(Z)
Definition:
Axis	Direction
X	North
Y	East
Z	Down

NED is NOT identical to the default Cartesian frame.
Conversion MUST be explicit.
Example:
Transform<
    ENU,
    NED
>
4. Vector Convention
A vector represents:
direction + magnitude
Example:
Vector3<Velocity>
means:
vx
vy
vz
4.1 Vector Storage Order
All vectors use:
[X,Y,Z]
Memory layout:
struct Vector3
{
    Scalar x;
    Scalar y;
    Scalar z;
};
No padding fields are allowed.
4.2 Vector Operations
Addition
Allowed:
Vector + Vector
Example:
velocity + acceleration
only if dimensions match.
Forbidden:
Position + Position
A point and vector are different concepts.
5. Point Convention
Point represents:
absolute location
Example:
Aircraft position
Allowed:
Point + Vector = Point
Example:
position + displacement
Forbidden:
Point + Point
6. Matrix Convention
AegisMathLib uses:
Column Vector Convention
Vectors are represented as:
v =
[
x
y
z
]
6.1 Matrix Multiplication
Transformation:
v' = M * v
NOT:
v' = v * M
Example:
Rotation:
v_body = R_body_world * v_world
6.2 Matrix Storage
Internal storage:
Default:
Row-major
Reason:
cache efficiency
interoperability
External interfaces may convert explicitly.
7. Rotation Convention
Rotations use:
Active Rotation
Meaning:
A rotation transforms the object/vector.
Example:
v_rotated = R * v
Passive frame transformation requires inverse:
R_frameA_frameB =
(R_frameB_frameA)^T
8. Quaternion Convention
AegisMathLib uses:
Hamilton Quaternion
Format:
q = w + xi + yj + zk
Storage:
struct Quaternion
{
    Scalar w;
    Scalar x;
    Scalar y;
    Scalar z;
};
8.1 Quaternion Multiplication
Hamilton product:
q = q1 * q2
means:
apply q2 first,
then q1
Example:
q_total = q_rotation2 * q_rotation1
8.2 Quaternion Normalization
All rotation quaternions MUST satisfy:
|q| = 1
Before use:
q.normalize();
9. Euler Angle Convention
Euler angles are dangerous.
AegisMathLib requires explicit order.
Default aerospace convention:
ZYX
Meaning:
Yaw
 ↓
Pitch
 ↓
Roll
Equivalent:
R = Rz(yaw)
    *
    Ry(pitch)
    *
    Rx(roll)
9.1 Angle Units
All internal angles use:
Radians
Degrees require explicit conversion.
Example:
auto rad =
degree_cast<Radian>(angle);
10. Coordinate Transformation Convention
All transformations must specify:
Source Frame

Target Frame
Example:
Transform<
    WorldFrame,
    BodyFrame
>
Meaning:
World coordinates
        |
        v
Body coordinates
10.1 Transform Composition
Given:
A -> B

B -> C
combined:
A -> C
is:
Tca = Tcb * Tba
11. Physical Quantity Convention
All physical values MUST use Units.
Forbidden:
double altitude;
Correct:
Length altitude;
11.1 SI Base System
Internal representation:
SI Units
Examples:
meter
second
kilogram
radian
12. Gravity Convention
Default gravity:
+Z upward
Therefore:
Gravity acceleration:
g = [0,0,-9.80665]
in Cartesian frame.
13. Navigation Convention
Navigation systems may use:
ECEF

ECI

ENU

NED

Body
Each frame MUST be represented as a unique type.
Example:
Position<ECEF>

Position<NED>
Conversion:
convert<ECEF,NED>();
14. Radar Coordinate Convention
Radar systems commonly use:
Spherical coordinates:
Range
Azimuth
Elevation
Definition:
Range:
distance from sensor


Azimuth:
rotation around Z axis


Elevation:
angle above horizon
Angles:
Radians
15. Forbidden Practices
The following are prohibited:
Implicit Coordinate Conversion
Forbidden:
ECEFPosition p = nedPosition;
Raw Angle Usage
Forbidden:
double heading;
Use:
Angle heading;
Unspecified Rotation
Forbidden:
Quaternion q;
without declaring:
frame
direction
16. API Requirement
Every mathematical type must document:
coordinate frame
unit
storage order
multiplication direction
rotation convention
17. Summary
AegisMathLib global conventions:
Category	Standard
Coordinate System	Right-handed
Vector	Column vector
Matrix	Row-major storage
Matrix Operation	M*v
Rotation	Active
Quaternion	Hamilton
Quaternion Order	w,x,y,z
Euler	ZYX
Angle	Radian
Units	SI
Gravity	-Z
Scalar	double

These conventions are mandatory for all future modules.

---