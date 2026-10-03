# ==============================================================================
# VectorisNumerics Module CMake Configuration
# Pure Mathematics & Numerical Computation Library
# ==============================================================================

set(VECTORIS_NUMERICS_PUBLIC_HEADERS
        # Core (11)
        include/Vectoris/Numerics/Core/Namespace.h
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

        # Units (31)
        include/Vectoris/Numerics/Units/Namespace.h
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

        # Geometry (16)
        include/Vectoris/Numerics/Geometry/Namespace.h
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
target_compile_features(VectorisNumerics INTERFACE cxx_std_20)

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
if(BUILD_TESTING)
    add_executable(VectorisNumerics_Tests
            tests/Core/NumericTraitsTest.cpp
            tests/Core/AlmostEqualExtremeTest.cpp
            tests/Core/MathFunctionsTest.cpp
            tests/Core/SqrtContractTest.cpp
            tests/Core/PublicTemplateInstantiationTest.cpp
            tests/Core/ResultTest.cpp
            tests/Core/ResultExceptionTest.cpp
            tests/Units/UnitsTest.cpp
            tests/Units/PublicTemplateInstantiationTest.cpp
            tests/Geometry/AttitudeEngineTest.cpp
            tests/Geometry/UnitVector3Test.cpp
            tests/Geometry/Matrix3PublicMultiplicationTest.cpp
            tests/Geometry/Matrix3RepresentationTest.cpp
            tests/Geometry/Matrix3Test.cpp
            tests/Geometry/SymmetricLinearSolver3Test.cpp
            tests/Geometry/SymmetricLinearSolver3ScaleTest.cpp
            tests/Geometry/PublicTemplateInstantiationTest.cpp
            tests/Geometry/GeometryComparisonTest.cpp
            tests/Geometry/AlmostEqualWrapperTest.cpp
            tests/PublicApi/PublicApiSurfaceTest.cpp
            tests/PublicApi/NamespaceOrder/Core_Units.cpp
            tests/PublicApi/NamespaceOrder/Core_Geometry.cpp
            tests/PublicApi/NamespaceOrder/Units_Core.cpp
            tests/PublicApi/NamespaceOrder/Units_Geometry.cpp
            tests/PublicApi/NamespaceOrder/Geometry_Core.cpp
            tests/PublicApi/NamespaceOrder/Geometry_Units.cpp
            tests/PublicApi/NamespaceOrder/Core_Units_Geometry.cpp
            tests/PublicApi/NamespaceOrder/Core_Geometry_Units.cpp
            tests/PublicApi/NamespaceOrder/Units_Core_Geometry.cpp
            tests/PublicApi/NamespaceOrder/Units_Geometry_Core.cpp
            tests/PublicApi/NamespaceOrder/Geometry_Core_Units.cpp
            tests/PublicApi/NamespaceOrder/Geometry_Units_Core.cpp
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

    include(${PROJECT_SOURCE_DIR}/cmake/Matrix3InverseDomainProbe.cmake)

    gtest_discover_tests(VectorisNumerics_Tests)

    # AFA3-001: execute the actual documented consumers on every qualification
    # compiler. Keep Python and these gates inside BUILD_TESTING only.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME Core.DocumentedIncludeContract
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/api_surface/verify_core_include_contract.py
            --repo-root ${PROJECT_SOURCE_DIR} --cxx ${CMAKE_CXX_COMPILER}
            --compiler-id ${CMAKE_CXX_COMPILER_ID})
    add_test(NAME Core.DocumentedIncludeContractSelfTests
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/api_surface/test_core_include_contract.py)
    set_tests_properties(Core.DocumentedIncludeContract Core.DocumentedIncludeContractSelfTests
        PROPERTIES LABELS "PublicConsumerContract")

    # ==============================================================================
    # Public Header Standalone Isolation Qualification
    # ==============================================================================
    set(VECTORIS_NUMERICS_PUBLIC_HEADERS ${VECTORIS_NUMERICS_PUBLIC_HEADERS})
    include(${PROJECT_SOURCE_DIR}/cmake/PublicHeaderIsolation.cmake)
endif()

# ==============================================================================
# Static Analysis Qualification
# ==============================================================================
if(BUILD_TESTING)
    include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalysis.cmake)
endif()

option(VECTORIS_BUILD_BENCHMARKS "Build informational performance microbenchmarks" OFF)
if(VECTORIS_BUILD_BENCHMARKS)
    add_executable(VectorisNumerics_Matrix3_Benchmarks
            ../../benchmarks/Matrix3Benchmark.cpp
    )
    target_compile_options(VectorisNumerics_Matrix3_Benchmarks PRIVATE ${VECTORIS_STRICT_WARNINGS})
    target_link_libraries(VectorisNumerics_Matrix3_Benchmarks PRIVATE VectorisNumerics)
endif()
