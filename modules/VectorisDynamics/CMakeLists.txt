# ==============================================================================
# VectorisDynamics Module CMake Configuration
# Domain Physics & Rigid-Body Mechanics
# Strictly depends on VectorisNumerics
# ==============================================================================

set(VECTORIS_DYNAMICS_PUBLIC_HEADERS
    include/Vectoris/Dynamics/Concepts.h
    include/Vectoris/Dynamics/Detail/DynamicsABI.h
    include/Vectoris/Dynamics/Detail/StateTypes.h
    include/Vectoris/Dynamics/DynamicsConvention.h
    include/Vectoris/Dynamics/EulerIntegrator.h
    include/Vectoris/Dynamics/InertiaTensor3.h
    include/Vectoris/Dynamics/QuantityVector3.h
    include/Vectoris/Dynamics/RigidBodyParameters.h
    include/Vectoris/Dynamics/RigidBodyState.h
    include/Vectoris/Dynamics/Twist6.h
    include/Vectoris/Dynamics/Wrench6.h
)

add_library(VectorisDynamics INTERFACE ${VECTORIS_DYNAMICS_PUBLIC_HEADERS})
add_library(Vectoris::Dynamics ALIAS VectorisDynamics)
target_compile_features(VectorisDynamics INTERFACE cxx_std_20)

target_include_directories(VectorisDynamics INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(VectorisDynamics INTERFACE Vectoris::Numerics)

if(BUILD_TESTING)
    add_executable(VectorisDynamics_Tests
        tests/DynamicsABITest.cpp
        tests/DynamicsConceptTest.cpp
        tests/DynamicsUnitsTest.cpp
        tests/EulerDynamicsTest.cpp
        tests/InertiaTensorTest.cpp
        tests/PropagationTest.cpp
        tests/Regression/FreeFallTest.cpp
        tests/RigidBodyStateTest.cpp
        tests/StateTypesTest.cpp
        tests/Twist6Test.cpp
        tests/Wrench6Test.cpp
    )

    target_compile_options(VectorisDynamics_Tests PRIVATE ${VECTORIS_STRICT_WARNINGS})

    target_link_libraries(VectorisDynamics_Tests
        PRIVATE
        VectorisDynamics
        VectorisNumerics
        gtest_main
    )

    if(COMMAND vectoris_apply_sanitizers)
        vectoris_apply_sanitizers(VectorisDynamics_Tests)
    elseif(COMMAND aegismath_apply_sanitizers)
        aegismath_apply_sanitizers(VectorisDynamics_Tests)
    endif()

    if(COMMAND vectoris_apply_coverage)
        vectoris_apply_coverage(VectorisDynamics_Tests)
    elseif(COMMAND aegismath_apply_coverage)
        aegismath_apply_coverage(VectorisDynamics_Tests)
    endif()

    gtest_discover_tests(VectorisDynamics_Tests)

    # ==============================================================================
    # Header Standalone Isolation for VectorisDynamics
    # ==============================================================================
    set(DYNAMICS_ISOLATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/header_isolation")
    file(MAKE_DIRECTORY "${DYNAMICS_ISOLATION_DIR}")

    set(DYNAMICS_ISOLATION_SOURCES "")
    set(DYNAMICS_FORWARD_INCLUDES "")

    foreach(HEADER_PATH IN LISTS VECTORIS_DYNAMICS_PUBLIC_HEADERS)
        file(RELATIVE_PATH REL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_PATH}")
        string(REGEX REPLACE "[/.]" "_" SANITIZED_NAME "${REL_HEADER}")
        set(TU_FILE "${DYNAMICS_ISOLATION_DIR}/iso_${SANITIZED_NAME}.cpp")

        file(WRITE "${TU_FILE}"
"// Standalone Translation-Unit Isolation Test for: <${REL_HEADER}>
#include <${REL_HEADER}>
namespace dynamics_contract = vectoris::dynamics;
")
        list(APPEND DYNAMICS_ISOLATION_SOURCES "${TU_FILE}")
        list(APPEND DYNAMICS_FORWARD_INCLUDES "#include <${REL_HEADER}>\n")
    endforeach()

    # Include-order poisoning
    string(CONCAT FORWARD_CONTENT ${DYNAMICS_FORWARD_INCLUDES})
    set(FORWARD_TU "${DYNAMICS_ISOLATION_DIR}/order_poison_forward.cpp")
    file(WRITE "${FORWARD_TU}"
"// Include-Order Poisoning Verification (Forward Order)
${FORWARD_CONTENT}
namespace dynamics_contract = vectoris::dynamics;
")
    list(APPEND DYNAMICS_ISOLATION_SOURCES "${FORWARD_TU}")

    set(DYNAMICS_REVERSE_LIST ${DYNAMICS_FORWARD_INCLUDES})
    list(REVERSE DYNAMICS_REVERSE_LIST)
    string(CONCAT REVERSE_CONTENT ${DYNAMICS_REVERSE_LIST})
    set(REVERSE_TU "${DYNAMICS_ISOLATION_DIR}/order_poison_reverse.cpp")
    file(WRITE "${REVERSE_TU}"
"// Include-Order Poisoning Verification (Reverse Order)
${REVERSE_CONTENT}
namespace dynamics_contract = vectoris::dynamics;
")
    list(APPEND DYNAMICS_ISOLATION_SOURCES "${REVERSE_TU}")

    include(${PROJECT_SOURCE_DIR}/cmake/HeaderIsolationTarget.cmake)
    vectoris_add_header_isolation_target(VectorisDynamics_HeaderIsolation
        SOURCES ${DYNAMICS_ISOLATION_SOURCES}
        LIBRARIES VectorisDynamics VectorisNumerics)
endif()
