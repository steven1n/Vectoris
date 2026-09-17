# ==============================================================================
# AegisDynamics Module CMake Configuration
# Domain Physics & Rigid-Body Mechanics
# Strictly depends on AegisMathLib
# ==============================================================================

set(AEGISDYNAMICS_PUBLIC_HEADERS
    include/AegisDynamics/Concepts.h
    include/AegisDynamics/Detail/DynamicsABI.h
    include/AegisDynamics/Detail/StateTypes.h
    include/AegisDynamics/DynamicsConvention.h
    include/AegisDynamics/EulerIntegrator.h
    include/AegisDynamics/InertiaTensor3.h
    include/AegisDynamics/QuantityVector3.h
    include/AegisDynamics/RigidBodyParameters.h
    include/AegisDynamics/RigidBodyState.h
    include/AegisDynamics/Twist6.h
    include/AegisDynamics/Wrench6.h
)

add_library(AegisDynamics INTERFACE ${AEGISDYNAMICS_PUBLIC_HEADERS})

target_include_directories(AegisDynamics INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(AegisDynamics INTERFACE AegisMathLib)

if(TARGET gtest)
    add_executable(AegisDynamics_Tests
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

    target_compile_options(AegisDynamics_Tests PRIVATE ${AEGIS_STRICT_WARNINGS})

    target_link_libraries(AegisDynamics_Tests
        PRIVATE
        AegisDynamics
        AegisMathLib
        gtest_main
    )

    if(COMMAND aegismath_apply_sanitizers)
        aegismath_apply_sanitizers(AegisDynamics_Tests)
    endif()

    if(COMMAND aegismath_apply_coverage)
        aegismath_apply_coverage(AegisDynamics_Tests)
    endif()

    gtest_discover_tests(AegisDynamics_Tests)

    # ==============================================================================
    # Header Standalone Isolation for AegisDynamics
    # ==============================================================================
    set(DYNAMICS_ISOLATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/header_isolation")
    file(MAKE_DIRECTORY "${DYNAMICS_ISOLATION_DIR}")

    set(DYNAMICS_ISOLATION_SOURCES "")
    set(DYNAMICS_FORWARD_INCLUDES "")

    foreach(HEADER_PATH IN LISTS AEGISDYNAMICS_PUBLIC_HEADERS)
        file(RELATIVE_PATH REL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_PATH}")
        string(REGEX REPLACE "[/.]" "_" SANITIZED_NAME "${REL_HEADER}")
        set(TU_FILE "${DYNAMICS_ISOLATION_DIR}/iso_${SANITIZED_NAME}.cpp")

        file(WRITE "${TU_FILE}"
"// Standalone Translation-Unit Isolation Test for: <${REL_HEADER}>
#include <${REL_HEADER}>

int main() {
    return 0;
}
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
int main() {
    return 0;
}
")
    list(APPEND DYNAMICS_ISOLATION_SOURCES "${FORWARD_TU}")

    set(DYNAMICS_REVERSE_LIST ${DYNAMICS_FORWARD_INCLUDES})
    list(REVERSE DYNAMICS_REVERSE_LIST)
    string(CONCAT REVERSE_CONTENT ${DYNAMICS_REVERSE_LIST})
    set(REVERSE_TU "${DYNAMICS_ISOLATION_DIR}/order_poison_reverse.cpp")
    file(WRITE "${REVERSE_TU}"
"// Include-Order Poisoning Verification (Reverse Order)
${REVERSE_CONTENT}
int main() {
    return 0;
}
")
    list(APPEND DYNAMICS_ISOLATION_SOURCES "${REVERSE_TU}")

    add_library(AegisDynamics_HeaderIsolation OBJECT ${DYNAMICS_ISOLATION_SOURCES})
    target_compile_options(AegisDynamics_HeaderIsolation PRIVATE ${AEGIS_STRICT_WARNINGS})
    target_link_libraries(AegisDynamics_HeaderIsolation PRIVATE AegisDynamics AegisMathLib)

    if(COMMAND aegismath_apply_sanitizers)
        aegismath_apply_sanitizers(AegisDynamics_HeaderIsolation)
    endif()
endif()
