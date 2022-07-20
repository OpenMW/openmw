add_library(Base64 INTERFACE)
target_include_directories(Base64 INTERFACE .)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(Base64 INTERFACE <string>)
endif()
