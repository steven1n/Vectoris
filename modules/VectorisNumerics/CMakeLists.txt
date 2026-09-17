# ==============================================================================
# VectorisNumerics Module CMake Configuration
# Pure Mathematics & Numerical Computation Library
# ==============================================================================

set(VECTORIS_NUMERICS_PUBLIC_HEADERS
        # Core (10)
        include/Vectoris/Numerics/Core/BasicTypes.h
        include/Vectoris/Numerics/Core/Compiler.h
        include/Vectoris/Numerics/Core/Concepts.h
        include/Vectoris/Numerics/Core/Constants.h
        include/Vectoris/Numerics/Core/Math.h
        include/Vectoris/Numerics/Core/MathError.h
        include/Vectoris/Numerics/Core/MathFunctions.h
        include/Vectoris/Numerics/Core/NumericTraits.h
        include/Vectoris/Numerics/Core/Precision.h
        include/Vectoris/Numerics/Core/Result.h

        # Units (30)
        include/Vectoris/Numerics/Units/BaseUnits/Amount.h
        include/Vectoris/Numerics/Units/BaseUnits/Angle.h
        include/Vectoris/Numerics/Units/BaseUnits/Current.h
        include/Vectoris/Numerics/Units/BaseUnits/Length.h
        include/Vectoris/Numerics/Units/BaseUnits/Luminosity.h
        include/Vectoris/Numerics/Units/BaseUnits/Mass.h
        include/Vectoris/Numerics/Units/BaseUnits/Temperature.h
        include/Vectoris/Numerics/Units/BaseUnits/Time.h
        include/Vectoris/Numerics/Units/DerivedUnits/Acceleration.h
        include/Vectoris/Numerics/Units/DerivedUnits/AngularAcceleration.h
        include/Vectoris/Numerics/Units/DerivedUnits/AngularMomentum.h
        include/Vectoris/Numerics/Units/DerivedUnits/AngularVelocity.h
        include/Vectoris/Numerics/Units/DerivedUnits/Force.h
        include/Vectoris/Numerics/Units/DerivedUnits/Frequency.h
        include/Vectoris/Numerics/Units/DerivedUnits/MomentOfInertia.h
        include/Vectoris/Numerics/Units/DerivedUnits/Power.h
        include/Vectoris/Numerics/Units/DerivedUnits/Torque.h
        include/Vectoris/Numerics/Units/DerivedUnits/Velocity.h
        include/Vectoris/Numerics/Units/Detail/ABI.h
        include/Vectoris/Numerics/Units/Detail/Ratio.h
        include/Vectoris/Numerics/Units/Common.h
        include/Vectoris/Numerics/Units/Conversion.h
        include/Vectoris/Numerics/Units/Core.h
        include/Vectoris/Numerics/Units/Dimension.h
        include/Vectoris/Numerics/Units/Literals.h
        include/Vectoris/Numerics/Units/Quantity.h
        include/Vectoris/Numerics/Units/QuantityABI.h
        include/Vectoris/Numerics/Units/UnitCast.h
        include/Vectoris/Numerics/Units/UnitConcepts.h
        include/Vectoris/Numerics/Units/UnitTraits.h

        # Geometry (15)
        include/Vectoris/Numerics/Geometry/AlmostEqual.h
        include/Vectoris/Numerics/Geometry/Concepts.h
        include/Vectoris/Numerics/Geometry/CoordinateConvention.h
        include/Vectoris/Numerics/Geometry/Detail/ABI.h
        include/Vectoris/Numerics/Geometry/Detail/RotationInvariant.h
        include/Vectoris/Numerics/Geometry/FrameTags.h
        include/Vectoris/Numerics/Geometry/Matrix3.h
        include/Vectoris/Numerics/Geometry/Point3.h
        include/Vectoris/Numerics/Geometry/Quaternion.h
        include/Vectoris/Numerics/Geometry/RotationMatrix3.h
        include/Vectoris/Numerics/Geometry/SymmetricLinearSolver3.h
        include/Vectoris/Numerics/Geometry/Traits.h
        include/Vectoris/Numerics/Geometry/Transform3.h
        include/Vectoris/Numerics/Geometry/UnitVector3.h
        include/Vectoris/Numerics/Geometry/Vector3.h
)

add_library(VectorisNumerics INTERFACE ${VECTORIS_NUMERICS_PUBLIC_HEADERS})
add_library(Vectoris::Numerics ALIAS VectorisNumerics)

target_include_directories(VectorisNumerics INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(VectorisNumerics INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ==============================================================================
# Numerics Unit Tests
# ==============================================================================
if(TARGET gtest)
    add_executable(VectorisNumerics_Tests
            tests/Core/NumericTraitsTest.cpp
            tests/Core/MathFunctionsTest.cpp
            tests/Core/PublicTemplateInstantiationTest.cpp
            tests/Core/ResultTest.cpp
            tests/Units/UnitsTest.cpp
            tests/Units/PublicTemplateInstantiationTest.cpp
            tests/Geometry/AttitudeEngineTest.cpp
            tests/Geometry/Matrix3Test.cpp
            tests/Geometry/SymmetricLinearSolver3Test.cpp
            tests/Geometry/PublicTemplateInstantiationTest.cpp
            tests/Geometry/GeometryComparisonTest.cpp
            tests/Architecture/DependencyLayerTest.cpp
    )

    # Pass project root for DependencyLayerTest path resolution
    target_compile_definitions(VectorisNumerics_Tests PRIVATE
            VECTORIS_PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
    )

    target_compile_options(VectorisNumerics_Tests PRIVATE ${VECTORIS_STRICT_WARNINGS})

    target_link_libraries(VectorisNumerics_Tests
            PRIVATE
            VectorisNumerics
            gtest_main
    )

    if(COMMAND vectoris_apply_sanitizers)
        vectoris_apply_sanitizers(VectorisNumerics_Tests)
    endif()

    if(COMMAND vectoris_apply_coverage)
        vectoris_apply_coverage(VectorisNumerics_Tests)
        vectoris_register_coverage_target(VectorisNumerics_Tests)
    endif()

    gtest_discover_tests(VectorisNumerics_Tests)

    # ==============================================================================
    # Public Header Standalone Isolation Qualification
    # ==============================================================================
    set(VECTORIS_NUMERICS_PUBLIC_HEADERS ${VECTORIS_NUMERICS_PUBLIC_HEADERS})
    include(${PROJECT_SOURCE_DIR}/cmake/PublicHeaderIsolation.cmake)
endif()

# ==============================================================================
# Static Analysis Qualification
# ==============================================================================
include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalysis.cmake)
