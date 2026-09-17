# ==============================================================================
# AegisMathLib Module CMake Configuration
# Pure Mathematics & Numerical Computation Library
# ==============================================================================

set(AEGISMATH_PUBLIC_HEADERS
        # Core (10)
        include/AegisMath/Core/BasicTypes.h
        include/AegisMath/Core/Compiler.h
        include/AegisMath/Core/Concepts.h
        include/AegisMath/Core/Constants.h
        include/AegisMath/Core/Math.h
        include/AegisMath/Core/MathError.h
        include/AegisMath/Core/MathFunctions.h
        include/AegisMath/Core/NumericTraits.h
        include/AegisMath/Core/Precision.h
        include/AegisMath/Core/Result.h

        # Units (30)
        include/AegisMath/Units/BaseUnits/Amount.h
        include/AegisMath/Units/BaseUnits/Angle.h
        include/AegisMath/Units/BaseUnits/Current.h
        include/AegisMath/Units/BaseUnits/Length.h
        include/AegisMath/Units/BaseUnits/Luminosity.h
        include/AegisMath/Units/BaseUnits/Mass.h
        include/AegisMath/Units/BaseUnits/Temperature.h
        include/AegisMath/Units/BaseUnits/Time.h
        include/AegisMath/Units/DerivedUnits/Acceleration.h
        include/AegisMath/Units/DerivedUnits/AngularAcceleration.h
        include/AegisMath/Units/DerivedUnits/AngularMomentum.h
        include/AegisMath/Units/DerivedUnits/AngularVelocity.h
        include/AegisMath/Units/DerivedUnits/Force.h
        include/AegisMath/Units/DerivedUnits/Frequency.h
        include/AegisMath/Units/DerivedUnits/MomentOfInertia.h
        include/AegisMath/Units/DerivedUnits/Power.h
        include/AegisMath/Units/DerivedUnits/Torque.h
        include/AegisMath/Units/DerivedUnits/Velocity.h
        include/AegisMath/Units/Detail/ABI.h
        include/AegisMath/Units/Detail/Ratio.h
        include/AegisMath/Units/Common.h
        include/AegisMath/Units/Conversion.h
        include/AegisMath/Units/Core.h
        include/AegisMath/Units/Dimension.h
        include/AegisMath/Units/Literals.h
        include/AegisMath/Units/Quantity.h
        include/AegisMath/Units/QuantityABI.h
        include/AegisMath/Units/UnitCast.h
        include/AegisMath/Units/UnitConcepts.h
        include/AegisMath/Units/UnitTraits.h

        # Geometry (15)
        include/AegisMath/Geometry/AlmostEqual.h
        include/AegisMath/Geometry/Concepts.h
        include/AegisMath/Geometry/CoordinateConvention.h
        include/AegisMath/Geometry/Detail/ABI.h
        include/AegisMath/Geometry/Detail/RotationInvariant.h
        include/AegisMath/Geometry/FrameTags.h
        include/AegisMath/Geometry/Matrix3.h
        include/AegisMath/Geometry/Point3.h
        include/AegisMath/Geometry/Quaternion.h
        include/AegisMath/Geometry/RotationMatrix3.h
        include/AegisMath/Geometry/SymmetricLinearSolver3.h
        include/AegisMath/Geometry/Traits.h
        include/AegisMath/Geometry/Transform3.h
        include/AegisMath/Geometry/UnitVector3.h
        include/AegisMath/Geometry/Vector3.h
)

add_library(AegisMathLib INTERFACE ${AEGISMATH_PUBLIC_HEADERS})

target_include_directories(AegisMathLib INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(AegisMathLib INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ==============================================================================
# MathLib Unit Tests
# ==============================================================================
if(TARGET gtest)
    add_executable(AegisMathLib_Tests
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
    target_compile_definitions(AegisMathLib_Tests PRIVATE
            AEGISMATH_PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
    )

    target_compile_options(AegisMathLib_Tests PRIVATE ${AEGIS_STRICT_WARNINGS})

    target_link_libraries(AegisMathLib_Tests
            PRIVATE
            AegisMathLib
            gtest_main
    )

    if(COMMAND aegismath_apply_sanitizers)
        aegismath_apply_sanitizers(AegisMathLib_Tests)
    endif()

    if(COMMAND aegismath_apply_coverage)
        aegismath_apply_coverage(AegisMathLib_Tests)
        aegismath_register_coverage_target(AegisMathLib_Tests)
    endif()

    gtest_discover_tests(AegisMathLib_Tests)

    # ==============================================================================
    # Public Header Standalone Isolation Qualification
    # ==============================================================================
    include(${PROJECT_SOURCE_DIR}/cmake/PublicHeaderIsolation.cmake)
endif()

# ==============================================================================
# Static Analysis Qualification
# ==============================================================================
include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalysis.cmake)
