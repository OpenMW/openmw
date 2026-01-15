openmw_add_executable(openmw_esm_refid_benchmark benchrefid.cpp)
target_link_libraries(openmw_esm_refid_benchmark benchmark::benchmark components)

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw_esm_refid_benchmark ${CMAKE_THREAD_LIBS_INIT})
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw_esm_refid_benchmark REUSE_FROM components)
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw_esm_refid_benchmark PRIVATE --coverage)
    target_link_libraries(openmw_esm_refid_benchmark gcov)
endif()
