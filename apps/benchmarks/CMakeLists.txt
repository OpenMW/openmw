if(OPENMW_USE_SYSTEM_BENCHMARK)
    find_package(benchmark REQUIRED)
endif()

add_subdirectory(detournavigator)
add_subdirectory(esm)
add_subdirectory(settings)
