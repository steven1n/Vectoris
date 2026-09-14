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

# 18. Rotational Dimensional Analysis & Angle Base Dimension Convention

Per Engineering Standard Section 10–13, AegisMathLib treats Plane Angle ($A = \text{Angle}$) as an independent semantic physical base dimension within its 8-dimensional system (`Length`, `Mass`, `Time`, `Current`, `Temperature`, `Amount`, `Luminosity`, `Angle`).

To preserve dimensional closure across rotational kinematics, dynamics, kinetic energy, work, torque, and power without algebraic inconsistency, rotational mechanical quantities carry compensating inverse-angle exponents:

1. **Angular Velocity ($\boldsymbol{\omega}$)**:
   - Dimension: $[A \cdot T^{-1}]$
   - Unit: $\text{rad}/\text{s}$
2. **Angular Acceleration ($\boldsymbol{\alpha}$)**:
   - Dimension: $[A \cdot T^{-2}]$
   - Unit: $\text{rad}/\text{s}^2$
3. **Rotational Kinetic Energy ($E = \frac{1}{2} I \omega^2$)**:
   - Dimension: $[M \cdot L^2 \cdot T^{-2}]$ (Joule)
   - Since $[\omega^2] = [A^2 \cdot T^{-2}]$, Moment of Inertia must carry $A^{-2}$:
4. **Moment of Inertia ($I$)**:
   - Dimension: $[M \cdot L^2 \cdot A^{-2}]$
   - Unit: $\text{kg}\cdot\text{m}^2/\text{rad}^2$
5. **Torque ($\tau = dE / d\theta = I \alpha$)**:
   - Dimension: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$
   - Unit: $\text{N}\cdot\text{m}/\text{rad}$
6. **Rotational Mechanical Power ($P = \boldsymbol{\tau} \cdot \boldsymbol{\omega}$)**:
   - $[P] = [\tau] \cdot [\omega] = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] \cdot [A \cdot T^{-1}] = [M \cdot L^2 \cdot T^{-3}]$ (Watt)
7. **Newton-Euler Rotational Law**:
   - $[I \cdot \alpha] = [M \cdot L^2 \cdot A^{-2}] \cdot [A \cdot T^{-2}] = [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}] = [\tau]$

### Dimensional Separation of Torque and Energy
In standard SI dimensional systems where angle is treated as dimensionless (1), Torque and Energy share the identical dimension $[M \cdot L^2 \cdot T^{-2}]$. Under AegisMathLib's 8-dimensional Model B:
- **Energy / Work**: $[M \cdot L^2 \cdot T^{-2} \cdot A^0]$ ($\text{J}$)
- **Torque**: $[M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$ ($\text{N}\cdot\text{m}/\text{rad}$)
This compile-time separation prevents assigning translational energy or work directly to torque (or vice versa), eliminating common dimensional errors in aerospace GNC code.

### Quaternion Kinematics Boundary
Quaternion components are dimensionless scalars in $\mathbb{R}^4$ with unit constraint $\|\mathbf{q}\| = 1$. In quaternion kinematic integration ($\dot{\mathbf{q}} = \frac{1}{2} \mathbf{q} \otimes \boldsymbol{\omega}$), rotational rates typed in $[\text{rad}/\text{s}]$ ($[A \cdot T^{-1}]$) are explicitly extracted as numerical scalars in radians per second at the local, auditable boundary to drive quaternion integration without implicit dimensionless demotion.

---

# 19. Dimensionful Angle and SO(3) Lie Algebra

When Plane Angle is treated as an independent physical dimension ($A$), the ordinary Cartesian cross product and the $\mathfrak{so}(3)$ Lie bracket / adjoint operation represent dimensionally distinct operations.

### 19.1 Ordinary Cross Product vs. Lie Bracket
- **Ordinary Vector Cross Product**:
  For general 3D vectors $\mathbf{u}, \mathbf{v}$, the cross product computes Cartesian components via $\mathbf{u} \times \mathbf{v}$. Its dimension is the strict product of the operand dimensions:
  $$[\mathbf{u} \times \mathbf{v}] = [\mathbf{u}] \cdot [\mathbf{v}]$$
- **Rotational Lie Bracket / Adjoint Operation ($\mathfrak{so}(3)$)**:
  In rigid body dynamics, the gyroscopic cross-coupling term $\boldsymbol{\omega} \times \mathbf{L}$ arises from the Lie algebra adjoint action $\operatorname{ad}_{\boldsymbol{\omega}} \mathbf{L} = [\boldsymbol{\omega}, \mathbf{L}]_{\mathfrak{so}(3)}$.
  Evaluating dimensions with Angle $A$:
  $$[\boldsymbol{\omega}] = [A \cdot T^{-1}], \quad [\mathbf{L}] = [M \cdot L^2 \cdot A^{-1} \cdot T^{-1}]$$
  The ordinary component-wise cross product yields:
  $$[\boldsymbol{\omega} \times \mathbf{L}] = [A \cdot T^{-1}] \cdot [M \cdot L^2 \cdot A^{-1} \cdot T^{-1}] = [M \cdot L^2 \cdot T^{-2}]$$
  This has dimension of **Energy / Work ($A^0$)**, NOT **Torque ($A^{-1}$)**.

### 19.2 Explicit Radian Normalization Factor
Standard engineering textbooks treat the radian as dimensionless ($1$), implicitly suppressing the normalization factor $1 / \text{rad}$. Under AegisMathLib's rigorous 8-dimensional type system, this hidden convention is forbidden.

The true Lie bracket operation in dimensionful mechanics carries an explicit normalization by $1\text{ rad}$ ($[A^1]$):
$$\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L}) \triangleq \frac{\boldsymbol{\omega} \times \mathbf{L}}{1\text{ rad}}$$
Dimensionally:
$$[\operatorname{RotationalCross}(\boldsymbol{\omega}, \mathbf{L})] = \frac{[M \cdot L^2 \cdot T^{-2}]}{[A^1]} = [M \cdot L^2 \cdot A^{-1} \cdot T^{-2}] \equiv [\boldsymbol{\tau}]$$

### 19.3 Compile-Time Type Safety Directives
1. **Generic Cross Product Preservation**: `Cross(a, b)` remains a pure Cartesian vector product. It must never implicitly divide by radians.
2. **Dedicated Rotational Operator**: Rotational dynamics code MUST call `RotationalCross(omega, L)` (or `LieBracket(omega, L)`).
3. **Compile-Time Static Guard**: The type system statically prevents subtracting `Cross(omega, L)` from `Torque3`, because $[M \cdot L^2 \cdot T^{-2} \cdot A^0] \neq [M \cdot L^2 \cdot T^{-2} \cdot A^{-1}]$.
4. **Consistency with Quaternion Boundary**: $SO(3)$ rotational kinematics similarly consume dimensionless coordinates by explicitly extracting radian scalars at the integration boundary, ensuring total consistency across the library.

---

# 20. Geometry Semantics, Matrix3 Scope, and Floating-Point Comparisons

### 20.1 Matrix3 Scope & Frame Separation
- **Unframed Generic Numerical Matrix**: `Matrix3<T>` is a generic numerical matrix container in $\mathbb{R}^{3 \times 3}$. It may represent coefficients, Jacobians, covariance blocks, solver matrices, or temporarily extracted tensor coefficients. It does not itself imply a geometric frame transformation and intentionally carries no coordinate frame tags (`FromFrame`, `ToFrame`, or `FrameType`).
- **Generic Linear Algebra Preservation**: Pure numerical routines—such as the $LDL^T$ symmetric positive definite solver (`SolveSymmetricPositiveDefinite3x3`), matrix determinants, adjoint inverses, covariances, and Jacobians—MUST accept `Matrix3<T>` without coordinate frame tags.
- **Dedicated Spatial Transformation Wrappers**: Coordinate frame semantics (`FromFrame -> ToFrame`) are strictly carried by semantic geometric types:
  - `RotationMatrix3<T, FromFrame, ToFrame>`
  - `Transform3<T, FromFrame, ToFrame>`
  - `Quaternion<T, FromFrame, ToFrame>`
- **Typed Inertia Bridge**: Inertia tensors (`InertiaTensor3<T, BodyFrame>`) hold coordinate frame tags; they construct local `Matrix3<T>` instances purely to invoke numerical solvers, preventing frame tags from polluting general linear algebra.

### 20.2 Coordinate Transformation & Composition Convention
- **Vector Transformation**:
  $$\mathbf{v}_{\text{Target}} = \mathbf{R}_{\text{Source} \to \text{Target}} \mathbf{v}_{\text{Source}}$$
- **Rotation Composition (Pipeline Convention)**:
  `operator*` on `RotationMatrix3` represents left-to-right frame transformation pipeline composition, not raw `Matrix3` multiplication order:
  $$\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C} \to \mathbf{R}_{A \to C}$$
  In standard linear algebra, the transformation pipeline evaluates:
  $$\mathbf{v}_C = \mathbf{M}_{BC} (\mathbf{M}_{AB} \mathbf{v}_A) = (\mathbf{M}_{BC} \mathbf{M}_{AB}) \mathbf{v}_A$$
  Therefore, the underlying Direction Cosine Matrix product evaluates $\mathbf{M}_{AC} = \mathbf{M}_{BC} \mathbf{M}_{AB}$ (`rhs.ToMatrix() * dcm_`).
  The composition satisfies pipeline associativity when applied to vectors:
  $$(\mathbf{R}_{A \to B} * \mathbf{R}_{B \to C}) * \mathbf{v}_A \approx \mathbf{R}_{B \to C} * (\mathbf{R}_{A \to B} * \mathbf{v}_A)$$
- **Quaternion to Rotation Matrix Agreement**:
  For any unit quaternion $\mathbf{q} \in SO(3)$ and vector $\mathbf{v}$:
  $$\mathbf{q} * \mathbf{v} \equiv \mathbf{R}(\mathbf{q}) * \mathbf{v}$$

### 20.3 Floating-Point Equality & Comparison Policy
In compliance with Rule 9 ("Never trust floating-point equality"):
1. **Exact Component-Wise Stored-Value Equality (`operator==`, `operator!=`)**:
   - Compares stored IEEE-754 components directly under standard C++ floating-point `==` semantics (`a.x == b.x && a.y == b.y && a.z == b.z`).
   - It is exact component-wise value equality, NOT bitwise equality:
     - $+0.0 == -0.0$ evaluates to `true` despite differing sign bits.
     - $\text{NaN} == \text{NaN}$ evaluates to `false`.
   - Reserved strictly for exact value checks: sentinel states, exact identity matrix initialization, serialization round-trips, and transactional immutability assertions.
   - FORBIDDEN for numerical convergence, solver residuals, or computed geometry comparisons.
2. **Tolerance-Aware Closeness (`AlmostEqual`)**:
   - All geometry types (`Vector3`, `Point3`, `Matrix3`, `UnitVector3`, `RotationMatrix3`, `Transform3`, `Quaternion`) provide `AlmostEqual(a, b, abs_tol, rel_tol)` delegating to `Traits::AlmostEqual`.
   - Thresholds combine absolute and relative tolerances: $|a - b| \le \max(\text{abs\_tol}, \text{rel\_tol} \times \max(|a|, |b|))$.
3. **Rotational Equivalence (`RotationEquivalent`) & Canonicalization**:
   - Quaternions represent $SO(3)$ rotations via a double cover ($\mathbb{S}^3 \to SO(3)$), meaning $\mathbf{q}$ and $-\mathbf{q}$ represent the identical physical orientation.
   - **Canonical Representation**: Public construction via `Quaternion::TryCreate` enforces a canonical sign representation ($w \ge 0$), collapsing the double cover to a single canonical representative at construction.
   - **Sign-Invariant Comparison**: `RotationEquivalent(q1, q2)` checks both $\mathbf{q}_1 \approx \mathbf{q}_2$ and $\mathbf{q}_1 \approx -\mathbf{q}_2$, providing robust tolerance-aware equivalence for un-canonicalized, legacy, or numerical sign-flipped instances.
4. **Rotation Invariant Tolerance**:
   - The orthogonality check $\mathbf{R}^T \mathbf{R} \approx \mathbf{I}$ uses the squared Frobenius norm:
     $$\|\mathbf{R}^T \mathbf{R} - \mathbf{I}\|_F^2 \le \tau^2$$
     guaranteeing exact scale consistency between linear tolerance $\tau$ and the quadratic norm.