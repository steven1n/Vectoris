cmake_minimum_required(VERSION 3.14)
project(AegisMathLib LANGUAGES CXX)

# ==============================================================================
# 1. 全局构建标准 (C++20 强制隔离)
# ==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# 2. 核心数学库定义 (Type System 模块)
# ==============================================================================
add_library(AegisMathLib INTERFACE
        include/AegisMath/Units/DerivedUnits/Acceleration.h
        include/AegisMath/Core/BasicTypes.h
        include/AegisMath/Core/Compiler.h
        include/AegisMath/Core/Concepts.h
        include/AegisMath/Core/Constants.h
        include/AegisMath/Core/NumericTraits.h
        include/AegisMath/Core/Precision.h
        include/AegisMath/Units/BaseUnits/Angle.h
        include/AegisMath/Units/BaseUnits/Length.h
        include/AegisMath/Units/BaseUnits/Mass.h
        include/AegisMath/Units/BaseUnits/Temperature.h
        include/AegisMath/Units/BaseUnits/Time.h
        include/AegisMath/Units/DerivedUnits/Frequency.h
        include/AegisMath/Units/DerivedUnits/Velocity.h
        include/AegisMath/Units/Common.h
        include/AegisMath/Units/Dimension.h
        include/AegisMath/Units/Literals.h
        include/AegisMath/Units/Quantity.h
        include/AegisMath/Units/Unit.h
        include/AegisMath/Units/UnitCast.h
        include/AegisMath/Units/UnitConcepts.h
        include/AegisMath/Units/UnitTraits.h
        include/AegisMath/Units/Core.h
        include/AegisMath/Units/Conversion.h
        include/AegisMath/Units/QuantityABI.h
        include/AegisMath/Units/BaseUnits/Current.h
        include/AegisMath/Units/BaseUnits/Amount.h
        include/AegisMath/Units/BaseUnits/Luminosity.h
        include/AegisMath/Units/DerivedUnits/Force.h
        include/AegisMath/Units/Detail/ABI.h
        include/AegisMath/Units/Detail/Ratio.h
        include/AegisMath/Geometry/Concepts.h
        include/AegisMath/Geometry/FrameTags.h
        include/AegisMath/Geometry/Detail/ABI.h
        include/AegisMath/Geometry/Vector3.h
        include/AegisMath/Geometry/Point3.h
        include/AegisMath/Geometry/Traits.h
        include/AegisMath/Geometry/Matrix3.h
        include/AegisMath/Geometry/UnitVector3.h
        include/AegisMath/Core/Math.h
        include/AegisMath/Geometry/Detail/RotationInvariant.h
        include/AegisMath/Geometry/RotationMatrix3.h
        include/AegisMath/Geometry/CoordinateConvention.h
        include/AegisMath/Core/Result.h
        include/AegisMath/Geometry/Quaternion.h
        include/AegisMath/Geometry/Transform3.h
        include/AegisMath/Core/MathFunctions.h
)

target_include_directories(AegisMathLib INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ==============================================================================
# 3. 自动化测试依赖拉取 (GoogleTest)
# ==============================================================================
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# ==============================================================================
# 4. 严苛编译纪律 (仅作用于 Aegis 本地代码，放过第三方库)
# ==============================================================================
set(AEGIS_STRICT_WARNINGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    set(AEGIS_STRICT_WARNINGS
            -Wall -Wextra -Wpedantic
            -Wconversion -Wshadow -Werror
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(AEGIS_STRICT_WARNINGS /W4 /WX)
endif()

# ==============================================================================
# 5. Core Layer 单元测试
# ==============================================================================
# 移除旧的 Units 测试，专注当前重构的 Core 测试
add_executable(AegisMathLib_Tests
        tests/Core/NumericTraitsTest.cpp
        tests/Units/UnitsTest.cpp
        tests/Geometry/AttitudeEngineTest.cpp)

# 精准挂载：只要求我们的测试代码做到零警告
target_compile_options(AegisMathLib_Tests PRIVATE ${AEGIS_STRICT_WARNINGS})

target_link_libraries(AegisMathLib_Tests
        PRIVATE
        AegisMathLib
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(AegisMathLib_Tests)