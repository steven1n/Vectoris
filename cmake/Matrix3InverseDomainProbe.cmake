# Candidate #11: execute the authoritative inverse-domain negative pair with
# the actual configured compiler (including native MSVC), not a substitute.
set(_vectoris_inverse_probe_root "${PROJECT_SOURCE_DIR}/tools/api_surface/negative_probes")
set(_vectoris_inverse_probe_flags
    "-DCMAKE_CXX_STANDARD=20"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_CXX_EXTENSIONS=OFF"
    "-DINCLUDE_DIRECTORIES=${PROJECT_SOURCE_DIR}/modules/VectorisNumerics/include")
unset(_vectoris_inverse_control CACHE)
try_compile(_vectoris_inverse_control
    "${CMAKE_CURRENT_BINARY_DIR}/inverse-domain-control"
    "${_vectoris_inverse_probe_root}/matrix_integer_inverse_control.cpp"
    CMAKE_FLAGS ${_vectoris_inverse_probe_flags}
    OUTPUT_VARIABLE _vectoris_inverse_control_output)
if(NOT _vectoris_inverse_control)
    message(FATAL_ERROR "Matrix3 floating inverse control failed: ${_vectoris_inverse_control_output}")
endif()
unset(_vectoris_inverse_negative CACHE)
try_compile(_vectoris_inverse_negative
    "${CMAKE_CURRENT_BINARY_DIR}/inverse-domain-negative"
    "${_vectoris_inverse_probe_root}/matrix_integer_inverse.cpp"
    CMAKE_FLAGS ${_vectoris_inverse_probe_flags}
    OUTPUT_VARIABLE _vectoris_inverse_negative_output)
if(_vectoris_inverse_negative OR NOT _vectoris_inverse_negative_output MATCHES "TryInverse")
    message(FATAL_ERROR "Matrix3 integral inverse must fail at TryInverse: ${_vectoris_inverse_negative_output}")
endif()
# Keep expected compiler errors in an evidence file, out of formal build logs.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/matrix3-inverse-domain-evidence.txt"
    "compiler=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
    "control=compiled\nintegral=rejected\n${_vectoris_inverse_negative_output}")
message(STATUS "VECTORIS_MATRIX3_INVERSE_DOMAIN: floating control compiled; integral TryInverse rejected")
