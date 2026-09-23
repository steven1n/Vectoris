# ==============================================================================
# Static Analysis Qualification Configuration (P2-STA)
# Provides scoped Clang-Tidy and Cppcheck qualification targets.
# ==============================================================================

option(VECTORIS_ENABLE_STATIC_ANALYSIS "Enable static analysis qualification targets" OFF)

if(NOT VECTORIS_ENABLE_STATIC_ANALYSIS)
    return()
endif()

# Support explicit cache overrides
set(VECTORIS_CLANG_TIDY "" CACHE FILEPATH "Path to clang-tidy executable override")
set(VECTORIS_CLANG_TIDY_MAJOR "" CACHE STRING "Required clang-tidy major version for qualification")
set(VECTORIS_CPPCHECK "" CACHE FILEPATH "Path to cppcheck executable override")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Target: VectorisNumerics_ClangTidy
add_custom_target(VectorisNumerics_ClangTidy
    COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/static_analysis/run_clang_tidy.py
            --build-dir ${CMAKE_BINARY_DIR}
            $<$<BOOL:${VECTORIS_CLANG_TIDY}>:--clang-tidy=${VECTORIS_CLANG_TIDY}>
            $<$<BOOL:${VECTORIS_CLANG_TIDY_MAJOR}>:--required-clang-tidy-major=${VECTORIS_CLANG_TIDY_MAJOR}>
            --warnings-as-errors
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Executing Clang-Tidy static analysis qualification gate..."
)

# Target: VectorisNumerics_Cppcheck
add_custom_target(VectorisNumerics_Cppcheck
    COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/static_analysis/run_cppcheck.py
            $<$<BOOL:${VECTORIS_CPPCHECK}>:--cppcheck=${VECTORIS_CPPCHECK}>
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Executing Cppcheck static analysis qualification runner..."
)

# Combined Target: VectorisNumerics_StaticAnalysis
add_custom_target(VectorisNumerics_StaticAnalysis
    DEPENDS VectorisNumerics_ClangTidy VectorisNumerics_Cppcheck
    COMMENT "Mandatory static-analysis gate completed; recommended analyzers reported separately."
)
