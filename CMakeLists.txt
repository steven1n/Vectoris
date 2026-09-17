cmake_minimum_required(VERSION 3.14)
project(Vectoris LANGUAGES CXX)

# ==============================================================================
# 1. Global Build Standard (C++20 Mandatory Isolation)
# ==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# 2. Global Strict Compiler Discipline (applied only to Vectoris-owned targets)
# ==============================================================================
set(VECTORIS_STRICT_WARNINGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    set(VECTORIS_STRICT_WARNINGS
            -Wall -Wextra -Wpedantic
            -Wconversion -Wshadow -Werror
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(VECTORIS_STRICT_WARNINGS /W4 /WX /permissive- /utf-8)
endif()
set(AEGIS_STRICT_WARNINGS "${VECTORIS_STRICT_WARNINGS}")

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
# 5. Core Mathematics Library Module (VectorisNumerics)
# ==============================================================================
add_subdirectory(modules/VectorisNumerics)

# ==============================================================================
# 6. Optional Domain Physics Module (VectorisDynamics)
# ==============================================================================
option(VECTORIS_BUILD_DYNAMICS "Build independent VectorisDynamics domain physics module" ON)
if(VECTORIS_BUILD_DYNAMICS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/VectorisDynamics/CMakeLists.txt")
    add_subdirectory(modules/VectorisDynamics)
endif()
