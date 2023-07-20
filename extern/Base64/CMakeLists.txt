add_library(Base64 INTERFACE)
target_include_directories(Base64 INTERFACE .)

if (MSVC)
    target_precompile_headers(Base64 INTERFACE <string>)
endif()
