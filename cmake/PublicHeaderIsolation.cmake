# ==============================================================================
# Public Header Isolation Qualification (P2-C)
# Enforces that every public header compiles independently in its own TU.
# ==============================================================================

set(AEGISMATH_ISOLATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/header_isolation")
file(MAKE_DIRECTORY "${AEGISMATH_ISOLATION_DIR}")

set(AEGISMATH_ISOLATION_SOURCES "")
set(AEGISMATH_FORWARD_INCLUDES "")

foreach(HEADER_PATH IN LISTS AEGISMATH_PUBLIC_HEADERS)
    # Compute relative path from include/
    file(RELATIVE_PATH REL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_PATH}")

    # Generate sanitized filename: e.g. iso_AegisMath_Core_BasicTypes.cpp
    string(REGEX REPLACE "[/.]" "_" SANITIZED_NAME "${REL_HEADER}")
    set(TU_FILE "${AEGISMATH_ISOLATION_DIR}/iso_${SANITIZED_NAME}.cpp")

    file(WRITE "${TU_FILE}"
"// Standalone Translation-Unit Isolation Test for: <${REL_HEADER}>
#include <${REL_HEADER}>

int main() {
    return 0;
}
")
    list(APPEND AEGISMATH_ISOLATION_SOURCES "${TU_FILE}")
    list(APPEND AEGISMATH_FORWARD_INCLUDES "#include <${REL_HEADER}>\n")
endforeach()

# ==============================================================================
# Include-Order Poisoning Verification
# ==============================================================================

# Forward order
string(CONCAT FORWARD_CONTENT ${AEGISMATH_FORWARD_INCLUDES})
set(FORWARD_TU "${AEGISMATH_ISOLATION_DIR}/order_poison_forward.cpp")
file(WRITE "${FORWARD_TU}"
"// Include-Order Poisoning Verification (Forward Order)
${FORWARD_CONTENT}
int main() {
    return 0;
}
")
list(APPEND AEGISMATH_ISOLATION_SOURCES "${FORWARD_TU}")

# Reverse order
set(AEGISMATH_REVERSE_LIST ${AEGISMATH_FORWARD_INCLUDES})
list(REVERSE AEGISMATH_REVERSE_LIST)
string(CONCAT REVERSE_CONTENT ${AEGISMATH_REVERSE_LIST})
set(REVERSE_TU "${AEGISMATH_ISOLATION_DIR}/order_poison_reverse.cpp")
file(WRITE "${REVERSE_TU}"
"// Include-Order Poisoning Verification (Reverse Order)
${REVERSE_CONTENT}
int main() {
    return 0;
}
")
list(APPEND AEGISMATH_ISOLATION_SOURCES "${REVERSE_TU}")

# ==============================================================================
# OBJECT Library target to build all isolation TUs in parallel
# ==============================================================================
add_library(AegisMathLib_HeaderIsolation OBJECT ${AEGISMATH_ISOLATION_SOURCES})
target_link_libraries(AegisMathLib_HeaderIsolation PRIVATE AegisMathLib)
target_compile_options(AegisMathLib_HeaderIsolation PRIVATE ${AEGIS_STRICT_WARNINGS})
set_target_properties(AegisMathLib_HeaderIsolation PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
if(COMMAND aegismath_apply_sanitizers)
    aegismath_apply_sanitizers(AegisMathLib_HeaderIsolation)
endif()
