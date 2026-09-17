# ==============================================================================
# Static Analysis Qualification Configuration (P2-STA)
# Provides scoped Clang-Tidy and Cppcheck qualification targets.
# ==============================================================================

option(AEGISMATH_ENABLE_STATIC_ANALYSIS "Enable static analysis qualification targets" OFF)

if(NOT AEGISMATH_ENABLE_STATIC_ANALYSIS)
    return()
endif()

# Support explicit cache overrides
set(AEGISMATH_CLANG_TIDY "" CACHE FILEPATH "Path to clang-tidy executable override")
set(AEGISMATH_CPPCHECK "" CACHE FILEPATH "Path to cppcheck executable override")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Target: AegisMathLib_ClangTidy
add_custom_target(AegisMathLib_ClangTidy
    COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/static_analysis/run_clang_tidy.py
            --build-dir ${CMAKE_BINARY_DIR}
            $<$<BOOL:${AEGISMATH_CLANG_TIDY}>:--clang-tidy=${AEGISMATH_CLANG_TIDY}>
            --warnings-as-errors
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Executing Clang-Tidy static analysis qualification gate..."
)

# Target: AegisMathLib_Cppcheck
add_custom_target(AegisMathLib_Cppcheck
    COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/static_analysis/run_cppcheck.py
            $<$<BOOL:${AEGISMATH_CPPCHECK}>:--cppcheck=${AEGISMATH_CPPCHECK}>
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Executing Cppcheck static analysis qualification runner..."
)

# Combined Target: AegisMathLib_StaticAnalysis
add_custom_target(AegisMathLib_StaticAnalysis
    DEPENDS AegisMathLib_ClangTidy AegisMathLib_Cppcheck
    COMMENT "Mandatory static-analysis gate completed; recommended analyzers reported separately."
)
