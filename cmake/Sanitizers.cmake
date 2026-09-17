# ==============================================================================
# Sanitizer Qualification Configuration (P2-SAN)
# Provides scoped AddressSanitizer and UndefinedBehaviorSanitizer options.
# ==============================================================================

option(VECTORIS_ENABLE_ASAN "Enable AddressSanitizer (ASan)" OFF)
option(VECTORIS_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (UBSan)" OFF)

function(vectoris_apply_sanitizers TARGET_NAME)
    if(NOT VECTORIS_ENABLE_ASAN AND NOT VECTORIS_ENABLE_UBSAN)
        return()
    endif()

    set(SANITIZER_COMPILE_FLAGS "")
    set(SANITIZER_LINK_FLAGS "")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        if(VECTORIS_ENABLE_ASAN)
            list(APPEND SANITIZER_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
            list(APPEND SANITIZER_LINK_FLAGS -fsanitize=address)
        endif()

        if(VECTORIS_ENABLE_UBSAN)
            list(APPEND SANITIZER_COMPILE_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
            list(APPEND SANITIZER_LINK_FLAGS -fsanitize=undefined)
        endif()
    else()
        message(FATAL_ERROR "Sanitizers (ASan/UBSan) are currently supported only for Clang, AppleClang, and GNU GCC. Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if(SANITIZER_COMPILE_FLAGS)
        target_compile_options(${TARGET_NAME} PRIVATE ${SANITIZER_COMPILE_FLAGS})
    endif()

    if(SANITIZER_LINK_FLAGS)
        get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
        if(NOT TARGET_TYPE STREQUAL "OBJECT_LIBRARY" AND NOT TARGET_TYPE STREQUAL "STATIC_LIBRARY")
            target_link_options(${TARGET_NAME} PRIVATE ${SANITIZER_LINK_FLAGS})
        endif()
    endif()
endfunction()

function(aegismath_apply_sanitizers TARGET_NAME)
    vectoris_apply_sanitizers(${TARGET_NAME})
endfunction()
