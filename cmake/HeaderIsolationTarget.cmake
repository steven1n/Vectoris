# Header probes are compile-only. Visual Studio represents OBJECT libraries as
# StaticLibrary projects, so invoke only their compilation targets on that backend.
include_guard(GLOBAL)

function(vectoris_add_header_isolation_target target_name)
    cmake_parse_arguments(PROBE "" "" "SOURCES;LIBRARIES" ${ARGN})
    set(object_target "${target_name}")
    if(CMAKE_GENERATOR MATCHES "^Visual Studio")
        set(object_target "${target_name}_Objects")
        add_library(${object_target} OBJECT EXCLUDE_FROM_ALL ${PROBE_SOURCES})
        set_target_properties(${object_target} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
        if(NOT CMAKE_VS_MSBUILD_COMMAND OR NOT CMAKE_VS_PLATFORM_NAME)
            message(FATAL_ERROR "HeaderIsolation requires the detected MSBuild command and platform")
        endif()
        list(LENGTH PROBE_SOURCES probe_count)
        add_custom_target(${target_name} ALL
            COMMAND "${CMAKE_VS_MSBUILD_COMMAND}"
                "${CMAKE_CURRENT_BINARY_DIR}/${object_target}.vcxproj"
                /nologo /t:PrepareForBuild,ClCompile
                "/p:Configuration=$<CONFIG>"
                "/p:Platform=${CMAKE_VS_PLATFORM_NAME}"
                /p:BuildProjectReferences=false
            COMMAND "${CMAKE_COMMAND}" -E echo
                "VECTORIS_HEADER_ISOLATION_COMPLETE ${target_name} ${probe_count}"
            DEPENDS ${PROBE_SOURCES}
            VERBATIM
        )
    else()
        add_library(${object_target} OBJECT ${PROBE_SOURCES})
    endif()
    target_link_libraries(${object_target} PRIVATE ${PROBE_LIBRARIES})
    target_compile_options(${object_target} PRIVATE ${VECTORIS_STRICT_WARNINGS})
    set_target_properties(${object_target} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    if(COMMAND vectoris_apply_sanitizers)
        vectoris_apply_sanitizers(${object_target})
    elseif(COMMAND aegismath_apply_sanitizers)
        aegismath_apply_sanitizers(${object_target})
    endif()
endfunction()
