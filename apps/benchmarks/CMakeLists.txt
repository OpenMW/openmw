if(OPENMW_USE_SYSTEM_BENCHMARK)
    find_package(benchmark REQUIRED)
endif()

openmw_add_executable(openmw_detournavigator_navmeshtilescache_benchmark detournavigator/navmeshtilescache.cpp)
target_compile_features(openmw_detournavigator_navmeshtilescache_benchmark PRIVATE cxx_std_17)
target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark benchmark::benchmark components)

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark ${CMAKE_THREAD_LIBS_INIT})
endif()
