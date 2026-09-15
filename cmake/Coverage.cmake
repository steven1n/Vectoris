# ==============================================================================
# Coverage Qualification Configuration (P2-COV)
# Provides scoped LLVM source-based coverage instrumentation and verification.
# ==============================================================================

option(AEGISMATH_ENABLE_COVERAGE "Enable Stable-Core source-based coverage instrumentation" OFF)

if(AEGISMATH_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        message(FATAL_ERROR "Coverage qualification is currently supported only for Clang and AppleClang. Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if(AEGISMATH_ENABLE_ASAN OR AEGISMATH_ENABLE_UBSAN)
        message(FATAL_ERROR "Coverage qualification requires sanitizer options (AEGISMATH_ENABLE_ASAN and AEGISMATH_ENABLE_UBSAN) to be OFF to prevent profile distortion.")
    endif()

    # Locate LLVM coverage tools
    find_program(LLVM_PROFDATA_BIN NAMES llvm-profdata)
    find_program(LLVM_COV_BIN NAMES llvm-cov)

    if(APPLE)
        if(NOT LLVM_PROFDATA_BIN)
            execute_process(
                COMMAND xcrun --find llvm-profdata
                OUTPUT_VARIABLE XCRUN_PROFDATA
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(XCRUN_PROFDATA)
                set(LLVM_PROFDATA_BIN "${XCRUN_PROFDATA}")
            endif()
        endif()

        if(NOT LLVM_COV_BIN)
            execute_process(
                COMMAND xcrun --find llvm-cov
                OUTPUT_VARIABLE XCRUN_COV
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(XCRUN_COV)
                set(LLVM_COV_BIN "${XCRUN_COV}")
            endif()
        endif()
    endif()

    if(NOT LLVM_PROFDATA_BIN OR NOT LLVM_COV_BIN)
        message(FATAL_ERROR "llvm-profdata and/or llvm-cov could not be found. Please ensure LLVM tools are installed.")
    endif()
endif()

function(aegismath_apply_coverage TARGET_NAME)
    if(NOT AEGISMATH_ENABLE_COVERAGE)
        return()
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if(NOT TARGET_TYPE STREQUAL "OBJECT_LIBRARY" AND NOT TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        target_link_options(${TARGET_NAME} PRIVATE -fprofile-instr-generate)
    endif()
endfunction()

function(aegismath_register_coverage_target TEST_TARGET)
    if(NOT AEGISMATH_ENABLE_COVERAGE)
        return()
    endif()

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_custom_target(AegisMathLib_Coverage
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/coverage/run_coverage.py"
            --test-binary "$<TARGET_FILE:${TEST_TARGET}>"
            --llvm-profdata "${LLVM_PROFDATA_BIN}"
            --llvm-cov "${LLVM_COV_BIN}"
            --build-dir "${CMAKE_BINARY_DIR}"
            --repo-root "${CMAKE_SOURCE_DIR}"
        DEPENDS ${TEST_TARGET}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running instrumented tests and evaluating coverage gate..."
    )

    add_custom_target(AegisMathLib_Coverage_HTML
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/coverage/run_coverage.py"
            --test-binary "$<TARGET_FILE:${TEST_TARGET}>"
            --llvm-profdata "${LLVM_PROFDATA_BIN}"
            --llvm-cov "${LLVM_COV_BIN}"
            --build-dir "${CMAKE_BINARY_DIR}"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --html-dir "${CMAKE_BINARY_DIR}/coverage-html"
        DEPENDS ${TEST_TARGET}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running instrumented tests and generating HTML coverage report..."
    )
endfunction()
