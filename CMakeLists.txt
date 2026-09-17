cmake_minimum_required(VERSION 3.14)
project(AegisMathLib LANGUAGES CXX)

# ==============================================================================
# 1. Global Build Standard (C++20 Mandatory Isolation)
# ==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# 2. Global Strict Compiler Discipline (applied only to Aegis-owned targets)
# ==============================================================================
set(AEGIS_STRICT_WARNINGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    set(AEGIS_STRICT_WARNINGS
            -Wall -Wextra -Wpedantic
            -Wconversion -Wshadow -Werror
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(AEGIS_STRICT_WARNINGS /W4 /WX /permissive- /utf-8)
endif()

# ==============================================================================
# 3. Third-Party Dependencies (GoogleTest)
# ==============================================================================
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
if(NOT WIN32)
    set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ==============================================================================
# 4. Shared CMake Infrastructure
# ==============================================================================
include(cmake/Sanitizers.cmake)
include(cmake/Coverage.cmake)

# ==============================================================================
# 5. Core Mathematics Library Module
# ==============================================================================
add_subdirectory(modules/AegisMathLib)

# ==============================================================================
# 6. Optional Domain Physics Module (AegisDynamics)
# ==============================================================================
option(AEGISMATH_BUILD_DYNAMICS "Build independent AegisDynamics domain physics module" ON)
if(AEGISMATH_BUILD_DYNAMICS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/AegisDynamics/CMakeLists.txt")
    add_subdirectory(modules/AegisDynamics)
endif()
