# ==============================================================================
# Public Header Isolation Qualification (P2-C)
# Enforces that every public header compiles independently in its own TU.
# ==============================================================================

set(VECTORIS_ISOLATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/header_isolation")
file(MAKE_DIRECTORY "${VECTORIS_ISOLATION_DIR}")

set(VECTORIS_ISOLATION_SOURCES "")
set(VECTORIS_FORWARD_INCLUDES "")

foreach(HEADER_PATH IN LISTS VECTORIS_NUMERICS_PUBLIC_HEADERS)
    # Compute relative path from include/
    file(RELATIVE_PATH REL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_PATH}")

    # Generate sanitized filename: e.g. iso_Vectoris_Numerics_Core_BasicTypes_h.cpp
    string(REGEX REPLACE "[/.]" "_" SANITIZED_NAME "${REL_HEADER}")
    set(TU_FILE "${VECTORIS_ISOLATION_DIR}/iso_${SANITIZED_NAME}.cpp")

    # VRT-12: every direct include must provide both documented namespace spellings.
    string(REGEX MATCH "Vectoris/Numerics/([^/]+)/" NAMESPACE_PATH "${REL_HEADER}")
    set(COMPAT_NAMESPACE "${CMAKE_MATCH_1}")
    string(TOLOWER "${COMPAT_NAMESPACE}" PUBLIC_NAMESPACE)
    set(NAMESPACE_CHECKS "namespace canonical = vectoris::numerics::${PUBLIC_NAMESPACE};\nnamespace compatibility = vectoris::numerics::${COMPAT_NAMESPACE};\n")
    set(NAMESPACE_PROBE "${CMAKE_CURRENT_SOURCE_DIR}/tests/PublicApi/NamespaceProbes/${SANITIZED_NAME}.inc")
    set(NAMESPACE_BODY "")
    if(EXISTS "${NAMESPACE_PROBE}")
        # Included inside main: no additional Vectoris headers may mask isolation.
        set(NAMESPACE_BODY "#include \"${NAMESPACE_PROBE}\"")
    endif()

    file(WRITE "${TU_FILE}"
"// Standalone Translation-Unit Isolation Test for: <${REL_HEADER}>
#include <${REL_HEADER}>
#include <type_traits>
${NAMESPACE_CHECKS}

int main() {
${NAMESPACE_BODY}
    return 0;
}
")
    list(APPEND VECTORIS_ISOLATION_SOURCES "${TU_FILE}")
    list(APPEND VECTORIS_FORWARD_INCLUDES "#include <${REL_HEADER}>\n")
endforeach()

# ==============================================================================
# Include-Order Poisoning Verification
# ==============================================================================

# Forward order
string(CONCAT FORWARD_CONTENT ${VECTORIS_FORWARD_INCLUDES})
set(FORWARD_TU "${VECTORIS_ISOLATION_DIR}/order_poison_forward.cpp")
file(WRITE "${FORWARD_TU}"
"// Include-Order Poisoning Verification (Forward Order)
${FORWARD_CONTENT}
int main() {
    return 0;
}
")
    list(APPEND VECTORIS_ISOLATION_SOURCES "${FORWARD_TU}")

# Reverse order
set(VECTORIS_REVERSE_LIST ${VECTORIS_FORWARD_INCLUDES})
list(REVERSE VECTORIS_REVERSE_LIST)
string(CONCAT REVERSE_CONTENT ${VECTORIS_REVERSE_LIST})
set(REVERSE_TU "${VECTORIS_ISOLATION_DIR}/order_poison_reverse.cpp")
file(WRITE "${REVERSE_TU}"
"// Include-Order Poisoning Verification (Reverse Order)
${REVERSE_CONTENT}
int main() {
    return 0;
}
")
list(APPEND VECTORIS_ISOLATION_SOURCES "${REVERSE_TU}")

# ==============================================================================
# OBJECT Library target to build all isolation TUs in parallel
# ==============================================================================
add_library(VectorisNumerics_HeaderIsolation OBJECT ${VECTORIS_ISOLATION_SOURCES})
target_link_libraries(VectorisNumerics_HeaderIsolation PRIVATE VectorisNumerics)
target_compile_options(VectorisNumerics_HeaderIsolation PRIVATE ${VECTORIS_STRICT_WARNINGS})
set_target_properties(VectorisNumerics_HeaderIsolation PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
if(COMMAND vectoris_apply_sanitizers)
    vectoris_apply_sanitizers(VectorisNumerics_HeaderIsolation)
elseif(COMMAND aegismath_apply_sanitizers)
    aegismath_apply_sanitizers(VectorisNumerics_HeaderIsolation)
endif()
