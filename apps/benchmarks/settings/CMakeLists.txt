openmw_add_executable(openmw_settings_access_benchmark access.cpp)
target_link_libraries(openmw_settings_access_benchmark benchmark::benchmark components)

target_compile_definitions(openmw_settings_access_benchmark
    PRIVATE OPENMW_PROJECT_SOURCE_DIR=u8"${PROJECT_SOURCE_DIR}")

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw_settings_access_benchmark ${CMAKE_THREAD_LIBS_INIT})
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw_settings_access_benchmark PRIVATE <algorithm>)
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw_settings_access_benchmark PRIVATE --coverage)
    target_link_libraries(openmw_settings_access_benchmark gcov)
endif()

if (WIN32)
    target_sources(openmw_settings_access_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/files/windows/other-apps.manifest)
endif()
